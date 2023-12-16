/***
  This file is part of systemd.

  Copyright (C) 2014 Intel Corporation. All rights reserved.

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <netinet/icmp6.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>

#include "socket-util.h"
#include "refcnt.h"
#include "async.h"

#include "dhcp6-internal.h"
#include "sd-icmp6-nd.h"

#define ICMP6_ROUTER_SOLICITATION_INTERVAL      4 * USEC_PER_SEC
#define ICMP6_MAX_ROUTER_SOLICITATIONS          3

enum icmp6_nd_state {
        ICMP6_NEIGHBOR_DISCOVERY_IDLE           = 0,
        ICMP6_ROUTER_SOLICITATION_SENT          = 10,
        ICMP6_ROUTER_ADVERTISMENT_LISTEN        = 11,
};

struct sd_icmp6_nd {
        RefCount n_ref;

        enum icmp6_nd_state state;
        sd_event *event;
        int event_priority;
        int index;
        struct ether_addr mac_addr;
        int fd;
        sd_event_source *recv;
        sd_event_source *timeout;
        int nd_sent;
        sd_icmp6_nd_callback_t callback;
        void *userdata;
};

#define log_icmp6_nd(p, fmt, ...) log_meta(LOG_DEBUG, __FILE__, __LINE__, __func__, "ICMPv6 CLIENT: " fmt, ##__VA_ARGS__)

static void icmp6_nd_notify(sd_icmp6_nd *nd, int event)
{
        if (nd->callback)
                nd->callback(nd, event, nd->userdata);
}

int sd_icmp6_nd_set_callback(sd_icmp6_nd *nd, sd_icmp6_nd_callback_t callback,
                             void *userdata) {
        assert(nd);

        nd->callback = callback;
        nd->userdata = userdata;

        return 0;
}

int sd_icmp6_nd_set_index(sd_icmp6_nd *nd, int interface_index) {
        assert(nd);
        assert(interface_index >= -1);

        nd->index = interface_index;

        return 0;
}

int sd_icmp6_nd_set_mac(sd_icmp6_nd *nd, const struct ether_addr *mac_addr) {
        assert(nd);

        if (mac_addr)
                memcpy(&nd->mac_addr, mac_addr, sizeof(nd->mac_addr));
        else
                memset(&nd->mac_addr, 0x00, sizeof(nd->mac_addr));

        return 0;

}

int sd_icmp6_nd_attach_event(sd_icmp6_nd *nd, sd_event *event, int priority) {
        int r;

        assert_return(nd, -EINVAL);
        assert_return(!nd->event, -EBUSY);

        if (event)
                nd->event = sd_event_ref(event);
        else {
                r = sd_event_default(&nd->event);
                if (r < 0)
                        return 0;
        }

        nd->event_priority = priority;

        return 0;
}

int sd_icmp6_nd_detach_event(sd_icmp6_nd *nd) {
        assert_return(nd, -EINVAL);

        nd->event = sd_event_unref(nd->event);

        return 0;
}

sd_event *sd_icmp6_nd_get_event(sd_icmp6_nd *nd) {
        assert(nd);

        return nd->event;
}

sd_icmp6_nd *sd_icmp6_nd_ref(sd_icmp6_nd *nd) {
        assert (nd);

        assert_se(REFCNT_INC(nd->n_ref) >= 2);

        return nd;
}

static int icmp6_nd_init(sd_icmp6_nd *nd) {
        assert(nd);

        nd->recv = sd_event_source_unref(nd->recv);
        nd->fd = asynchronous_close(nd->fd);
        nd->timeout = sd_event_source_unref(nd->timeout);

        return 0;
}

sd_icmp6_nd *sd_icmp6_nd_unref(sd_icmp6_nd *nd) {
        if (nd && REFCNT_DEC(nd->n_ref) <= 0) {

                icmp6_nd_init(nd);
                sd_icmp6_nd_detach_event(nd);

                free(nd);
        }

        return NULL;
}

DEFINE_TRIVIAL_CLEANUP_FUNC(sd_icmp6_nd*, sd_icmp6_nd_unref);
#define _cleanup_sd_icmp6_nd_free_ _cleanup_(sd_icmp6_nd_unrefp)

int sd_icmp6_nd_new(sd_icmp6_nd **ret) {
        _cleanup_sd_icmp6_nd_free_ sd_icmp6_nd *nd = NULL;

        assert(ret);

        nd = new0(sd_icmp6_nd, 1);
        if (!nd)
                return -ENOMEM;

        nd->n_ref = REFCNT_INIT;

        nd->index = -1;
        nd->fd = -1;

        *ret = nd;
        nd = NULL;

        return 0;
}

static int icmp6_router_advertisment_recv(sd_event_source *s, int fd,
                                          uint32_t revents, void *userdata)
{
        sd_icmp6_nd *nd = userdata;
        ssize_t len;
        struct nd_router_advert ra;
        int event = ICMP6_EVENT_ROUTER_ADVERTISMENT_NONE;

        assert(s);
        assert(nd);
        assert(nd->event);

        /* only interested in Managed/Other flag */
        len = read(fd, &ra, sizeof(ra));
        if ((size_t)len < sizeof(ra))
                return 0;

        if (ra.nd_ra_type != ND_ROUTER_ADVERT)
                return 0;

        if (ra.nd_ra_code != 0)
                return 0;

        nd->timeout = sd_event_source_unref(nd->timeout);

        nd->state = ICMP6_ROUTER_ADVERTISMENT_LISTEN;

        if (ra.nd_ra_flags_reserved & ND_RA_FLAG_OTHER )
                event = ICMP6_EVENT_ROUTER_ADVERTISMENT_OTHER;

        if (ra.nd_ra_flags_reserved & ND_RA_FLAG_MANAGED)
                event = ICMP6_EVENT_ROUTER_ADVERTISMENT_MANAGED;

        log_icmp6_nd(nd, "Received Router Advertisment flags %s/%s",
                     (ra.nd_ra_flags_reserved & ND_RA_FLAG_MANAGED)? "MANAGED":
                     "none",
                     (ra.nd_ra_flags_reserved & ND_RA_FLAG_OTHER)? "OTHER":
                     "none");

        icmp6_nd_notify(nd, event);

        return 0;
}

static int icmp6_router_solicitation_timeout(sd_event_source *s, uint64_t usec,
                                             void *userdata)
{
        sd_icmp6_nd *nd = userdata;
        uint64_t time_now, next_timeout;
        struct ether_addr unset = { };
        struct ether_addr *addr = NULL;
        int r;

        assert(s);
        assert(nd);
        assert(nd->event);

        nd->timeout = sd_event_source_unref(nd->timeout);

        if (nd->nd_sent >= ICMP6_MAX_ROUTER_SOLICITATIONS) {
                icmp6_nd_notify(nd, ICMP6_EVENT_ROUTER_ADVERTISMENT_TIMEOUT);
                nd->state = ICMP6_ROUTER_ADVERTISMENT_LISTEN;
        } else {
                if (memcmp(&nd->mac_addr, &unset, sizeof(struct ether_addr)))
                        addr = &nd->mac_addr;

                r = dhcp_network_icmp6_send_router_solicitation(nd->fd, addr);
                if (r < 0)
                        log_icmp6_nd(nd, "Error sending Router Solicitation");
                else {
                        nd->state = ICMP6_ROUTER_SOLICITATION_SENT;
                        log_icmp6_nd(nd, "Sent Router Solicitation");
                }

                nd->nd_sent++;

                r = sd_event_now(nd->event, clock_boottime_or_monotonic(), &time_now);
                if (r < 0) {
                        icmp6_nd_notify(nd, r);
                        return 0;
                }

                next_timeout = time_now + ICMP6_ROUTER_SOLICITATION_INTERVAL;

                r = sd_event_add_time(nd->event, &nd->timeout, clock_boottime_or_monotonic(),
                                      next_timeout, 0,
                                      icmp6_router_solicitation_timeout, nd);
                if (r < 0) {
                        icmp6_nd_notify(nd, r);
                        return 0;
                }

                r = sd_event_source_set_priority(nd->timeout,
                                                 nd->event_priority);
                if (r < 0) {
                        icmp6_nd_notify(nd, r);
                        return 0;
                }
        }

        return 0;
}

int sd_icmp6_nd_stop(sd_icmp6_nd *nd) {
        assert_return(nd, -EINVAL);
        assert_return(nd->event, -EINVAL);

        log_icmp6_nd(client, "Stop ICMPv6");

        icmp6_nd_init(nd);

        nd->state = ICMP6_NEIGHBOR_DISCOVERY_IDLE;

        return 0;
}

int sd_icmp6_router_solicitation_start(sd_icmp6_nd *nd) {
        int r;

        assert(nd);
        assert(nd->event);

        if (nd->state != ICMP6_NEIGHBOR_DISCOVERY_IDLE)
                return -EINVAL;

        if (nd->index < 1)
                return -EINVAL;

        r = dhcp_network_icmp6_bind_router_solicitation(nd->index);
        if (r < 0)
                return r;

        nd->fd = r;

        r = sd_event_add_io(nd->event, &nd->recv, nd->fd, EPOLLIN,
                            icmp6_router_advertisment_recv, nd);
        if (r < 0)
                goto error;

        r = sd_event_source_set_priority(nd->recv, nd->event_priority);
        if (r < 0)
                goto error;

        r = sd_event_add_time(nd->event, &nd->timeout, clock_boottime_or_monotonic(),
                              0, 0, icmp6_router_solicitation_timeout, nd);
        if (r < 0)
                goto error;

        r = sd_event_source_set_priority(nd->timeout, nd->event_priority);

error:
        if (r < 0)
                icmp6_nd_init(nd);
        else
                log_icmp6_nd(client, "Start Router Solicitation");

        return r;
}
