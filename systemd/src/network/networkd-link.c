/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2013 Tom Gundersen <teg@jklm.no>

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

#include <netinet/ether.h>
#include <linux/if.h>
#include <unistd.h>

#include "networkd-link.h"
#include "networkd-netdev.h"
#include "libudev-private.h"
#include "udev-util.h"
#include "util.h"
#include "virt.h"
#include "bus-util.h"
#include "network-internal.h"
#include "conf-parser.h"

#include "dhcp-lease-internal.h"

static int link_new(Manager *manager, sd_rtnl_message *message, Link **ret) {
        _cleanup_link_unref_ Link *link = NULL;
        uint16_t type;
        const char *ifname;
        int r, ifindex;

        assert(manager);
        assert(message);
        assert(ret);

        r = sd_rtnl_message_get_type(message, &type);
        if (r < 0)
                return r;
        else if (type != RTM_NEWLINK)
                return -EINVAL;

        r = sd_rtnl_message_link_get_ifindex(message, &ifindex);
        if (r < 0)
                return r;
        else if (ifindex <= 0)
                return -EINVAL;

        r = sd_rtnl_message_read_string(message, IFLA_IFNAME, &ifname);
        if (r < 0)
                return r;

        link = new0(Link, 1);
        if (!link)
                return -ENOMEM;

        link->n_ref = 1;
        link->manager = manager;
        link->state = LINK_STATE_PENDING;
        link->ifindex = ifindex;
        link->ifname = strdup(ifname);
        if (!link->ifname)
                return -ENOMEM;

        r = sd_rtnl_message_read_ether_addr(message, IFLA_ADDRESS, &link->mac);
        if (r < 0)
                log_debug_link(link, "MAC address not found for new device, continuing without");

        r = asprintf(&link->state_file, "/run/systemd/netif/links/%d",
                     link->ifindex);
        if (r < 0)
                return -ENOMEM;

        r = asprintf(&link->lease_file, "/run/systemd/netif/leases/%d",
                     link->ifindex);
        if (r < 0)
                return -ENOMEM;

        r = hashmap_ensure_allocated(&manager->links, NULL, NULL);
        if (r < 0)
                return r;

        r = hashmap_put(manager->links, INT_TO_PTR(link->ifindex), link);
        if (r < 0)
                return r;

        *ret = link;
        link = NULL;

        return 0;
}

static void link_free(Link *link) {
        Address *address;

        if (!link)
                return;

        while ((address = link->addresses)) {
                LIST_REMOVE(addresses, link->addresses, address);
                address_free(address);
        }

        while ((address = link->pool_addresses)) {
                LIST_REMOVE(addresses, link->pool_addresses, address);
                address_free(address);
        }

        sd_dhcp_client_unref(link->dhcp_client);
        sd_dhcp_lease_unref(link->dhcp_lease);

        unlink(link->lease_file);
        free(link->lease_file);

        sd_ipv4ll_unref(link->ipv4ll);
        sd_dhcp6_client_unref(link->dhcp6_client);
        sd_icmp6_nd_unref(link->icmp6_router_discovery);

        if (link->manager)
                hashmap_remove(link->manager->links, INT_TO_PTR(link->ifindex));

        free(link->ifname);

        unlink(link->state_file);
        free(link->state_file);

        udev_device_unref(link->udev_device);

        free(link);
}

Link *link_unref(Link *link) {
        if (link && (-- link->n_ref <= 0))
                link_free(link);

        return NULL;
}

Link *link_ref(Link *link) {
        if (link)
                assert_se(++ link->n_ref >= 2);

        return link;
}

int link_get(Manager *m, int ifindex, Link **ret) {
        Link *link;

        assert(m);
        assert(ifindex);
        assert(ret);

        link = hashmap_get(m->links, INT_TO_PTR(ifindex));
        if (!link)
                return -ENODEV;

        *ret = link;

        return 0;
}

void link_drop(Link *link) {
        if (!link || link->state == LINK_STATE_LINGER)
                return;

        link->state = LINK_STATE_LINGER;

        log_debug_link(link, "link removed");

        link_unref(link);

        return;
}

static void link_enter_unmanaged(Link *link) {
        assert(link);

        log_debug_link(link, "unmanaged");

        link->state = LINK_STATE_UNMANAGED;

        link_save(link);
}

static int link_stop_clients(Link *link) {
        int r = 0, k;

        assert(link);
        assert(link->manager);
        assert(link->manager->event);

        if (!link->network)
                return 0;

        if (link->dhcp_client) {
                k = sd_dhcp_client_stop(link->dhcp_client);
                if (k < 0) {
                        log_warning_link(link, "Could not stop DHCPv4 client: %s",
                                         strerror(-r));
                        r = k;
                }
        }

        if (link->ipv4ll) {
                k = sd_ipv4ll_stop(link->ipv4ll);
                if (k < 0) {
                        log_warning_link(link, "Could not stop IPv4 link-local: %s",
                                         strerror(-r));
                        r = k;
                }
        }

        if (link->dhcp_server) {
                k = sd_dhcp_server_stop(link->dhcp_server);
                if (k < 0) {
                        log_warning_link(link, "Could not stop DHCPv4 server: %s",
                                         strerror(-r));
                        r = k;
                }
        }

        if(link->icmp6_router_discovery) {

                if (link->dhcp6_client) {
                        k = sd_dhcp6_client_stop(link->dhcp6_client);
                        if (k < 0) {
                                log_warning_link(link, "Could not stop DHCPv6 client: %s",
                                                 strerror(-r));
                                r = k;
                        }
                }

                k = sd_icmp6_nd_stop(link->icmp6_router_discovery);
                if (k < 0) {
                        log_warning_link(link,
                                         "Could not stop ICMPv6 router discovery: %s",
                                         strerror(-r));
                        r = k;
                }
        }

        return r;
}

void link_enter_failed(Link *link) {
        assert(link);

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return;

        log_warning_link(link, "failed");

        link->state = LINK_STATE_FAILED;

        link_stop_clients(link);

        link_save(link);
}

static Address* link_find_dhcp_server_address(Link *link) {
        Address *address;

        assert(link);
        assert(link->network);

        /* The the first statically configured address if there is any */
        LIST_FOREACH(addresses, address, link->network->static_addresses) {

                if (address->family != AF_INET)
                        continue;

                if (in_addr_is_null(address->family, &address->in_addr))
                        continue;

                return address;
        }

        /* If that didn't work, find a suitable address we got from the pool */
        LIST_FOREACH(addresses, address, link->pool_addresses) {
                if (address->family != AF_INET)
                        continue;

                return address;
        }

        return NULL;
}

static int link_enter_configured(Link *link) {
        int r;

        assert(link);
        assert(link->network);
        assert(link->state == LINK_STATE_SETTING_ROUTES);

        if (link->network->dhcp_server &&
            !sd_dhcp_server_is_running(link->dhcp_server)) {
                struct in_addr pool_start;
                Address *address;

                address = link_find_dhcp_server_address(link);
                if (!address) {
                        log_warning_link(link,
                                         "Failed to find suitable address for DHCPv4 server instance.");
                        link_enter_failed(link);
                        return 0;
                }

                log_debug_link(link, "offering DHCPv4 leases");

                r = sd_dhcp_server_set_address(link->dhcp_server,
                                               &address->in_addr.in,
                                               address->prefixlen);
                if (r < 0)
                        return r;

                /* offer 32 addresses starting from the address following the server address */
                pool_start.s_addr = htobe32(be32toh(address->in_addr.in.s_addr) + 1);
                r = sd_dhcp_server_set_lease_pool(link->dhcp_server,
                                                  &pool_start, 32);
                if (r < 0)
                        return r;

                /* TODO:
                r = sd_dhcp_server_set_router(link->dhcp_server,
                                              &main_address->in_addr.in);
                if (r < 0)
                        return r;

                r = sd_dhcp_server_set_prefixlen(link->dhcp_server,
                                                 main_address->prefixlen);
                if (r < 0)
                        return r;
                */

                r = sd_dhcp_server_start(link->dhcp_server);
                if (r < 0) {
                        log_warning_link(link, "could not start DHCPv4 server "
                                         "instance: %s", strerror(-r));

                        link_enter_failed(link);

                        return 0;
                }
        }

        log_info_link(link, "link configured");

        link->state = LINK_STATE_CONFIGURED;

        link_save(link);

        return 0;
}

void link_client_handler(Link *link) {
        assert(link);
        assert(link->network);

        if (!link->static_configured)
                return;

        if (link->network->ipv4ll)
                if (!link->ipv4ll_address ||
                    !link->ipv4ll_route)
                        return;

        if (IN_SET(link->network->dhcp, DHCP_SUPPORT_BOTH, DHCP_SUPPORT_V4))
                if (!link->dhcp4_configured)
                        return;

        if (link->state != LINK_STATE_CONFIGURED)
                link_enter_configured(link);

        return;
}

static int route_handler(sd_rtnl *rtnl, sd_rtnl_message *m, void *userdata) {
        _cleanup_link_unref_ Link *link = userdata;
        int r;

        assert(link->link_messages > 0);
        assert(IN_SET(link->state, LINK_STATE_SETTING_ADDRESSES,
                      LINK_STATE_SETTING_ROUTES, LINK_STATE_FAILED,
                      LINK_STATE_LINGER));

        link->link_messages --;

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return 1;

        r = sd_rtnl_message_get_errno(m);
        if (r < 0 && r != -EEXIST)
                log_struct_link(LOG_WARNING, link,
                                "MESSAGE=%-*s: could not set route: %s",
                                IFNAMSIZ,
                                link->ifname, strerror(-r),
                                "ERRNO=%d", -r,
                                NULL);

        if (link->link_messages == 0) {
                log_debug_link(link, "routes set");
                link->static_configured = true;
                link_client_handler(link);
        }

        return 1;
}

static int link_enter_set_routes(Link *link) {
        Route *rt;
        int r;

        assert(link);
        assert(link->network);
        assert(link->state == LINK_STATE_SETTING_ADDRESSES);

        link->state = LINK_STATE_SETTING_ROUTES;

        LIST_FOREACH(routes, rt, link->network->static_routes) {
                r = route_configure(rt, link, &route_handler);
                if (r < 0) {
                        log_warning_link(link,
                                         "could not set routes: %s",
                                         strerror(-r));
                        link_enter_failed(link);
                        return r;
                }

                link->link_messages ++;
        }

        if (link->link_messages == 0) {
                link->static_configured = true;
                link_client_handler(link);
        } else
                log_debug_link(link, "setting routes");

        return 0;
}

int link_route_drop_handler(sd_rtnl *rtnl, sd_rtnl_message *m, void *userdata) {
        _cleanup_link_unref_ Link *link = userdata;
        int r;

        assert(m);
        assert(link);
        assert(link->ifname);

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return 1;

        r = sd_rtnl_message_get_errno(m);
        if (r < 0 && r != -ESRCH)
                log_struct_link(LOG_WARNING, link,
                                "MESSAGE=%-*s: could not drop route: %s",
                                IFNAMSIZ,
                                link->ifname, strerror(-r),
                                "ERRNO=%d", -r,
                                NULL);

        return 1;
}

int link_get_address_handler(sd_rtnl *rtnl, sd_rtnl_message *m, void *userdata) {
        _cleanup_link_unref_ Link *link = userdata;
        int r;

        assert(rtnl);
        assert(m);
        assert(link);
        assert(link->manager);

        for (; m; m = sd_rtnl_message_next(m)) {
                r = sd_rtnl_message_get_errno(m);
                if (r < 0) {
                        log_debug_link(link, "getting address failed: %s",
                                       strerror(-r));
                        continue;
                }

                r = link_rtnl_process_address(rtnl, m, link->manager);
                if (r < 0)
                        log_warning_link(link, "could not process address: %s",
                                         strerror(-r));
        }

        return 1;
}

static int address_handler(sd_rtnl *rtnl, sd_rtnl_message *m, void *userdata) {
        _cleanup_link_unref_ Link *link = userdata;
        int r;

        assert(rtnl);
        assert(m);
        assert(link);
        assert(link->ifname);
        assert(link->link_messages > 0);
        assert(IN_SET(link->state, LINK_STATE_SETTING_ADDRESSES,
               LINK_STATE_FAILED, LINK_STATE_LINGER));

        link->link_messages --;

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return 1;

        r = sd_rtnl_message_get_errno(m);
        if (r < 0 && r != -EEXIST)
                log_struct_link(LOG_WARNING, link,
                                "MESSAGE=%-*s: could not set address: %s",
                                IFNAMSIZ,
                                link->ifname, strerror(-r),
                                "ERRNO=%d", -r,
                                NULL);
        else if (r >= 0) {
                /* calling handler directly so take a ref */
                link_ref(link);
                link_get_address_handler(rtnl, m, link);
        }

        if (link->link_messages == 0) {
                log_debug_link(link, "addresses set");
                link_enter_set_routes(link);
        }

        return 1;
}

static int link_enter_set_addresses(Link *link) {
        Address *ad;
        int r;

        assert(link);
        assert(link->network);
        assert(link->state != _LINK_STATE_INVALID);

        link->state = LINK_STATE_SETTING_ADDRESSES;

        LIST_FOREACH(addresses, ad, link->network->static_addresses) {
                r = address_configure(ad, link, &address_handler);
                if (r < 0) {
                        log_warning_link(link,
                                         "could not set addresses: %s",
                                         strerror(-r));
                        link_enter_failed(link);
                        return r;
                }

                link->link_messages ++;
        }

        if (link->link_messages == 0) {
                link_enter_set_routes(link);
        } else
                log_debug_link(link, "setting addresses");

        return 0;
}

int link_address_drop_handler(sd_rtnl *rtnl, sd_rtnl_message *m, void *userdata) {
        _cleanup_link_unref_ Link *link = userdata;
        int r;

        assert(m);
        assert(link);
        assert(link->ifname);

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return 1;

        r = sd_rtnl_message_get_errno(m);
        if (r < 0 && r != -EADDRNOTAVAIL)
                log_struct_link(LOG_WARNING, link,
                                "MESSAGE=%-*s: could not drop address: %s",
                                IFNAMSIZ,
                                link->ifname, strerror(-r),
                                "ERRNO=%d", -r,
                                NULL);

        return 1;
}

static int set_hostname_handler(sd_bus *bus, sd_bus_message *m, void *userdata,
                                sd_bus_error *ret_error) {
        _cleanup_link_unref_ Link *link = userdata;
        int r;

        assert(link);

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return 1;

        r = sd_bus_message_get_errno(m);
        if (r < 0)
                r = -r;
        if (r > 0)
                log_warning_link(link, "Could not set hostname: %s",
                                 strerror(r));

        return 1;
}

int link_set_hostname(Link *link, const char *hostname) {
        _cleanup_bus_message_unref_ sd_bus_message *m = NULL;
        int r = 0;

        assert(link);
        assert(link->manager);
        assert(hostname);

        log_debug_link(link, "Setting transient hostname: '%s'", hostname);

        if (!link->manager->bus) {
                /* TODO: replace by assert when we can rely on kdbus */
                log_info_link(link,
                              "Not connected to system bus, ignoring transient hostname.");
                return 0;
        }

        r = sd_bus_message_new_method_call(
                        link->manager->bus,
                        &m,
                        "org.freedesktop.hostname1",
                        "/org/freedesktop/hostname1",
                        "org.freedesktop.hostname1",
                        "SetHostname");
        if (r < 0)
                return r;

        r = sd_bus_message_append(m, "sb", hostname, false);
        if (r < 0)
                return r;

        r = sd_bus_call_async(link->manager->bus, NULL, m, set_hostname_handler,
                              link, 0);
        if (r < 0) {
                log_error_link(link, "Could not set transient hostname: %s",
                               strerror(-r));
                return r;
        }

        link_ref(link);

        return 0;
}

static int set_mtu_handler(sd_rtnl *rtnl, sd_rtnl_message *m, void *userdata) {
        _cleanup_link_unref_ Link *link = userdata;
        int r;

        assert(m);
        assert(link);
        assert(link->ifname);

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return 1;

        r = sd_rtnl_message_get_errno(m);
        if (r < 0)
                log_struct_link(LOG_WARNING, link,
                                "MESSAGE=%-*s: could not set MTU: %s",
                                IFNAMSIZ, link->ifname, strerror(-r),
                                "ERRNO=%d", -r,
                                NULL);

        return 1;
}

int link_set_mtu(Link *link, uint32_t mtu) {
        _cleanup_rtnl_message_unref_ sd_rtnl_message *req = NULL;
        int r;

        assert(link);
        assert(link->manager);
        assert(link->manager->rtnl);

        log_debug_link(link, "setting MTU: %" PRIu32, mtu);

        r = sd_rtnl_message_new_link(link->manager->rtnl, &req,
                                     RTM_SETLINK, link->ifindex);
        if (r < 0) {
                log_error_link(link, "Could not allocate RTM_SETLINK message");
                return r;
        }

        r = sd_rtnl_message_append_u32(req, IFLA_MTU, mtu);
        if (r < 0) {
                log_error_link(link, "Could not append MTU: %s", strerror(-r));
                return r;
        }

        r = sd_rtnl_call_async(link->manager->rtnl, req, set_mtu_handler, link,
                               0, NULL);
        if (r < 0) {
                log_error_link(link,
                               "Could not send rtnetlink message: %s",
                               strerror(-r));
                return r;
        }

        link_ref(link);

        return 0;
}

static void dhcp6_handler(sd_dhcp6_client *client, int event, void *userdata) {
        Link *link = userdata;

        assert(link);
        assert(link->network);
        assert(link->manager);

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return;

        switch(event) {
        case DHCP6_EVENT_STOP:
        case DHCP6_EVENT_RESEND_EXPIRE:
        case DHCP6_EVENT_RETRANS_MAX:
        case DHCP6_EVENT_IP_ACQUIRE:
                log_debug_link(link, "DHCPv6 event %d", event);

                break;

        default:
                if (event < 0)
                        log_warning_link(link, "DHCPv6 error: %s",
                                         strerror(-event));
                else
                        log_warning_link(link, "DHCPv6 unknown event: %d",
                                         event);
                return;
        }
}

static void icmp6_router_handler(sd_icmp6_nd *nd, int event, void *userdata) {
        Link *link = userdata;
        int r;

        assert(link);
        assert(link->network);
        assert(link->manager);

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return;

        switch(event) {
        case ICMP6_EVENT_ROUTER_ADVERTISMENT_NONE:
        case ICMP6_EVENT_ROUTER_ADVERTISMENT_OTHER:
                return;

        case ICMP6_EVENT_ROUTER_ADVERTISMENT_TIMEOUT:
        case ICMP6_EVENT_ROUTER_ADVERTISMENT_MANAGED:
                break;

        default:
                if (event < 0)
                        log_warning_link(link, "ICMPv6 error: %s",
                                         strerror(-event));
                else
                        log_warning_link(link, "ICMPv6 unknown event: %d",
                                         event);

                return;
        }

        if (link->dhcp6_client)
                return;

        r = sd_dhcp6_client_new(&link->dhcp6_client);
        if (r < 0)
                return;

        r = sd_dhcp6_client_attach_event(link->dhcp6_client, NULL, 0);
        if (r < 0) {
                link->dhcp6_client = sd_dhcp6_client_unref(link->dhcp6_client);
                return;
        }

        r = sd_dhcp6_client_set_mac(link->dhcp6_client, &link->mac);
        if (r < 0) {
                link->dhcp6_client = sd_dhcp6_client_unref(link->dhcp6_client);
                return;
        }

        r = sd_dhcp6_client_set_index(link->dhcp6_client, link->ifindex);
        if (r < 0) {
                link->dhcp6_client = sd_dhcp6_client_unref(link->dhcp6_client);
                return;
        }

        r = sd_dhcp6_client_set_callback(link->dhcp6_client, dhcp6_handler,
                                         link);
        if (r < 0) {
                link->dhcp6_client = sd_dhcp6_client_unref(link->dhcp6_client);
                return;
        }

        r = sd_dhcp6_client_start(link->dhcp6_client);
        if (r < 0)
                link->dhcp6_client = sd_dhcp6_client_unref(link->dhcp6_client);
}

static int link_acquire_conf(Link *link) {
        int r;

        assert(link);
        assert(link->network);
        assert(link->manager);
        assert(link->manager->event);

        if (link->network->ipv4ll) {
                assert(link->ipv4ll);

                log_debug_link(link, "acquiring IPv4 link-local address");

                r = sd_ipv4ll_start(link->ipv4ll);
                if (r < 0) {
                        log_warning_link(link, "could not acquire IPv4 "
                                         "link-local address");
                        return r;
                }
        }

        if (IN_SET(link->network->dhcp, DHCP_SUPPORT_BOTH, DHCP_SUPPORT_V4)) {
                assert(link->dhcp_client);

                log_debug_link(link, "acquiring DHCPv4 lease");

                r = sd_dhcp_client_start(link->dhcp_client);
                if (r < 0) {
                        log_warning_link(link, "could not acquire DHCPv4 "
                                         "lease");
                        return r;
                }
        }

        if (IN_SET(link->network->dhcp, DHCP_SUPPORT_BOTH, DHCP_SUPPORT_V6)) {
                assert(link->icmp6_router_discovery);

                log_debug_link(link, "discovering IPv6 routers");

                r = sd_icmp6_router_solicitation_start(link->icmp6_router_discovery);
                if (r < 0) {
                        log_warning_link(link,
                                         "could not start IPv6 router discovery");
                        return r;
                }
        }

        return 0;
}

bool link_has_carrier(unsigned flags, uint8_t operstate) {
        /* see Documentation/networking/operstates.txt in the kernel sources */

        if (operstate == IF_OPER_UP)
                return true;

        if (operstate == IF_OPER_UNKNOWN)
                /* operstate may not be implemented, so fall back to flags */
                if ((flags & IFF_LOWER_UP) && !(flags & IFF_DORMANT))
                        return true;

        return false;
}

#define FLAG_STRING(string, flag, old, new) \
        (((old ^ new) & flag) \
                ? ((old & flag) ? (" -" string) : (" +" string)) \
                : "")

static int link_update_flags(Link *link, sd_rtnl_message *m) {
        unsigned flags, unknown_flags_added, unknown_flags_removed, unknown_flags;
        uint8_t operstate;
        bool carrier_gained = false, carrier_lost = false;
        int r;

        assert(link);

        r = sd_rtnl_message_link_get_flags(m, &flags);
        if (r < 0) {
                log_warning_link(link, "Could not get link flags");
                return r;
        }

        r = sd_rtnl_message_read_u8(m, IFLA_OPERSTATE, &operstate);
        if (r < 0)
                /* if we got a message without operstate, take it to mean
                   the state was unchanged */
                operstate = link->kernel_operstate;

        if ((link->flags == flags) && (link->kernel_operstate == operstate))
                return 0;

        if (link->flags != flags) {
                log_debug_link(link, "flags change:%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
                               FLAG_STRING("LOOPBACK", IFF_LOOPBACK, link->flags, flags),
                               FLAG_STRING("MASTER", IFF_MASTER, link->flags, flags),
                               FLAG_STRING("SLAVE", IFF_SLAVE, link->flags, flags),
                               FLAG_STRING("UP", IFF_UP, link->flags, flags),
                               FLAG_STRING("DORMANT", IFF_DORMANT, link->flags, flags),
                               FLAG_STRING("LOWER_UP", IFF_LOWER_UP, link->flags, flags),
                               FLAG_STRING("RUNNING", IFF_RUNNING, link->flags, flags),
                               FLAG_STRING("MULTICAST", IFF_MULTICAST, link->flags, flags),
                               FLAG_STRING("BROADCAST", IFF_BROADCAST, link->flags, flags),
                               FLAG_STRING("POINTOPOINT", IFF_POINTOPOINT, link->flags, flags),
                               FLAG_STRING("PROMISC", IFF_PROMISC, link->flags, flags),
                               FLAG_STRING("ALLMULTI", IFF_ALLMULTI, link->flags, flags),
                               FLAG_STRING("PORTSEL", IFF_PORTSEL, link->flags, flags),
                               FLAG_STRING("AUTOMEDIA", IFF_AUTOMEDIA, link->flags, flags),
                               FLAG_STRING("DYNAMIC", IFF_DYNAMIC, link->flags, flags),
                               FLAG_STRING("NOARP", IFF_NOARP, link->flags, flags),
                               FLAG_STRING("NOTRAILERS", IFF_NOTRAILERS, link->flags, flags),
                               FLAG_STRING("DEBUG", IFF_DEBUG, link->flags, flags),
                               FLAG_STRING("ECHO", IFF_ECHO, link->flags, flags));

                unknown_flags = ~(IFF_LOOPBACK | IFF_MASTER | IFF_SLAVE | IFF_UP |
                                  IFF_DORMANT | IFF_LOWER_UP | IFF_RUNNING |
                                  IFF_MULTICAST | IFF_BROADCAST | IFF_POINTOPOINT |
                                  IFF_PROMISC | IFF_ALLMULTI | IFF_PORTSEL |
                                  IFF_AUTOMEDIA | IFF_DYNAMIC | IFF_NOARP |
                                  IFF_NOTRAILERS | IFF_DEBUG | IFF_ECHO);
                unknown_flags_added = ((link->flags ^ flags) & flags & unknown_flags);
                unknown_flags_removed = ((link->flags ^ flags) & link->flags & unknown_flags);

                /* link flags are currently at most 18 bits, let's align to
                 * printing 20 */
                if (unknown_flags_added)
                        log_debug_link(link,
                                       "unknown link flags gained: %#.5x (ignoring)",
                                       unknown_flags_added);

                if (unknown_flags_removed)
                        log_debug_link(link,
                                       "unknown link flags lost: %#.5x (ignoring)",
                                       unknown_flags_removed);
        }

        carrier_gained = !link_has_carrier(link->flags, link->kernel_operstate) &&
                       link_has_carrier(flags, operstate);
        carrier_lost = link_has_carrier(link->flags, link->kernel_operstate) &&
                         !link_has_carrier(flags, operstate);

        link->flags = flags;
        link->kernel_operstate = operstate;

        link_save(link);

        if (link->state == LINK_STATE_FAILED ||
            link->state == LINK_STATE_UNMANAGED)
                return 0;

        if (carrier_gained) {
                log_info_link(link, "gained carrier");

                if (link->network) {
                        r = link_acquire_conf(link);
                        if (r < 0) {
                                link_enter_failed(link);
                                return r;
                        }
                }
        } else if (carrier_lost) {
                log_info_link(link, "lost carrier");

                r = link_stop_clients(link);
                if (r < 0) {
                        link_enter_failed(link);
                        return r;
                }
        }

        return 0;
}

static int link_up_handler(sd_rtnl *rtnl, sd_rtnl_message *m, void *userdata) {
        _cleanup_link_unref_ Link *link = userdata;
        int r;

        assert(link);

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return 1;

        r = sd_rtnl_message_get_errno(m);
        if (r < 0) {
                /* we warn but don't fail the link, as it may
                   be brought up later */
                log_struct_link(LOG_WARNING, link,
                                "MESSAGE=%-*s: could not bring up interface: %s",
                                IFNAMSIZ,
                                link->ifname, strerror(-r),
                                "ERRNO=%d", -r,
                                NULL);
        }

        return 1;
}

static int link_up(Link *link) {
        _cleanup_rtnl_message_unref_ sd_rtnl_message *req = NULL;
        int r;

        assert(link);
        assert(link->manager);
        assert(link->manager->rtnl);

        log_debug_link(link, "bringing link up");

        r = sd_rtnl_message_new_link(link->manager->rtnl, &req,
                                     RTM_SETLINK, link->ifindex);
        if (r < 0) {
                log_error_link(link, "Could not allocate RTM_SETLINK message");
                return r;
        }

        r = sd_rtnl_message_link_set_flags(req, IFF_UP, IFF_UP);
        if (r < 0) {
                log_error_link(link, "Could not set link flags: %s",
                               strerror(-r));
                return r;
        }

        r = sd_rtnl_call_async(link->manager->rtnl, req, link_up_handler, link,
                               0, NULL);
        if (r < 0) {
                log_error_link(link,
                               "Could not send rtnetlink message: %s",
                               strerror(-r));
                return r;
        }

        link_ref(link);

        return 0;
}

static int link_joined(Link *link) {
        int r;

        assert(link);
        assert(link->network);

        if (!(link->flags & IFF_UP)) {
                r = link_up(link);
                if (r < 0) {
                        link_enter_failed(link);
                        return r;
                }
        }

        return link_enter_set_addresses(link);
}

static int netdev_join_handler(sd_rtnl *rtnl, sd_rtnl_message *m,
                               void *userdata) {
        _cleanup_link_unref_ Link *link = userdata;
        int r;

        assert(link);
        assert(link->network);

        link->enslaving --;

        if (IN_SET(link->state, LINK_STATE_FAILED, LINK_STATE_LINGER))
                return 1;

        r = sd_rtnl_message_get_errno(m);
        if (r < 0 && r != -EEXIST) {
                log_struct_link(LOG_ERR, link,
                                "MESSAGE=%-*s: could not join netdev: %s",
                                IFNAMSIZ,
                                link->ifname, strerror(-r),
                                "ERRNO=%d", -r,
                                NULL);
                link_enter_failed(link);
                return 1;
        } else
                log_debug_link(link, "joined netdev");

        if (link->enslaving <= 0)
                link_joined(link);

        return 1;
}

static int link_enter_join_netdev(Link *link) {
        NetDev *netdev;
        Iterator i;
        int r;

        assert(link);
        assert(link->network);
        assert(link->state == LINK_STATE_PENDING);

        link->state = LINK_STATE_ENSLAVING;

        link_save(link);

        if (!link->network->bridge &&
            !link->network->bond &&
            hashmap_isempty(link->network->stacked_netdevs))
                return link_joined(link);

        if (link->network->bond) {
                log_struct_link(LOG_DEBUG, link,
                                "MESSAGE=%-*s: enslaving by '%s'",
                                IFNAMSIZ,
                                link->ifname, link->network->bond->ifname,
                                NETDEVIF(link->network->bond),
                                NULL);

                r = netdev_join(link->network->bond, link, &netdev_join_handler);
                if (r < 0) {
                        log_struct_link(LOG_WARNING, link,
                                        "MESSAGE=%-*s: could not join netdev '%s': %s",
                                        IFNAMSIZ,
                                        link->ifname, link->network->bond->ifname,
                                        strerror(-r),
                                        NETDEVIF(link->network->bond),
                                        NULL);
                        link_enter_failed(link);
                        return r;
                }

                link->enslaving ++;
        }

        if (link->network->bridge) {
                log_struct_link(LOG_DEBUG, link,
                                "MESSAGE=%-*s: enslaving by '%s'",
                                IFNAMSIZ,
                                link->ifname, link->network->bridge->ifname,
                                NETDEVIF(link->network->bridge),
                                NULL);

                r = netdev_join(link->network->bridge, link,
                                &netdev_join_handler);
                if (r < 0) {
                        log_struct_link(LOG_WARNING, link,
                                        "MESSAGE=%-*s: could not join netdev '%s': %s",
                                        IFNAMSIZ,
                                        link->ifname, link->network->bridge->ifname,
                                        strerror(-r),
                                        NETDEVIF(link->network->bridge),
                                        NULL);
                        link_enter_failed(link);
                        return r;
                }

                link->enslaving ++;
        }

        HASHMAP_FOREACH(netdev, link->network->stacked_netdevs, i) {
                log_struct_link(LOG_DEBUG, link,
                                "MESSAGE=%-*s: enslaving by '%s'",
                                IFNAMSIZ,
                                link->ifname, netdev->ifname, NETDEVIF(netdev),
                                NULL);

                r = netdev_join(netdev, link, &netdev_join_handler);
                if (r < 0) {
                        log_struct_link(LOG_WARNING, link,
                                        "MESSAGE=%-*s: could not join netdev '%s': %s",
                                        IFNAMSIZ,
                                        link->ifname, netdev->ifname,
                                        strerror(-r),
                                        NETDEVIF(netdev), NULL);
                        link_enter_failed(link);
                        return r;
                }

                link->enslaving ++;
        }

        return 0;
}

static int link_configure(Link *link) {
        int r;

        assert(link);
        assert(link->network);
        assert(link->state == LINK_STATE_PENDING);

        if (link->network->ipv4ll) {
                r = ipv4ll_configure(link);
                if (r < 0)
                        return r;
        }

        if (IN_SET(link->network->dhcp, DHCP_SUPPORT_BOTH, DHCP_SUPPORT_V4)) {
                r = dhcp4_configure(link);
                if (r < 0)
                        return r;
        }

        if (link->network->dhcp_server) {
                r = sd_dhcp_server_new(&link->dhcp_server, link->ifindex);
                if (r < 0)
                        return r;

                r = sd_dhcp_server_attach_event(link->dhcp_server, NULL, 0);
                if (r < 0)
                        return r;
        }

        if (IN_SET(link->network->dhcp, DHCP_SUPPORT_BOTH, DHCP_SUPPORT_V6)) {
                r = sd_icmp6_nd_new(&link->icmp6_router_discovery);
                if (r < 0)
                        return r;

                r = sd_icmp6_nd_attach_event(link->icmp6_router_discovery,
                                             NULL, 0);
                if (r < 0)
                        return r;

                r = sd_icmp6_nd_set_mac(link->icmp6_router_discovery,
                                        &link->mac);
                if (r < 0)
                        return r;

                r = sd_icmp6_nd_set_index(link->icmp6_router_discovery,
                                          link->ifindex);
                if (r < 0)
                        return r;

                r = sd_icmp6_nd_set_callback(link->icmp6_router_discovery,
                                             icmp6_router_handler, link);
                if (r < 0)
                        return r;
        }

        if (link_has_carrier(link->flags, link->kernel_operstate)) {
                r = link_acquire_conf(link);
                if (r < 0)
                        return r;
        }

        return link_enter_join_netdev(link);
}

static int link_initialized_and_synced(sd_rtnl *rtnl, sd_rtnl_message *m,
                                       void *userdata) {
        _cleanup_link_unref_ Link *link = userdata;
        Network *network;
        int r;

        assert(link);
        assert(link->ifname);
        assert(link->manager);

        if (link->state != LINK_STATE_PENDING)
                return 1;

        log_debug_link(link, "link state is up-to-date");

        r = network_get(link->manager, link->udev_device, link->ifname,
                        &link->mac, &network);
        if (r == -ENOENT) {
                link_enter_unmanaged(link);
                return 1;
        } else if (r < 0)
                return r;

        r = network_apply(link->manager, network, link);
        if (r < 0)
                return r;

        r = link_configure(link);
        if (r < 0)
                return r;

        return 1;
}

int link_initialized(Link *link, struct udev_device *device) {
        _cleanup_rtnl_message_unref_ sd_rtnl_message *req = NULL;
        int r;

        assert(link);
        assert(link->manager);
        assert(link->manager->rtnl);
        assert(device);

        if (link->state != LINK_STATE_PENDING)
                return 0;

        if (link->udev_device)
                return 0;

        log_debug_link(link, "udev initialized link");

        link->udev_device = udev_device_ref(device);

        /* udev has initialized the link, but we don't know if we have yet
         * processed the NEWLINK messages with the latest state. Do a GETLINK,
         * when it returns we know that the pending NEWLINKs have already been
         * processed and that we are up-to-date */

        r = sd_rtnl_message_new_link(link->manager->rtnl, &req, RTM_GETLINK,
                                     link->ifindex);
        if (r < 0)
                return r;

        r = sd_rtnl_call_async(link->manager->rtnl, req,
                               link_initialized_and_synced, link, 0, NULL);
        if (r < 0)
                return r;

        link_ref(link);

        return 0;
}

int link_rtnl_process_address(sd_rtnl *rtnl, sd_rtnl_message *message,
                              void *userdata) {
        Manager *m = userdata;
        Link *link = NULL;
        uint16_t type;
        _cleanup_address_free_ Address *address = NULL;
        Address *ad;
        char buf[INET6_ADDRSTRLEN];
        char valid_buf[FORMAT_TIMESPAN_MAX];
        const char *valid_str = NULL;
        bool address_dropped = false;
        int r, ifindex;

        assert(rtnl);
        assert(message);
        assert(m);

        r = sd_rtnl_message_get_type(message, &type);
        if (r < 0) {
                log_warning("rtnl: could not get message type");
                return 0;
        }

        r = sd_rtnl_message_addr_get_ifindex(message, &ifindex);
        if (r < 0 || ifindex <= 0) {
                log_warning("rtnl: received address message without valid ifindex, ignoring");
                return 0;
        } else {
                r = link_get(m, ifindex, &link);
                if (r < 0 || !link) {
                        log_warning("rtnl: received address for a nonexistent link, ignoring");
                        return 0;
                }
        }

        r = address_new_dynamic(&address);
        if (r < 0)
                return r;

        r = sd_rtnl_message_addr_get_family(message, &address->family);
        if (r < 0 || !IN_SET(address->family, AF_INET, AF_INET6)) {
                log_warning_link(link,
                                 "rtnl: received address with invalid family, ignoring");
                return 0;
        }

        r = sd_rtnl_message_addr_get_prefixlen(message, &address->prefixlen);
        if (r < 0) {
                log_warning_link(link,
                                 "rtnl: received address with invalid prefixlen, ignoring");
                return 0;
        }

        r = sd_rtnl_message_addr_get_scope(message, &address->scope);
        if (r < 0) {
                log_warning_link(link,
                                 "rtnl: received address with invalid scope, ignoring");
                return 0;
        }

        r = sd_rtnl_message_addr_get_flags(message, &address->flags);
        if (r < 0) {
                log_warning_link(link,
                                 "rtnl: received address with invalid flags, ignoring");
                return 0;
        }

        switch (address->family) {
        case AF_INET:
                r = sd_rtnl_message_read_in_addr(message, IFA_LOCAL,
                                                 &address->in_addr.in);
                if (r < 0) {
                        log_warning_link(link,
                                         "rtnl: received address without valid address, ignoring");
                        return 0;
                }

                break;

        case AF_INET6:
                r = sd_rtnl_message_read_in6_addr(message, IFA_ADDRESS,
                                                  &address->in_addr.in6);
                if (r < 0) {
                        log_warning_link(link,
                                         "rtnl: received address without valid address, ignoring");
                        return 0;
                }

                break;

        default:
                assert_not_reached("invalid address family");
        }

        if (!inet_ntop(address->family, &address->in_addr, buf,
                       INET6_ADDRSTRLEN)) {
                log_warning_link(link, "could not print address");
                return 0;
        }

        r = sd_rtnl_message_read_cache_info(message, IFA_CACHEINFO,
                                            &address->cinfo);
        if (r >= 0) {
                if (address->cinfo.ifa_valid == CACHE_INFO_INFINITY_LIFE_TIME)
                        valid_str = "ever";
                else
                        valid_str = format_timespan(valid_buf, FORMAT_TIMESPAN_MAX,
                                                    address->cinfo.ifa_valid * USEC_PER_SEC,
                                                    USEC_PER_SEC);
        }

        LIST_FOREACH(addresses, ad, link->addresses) {
                if (address_equal(ad, address)) {
                        LIST_REMOVE(addresses, link->addresses, ad);

                        address_free(ad);

                        address_dropped = true;

                        break;
                }
        }

        switch (type) {
        case RTM_NEWADDR:
                if (!address_dropped)
                        log_debug_link(link, "added address: %s/%u (valid for %s)",
                                       buf, address->prefixlen,
                                       strna(valid_str));
                else
                        log_debug_link(link, "updated address: %s/%u (valid for %s)",
                                       buf, address->prefixlen,
                                       strna(valid_str));

                LIST_PREPEND(addresses, link->addresses, address);
                address = NULL;

                link_save(link);

                break;
        case RTM_DELADDR:
                if (address_dropped) {
                        log_debug_link(link, "removed address: %s/%u (valid for %s)",
                                       buf, address->prefixlen,
                                       strna(valid_str));

                        link_save(link);
                } else
                        log_warning_link(link,
                                         "removing non-existent address: %s/%u (valid for %s)",
                                         buf, address->prefixlen,
                                         strna(valid_str));

                break;
        default:
                assert_not_reached("Received invalid RTNL message type");
        }

        return 1;
}

int link_add(Manager *m, sd_rtnl_message *message, Link **ret) {
        Link *link;
        _cleanup_rtnl_message_unref_ sd_rtnl_message *req = NULL;
        _cleanup_udev_device_unref_ struct udev_device *device = NULL;
        char ifindex_str[2 + DECIMAL_STR_MAX(int)];
        int r;

        assert(m);
        assert(m->rtnl);
        assert(message);
        assert(ret);

        r = link_new(m, message, ret);
        if (r < 0)
                return r;

        link = *ret;

        log_debug_link(link, "link %d added", link->ifindex);

        r = sd_rtnl_message_new_addr(m->rtnl, &req, RTM_GETADDR, link->ifindex,
                                     0);
        if (r < 0)
                return r;

        r = sd_rtnl_call_async(m->rtnl, req, link_get_address_handler, link, 0,
                               NULL);
        if (r < 0)
                return r;

        link_ref(link);

        if (detect_container(NULL) <= 0) {
                /* not in a container, udev will be around */
                sprintf(ifindex_str, "n%d", link->ifindex);
                device = udev_device_new_from_device_id(m->udev, ifindex_str);
                if (!device) {
                        log_warning_link(link,
                                         "could not find udev device: %m");
                        return -errno;
                }

                if (udev_device_get_is_initialized(device) <= 0) {
                        /* not yet ready */
                        log_debug_link(link, "link pending udev initialization...");
                        return 0;
                }

                r = link_initialized(link, device);
                if (r < 0)
                        return r;
        } else {
                /* we are calling a callback directly, so must take a ref */
                link_ref(link);

                r = link_initialized_and_synced(m->rtnl, NULL, link);
                if (r < 0)
                        return r;
        }

        return 0;
}

int link_update(Link *link, sd_rtnl_message *m) {
        struct ether_addr mac;
        const char *ifname;
        uint32_t mtu;
        int r;

        assert(link);
        assert(link->ifname);
        assert(m);

        if (link->state == LINK_STATE_LINGER) {
                link_ref(link);
                log_info_link(link, "link readded");
                link->state = LINK_STATE_ENSLAVING;
        }

        r = sd_rtnl_message_read_string(m, IFLA_IFNAME, &ifname);
        if (r >= 0 && !streq(ifname, link->ifname)) {
                log_info_link(link, "renamed to %s", ifname);

                free(link->ifname);
                link->ifname = strdup(ifname);
                if (!link->ifname)
                        return -ENOMEM;
        }

        r = sd_rtnl_message_read_u32(m, IFLA_MTU, &mtu);
        if (r >= 0 && mtu > 0) {
                link->mtu = mtu;
                if (!link->original_mtu) {
                        link->original_mtu = mtu;
                        log_debug_link(link, "saved original MTU: %"
                                       PRIu32, link->original_mtu);
                }

                if (link->dhcp_client) {
                        r = sd_dhcp_client_set_mtu(link->dhcp_client,
                                                   link->mtu);
                        if (r < 0) {
                                log_warning_link(link,
                                                 "Could not update MTU in DHCP client: %s",
                                                 strerror(-r));
                                return r;
                        }
                }
        }

        /* The kernel may broadcast NEWLINK messages without the MAC address
           set, simply ignore them. */
        r = sd_rtnl_message_read_ether_addr(m, IFLA_ADDRESS, &mac);
        if (r >= 0) {
                if (memcmp(link->mac.ether_addr_octet, mac.ether_addr_octet,
                           ETH_ALEN)) {

                        memcpy(link->mac.ether_addr_octet, mac.ether_addr_octet,
                               ETH_ALEN);

                        log_debug_link(link, "MAC address: "
                                       "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                                       mac.ether_addr_octet[0],
                                       mac.ether_addr_octet[1],
                                       mac.ether_addr_octet[2],
                                       mac.ether_addr_octet[3],
                                       mac.ether_addr_octet[4],
                                       mac.ether_addr_octet[5]);

                        if (link->ipv4ll) {
                                r = sd_ipv4ll_set_mac(link->ipv4ll, &link->mac);
                                if (r < 0) {
                                        log_warning_link(link,
                                                         "Could not update MAC address in IPv4LL client: %s",
                                                         strerror(-r));
                                        return r;
                                }
                        }

                        if (link->dhcp_client) {
                                r = sd_dhcp_client_set_mac(link->dhcp_client,
                                                           &link->mac);
                                if (r < 0) {
                                        log_warning_link(link,
                                                         "Could not update MAC address in DHCP client: %s",
                                                         strerror(-r));
                                        return r;
                                }
                        }

                        if (link->dhcp6_client) {
                                r = sd_dhcp6_client_set_mac(link->dhcp6_client,
                                                            &link->mac);
                                if (r < 0) {
                                        log_warning_link(link,
                                                         "Could not update MAC address in DHCPv6 client: %s",
                                                         strerror(-r));
                                        return r;
                                }
                        }
                }
        }

        return link_update_flags(link, m);
}

static void link_update_operstate(Link *link) {

        assert(link);

        if (link->kernel_operstate == IF_OPER_DORMANT)
                link->operstate = LINK_OPERSTATE_DORMANT;
        else if (link_has_carrier(link->flags, link->kernel_operstate)) {
                Address *address;
                uint8_t scope = RT_SCOPE_NOWHERE;

                /* if we have carrier, check what addresses we have */
                LIST_FOREACH(addresses, address, link->addresses) {
                        if (address->flags & (IFA_F_TENTATIVE | IFA_F_DEPRECATED))
                                continue;

                        if (address->scope < scope)
                                scope = address->scope;
                }

                if (scope < RT_SCOPE_SITE)
                        /* universally accessible addresses found */
                        link->operstate = LINK_OPERSTATE_ROUTABLE;
                else if (scope < RT_SCOPE_HOST)
                        /* only link or site local addresses found */
                        link->operstate = LINK_OPERSTATE_DEGRADED;
                else
                        /* no useful addresses found */
                        link->operstate = LINK_OPERSTATE_CARRIER;
        } else if (link->flags & IFF_UP)
                link->operstate = LINK_OPERSTATE_NO_CARRIER;
        else
                link->operstate = LINK_OPERSTATE_OFF;
}

int link_save(Link *link) {
        _cleanup_free_ char *temp_path = NULL;
        _cleanup_fclose_ FILE *f = NULL;
        const char *admin_state, *oper_state;
        int r;

        assert(link);
        assert(link->state_file);
        assert(link->lease_file);
        assert(link->manager);

        link_update_operstate(link);

        r = manager_save(link->manager);
        if (r < 0)
                return r;

        if (link->state == LINK_STATE_LINGER) {
                unlink(link->state_file);
                return 0;
        }

        admin_state = link_state_to_string(link->state);
        assert(admin_state);

        oper_state = link_operstate_to_string(link->operstate);
        assert(oper_state);

        r = fopen_temporary(link->state_file, &f, &temp_path);
        if (r < 0)
                return r;

        fchmod(fileno(f), 0644);

        fprintf(f,
                "# This is private data. Do not parse.\n"
                "ADMIN_STATE=%s\n"
                "OPER_STATE=%s\n",
                admin_state, oper_state);

        if (link->network) {
                char **address, **domain;
                bool space;

                fputs("DNS=", f);
                space = false;
                STRV_FOREACH(address, link->network->dns) {
                        if (space)
                                fputc(' ', f);
                        fputs(*address, f);
                        space = true;
                }

                if (link->network->dhcp_dns &&
                    link->dhcp_lease) {
                        const struct in_addr *addresses;

                        r = sd_dhcp_lease_get_dns(link->dhcp_lease, &addresses);
                        if (r > 0) {
                                if (space)
                                        fputc(' ', f);
                                serialize_in_addrs(f, addresses, r);
                        }
                }

                fputs("\n", f);

                fprintf(f, "NTP=");
                space = false;
                STRV_FOREACH(address, link->network->ntp) {
                        if (space)
                                fputc(' ', f);
                        fputs(*address, f);
                        space = true;
                }

                if (link->network->dhcp_ntp &&
                    link->dhcp_lease) {
                        const struct in_addr *addresses;

                        r = sd_dhcp_lease_get_ntp(link->dhcp_lease, &addresses);
                        if (r > 0) {
                                if (space)
                                        fputc(' ', f);
                                serialize_in_addrs(f, addresses, r);
                        }
                }

                fputs("\n", f);

                fprintf(f, "DOMAINS=");
                space = false;
                STRV_FOREACH(domain, link->network->domains) {
                        if (space)
                                fputc(' ', f);
                        fputs(*domain, f);
                        space = true;
                }

                if (link->network->dhcp_domains &&
                    link->dhcp_lease) {
                        const char *domainname;

                        r = sd_dhcp_lease_get_domainname(link->dhcp_lease, &domainname);
                        if (r >= 0) {
                                if (space)
                                        fputc(' ', f);
                                fputs(domainname, f);
                        }
                }

                fputs("\n", f);

                fprintf(f, "WILDCARD_DOMAIN=%s\n",
                        yes_no(link->network->wildcard_domain));

                fprintf(f, "LLMNR=%s\n",
                        llmnr_support_to_string(link->network->llmnr));
        }

        if (link->dhcp_lease) {
                assert(link->network);

                r = dhcp_lease_save(link->dhcp_lease, link->lease_file);
                if (r < 0)
                        goto fail;

                fprintf(f,
                        "DHCP_LEASE=%s\n",
                        link->lease_file);
        } else
                unlink(link->lease_file);

        r = fflush_and_check(f);
        if (r < 0)
                goto fail;

        if (rename(temp_path, link->state_file) < 0) {
                r = -errno;
                goto fail;
        }

        return 0;
fail:
        log_error_link(link, "Failed to save link data to %s: %s", link->state_file, strerror(-r));
        unlink(link->state_file);
        unlink(temp_path);
        return r;
}

static const char* const link_state_table[_LINK_STATE_MAX] = {
        [LINK_STATE_PENDING] = "pending",
        [LINK_STATE_ENSLAVING] = "configuring",
        [LINK_STATE_SETTING_ADDRESSES] = "configuring",
        [LINK_STATE_SETTING_ROUTES] = "configuring",
        [LINK_STATE_CONFIGURED] = "configured",
        [LINK_STATE_UNMANAGED] = "unmanaged",
        [LINK_STATE_FAILED] = "failed",
        [LINK_STATE_LINGER] = "linger",
};

DEFINE_STRING_TABLE_LOOKUP(link_state, LinkState);

static const char* const link_operstate_table[_LINK_OPERSTATE_MAX] = {
        [LINK_OPERSTATE_OFF] = "off",
        [LINK_OPERSTATE_NO_CARRIER] = "no-carrier",
        [LINK_OPERSTATE_DORMANT] = "dormant",
        [LINK_OPERSTATE_CARRIER] = "carrier",
        [LINK_OPERSTATE_DEGRADED] = "degraded",
        [LINK_OPERSTATE_ROUTABLE] = "routable",
};

DEFINE_STRING_TABLE_LOOKUP(link_operstate, LinkOperationalState);
