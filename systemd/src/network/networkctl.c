/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2014 Lennart Poettering

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

#include <stdbool.h>
#include <getopt.h>

#include "sd-network.h"
#include "sd-rtnl.h"
#include "libudev.h"

#include "build.h"
#include "util.h"
#include "pager.h"
#include "rtnl-util.h"
#include "udev-util.h"
#include "arphrd-list.h"
#include "local-addresses.h"
#include "socket-util.h"

static bool arg_no_pager = false;
static bool arg_legend = true;
static bool arg_all = false;

static void pager_open_if_enabled(void) {

        if (arg_no_pager)
                return;

        pager_open(false);
}

static int link_get_type_string(int iftype, struct udev_device *d, char **ret) {
        const char *t;
        char *p;

        if (iftype == ARPHRD_ETHER && d) {
                const char *devtype, *id = NULL;
                /* WLANs have iftype ARPHRD_ETHER, but we want
                 * to show a more useful type string for
                 * them */

                devtype = udev_device_get_devtype(d);
                if (streq_ptr(devtype, "wlan"))
                        id = "wlan";
                else if (streq_ptr(devtype, "wwan"))
                        id = "wwan";

                if (id) {
                        p = strdup(id);
                        if (!p)
                                return -ENOMEM;

                        *ret = p;
                        return 1;
                }
        }

        t = arphrd_to_name(iftype);
        if (!t) {
                *ret = NULL;
                return 0;
        }

        p = strdup(t);
        if (!p)
                return -ENOMEM;

        ascii_strlower(p);
        *ret = p;

        return 0;
}

typedef struct LinkInfo {
        const char *name;
        int ifindex;
        unsigned iftype;
} LinkInfo;

static int link_info_compare(const void *a, const void *b) {
        const LinkInfo *x = a, *y = b;

        return x->ifindex - y->ifindex;
}

static int decode_and_sort_links(sd_rtnl_message *m, LinkInfo **ret) {
        _cleanup_free_ LinkInfo *links = NULL;
        size_t size = 0, c = 0;
        sd_rtnl_message *i;
        int r;

        for (i = m; i; i = sd_rtnl_message_next(i)) {
                const char *name;
                unsigned iftype;
                uint16_t type;
                int ifindex;

                r = sd_rtnl_message_get_type(i, &type);
                if (r < 0)
                        return r;

                if (type != RTM_NEWLINK)
                        continue;

                r = sd_rtnl_message_link_get_ifindex(i, &ifindex);
                if (r < 0)
                        return r;

                r = sd_rtnl_message_read_string(i, IFLA_IFNAME, &name);
                if (r < 0)
                        return r;

                r = sd_rtnl_message_link_get_type(i, &iftype);
                if (r < 0)
                        return r;

                if (!GREEDY_REALLOC(links, size, c+1))
                        return -ENOMEM;

                links[c].name = name;
                links[c].ifindex = ifindex;
                links[c].iftype = iftype;
                c++;
        }

        qsort_safe(links, c, sizeof(LinkInfo), link_info_compare);

        *ret = links;
        links = NULL;

        return (int) c;
}

static void operational_state_to_color(const char *state, const char **on, const char **off) {
        assert(on);
        assert(off);

        if (streq_ptr(state, "routable")) {
                *on = ansi_highlight_green();
                *off = ansi_highlight_off();
        } else if (streq_ptr(state, "degraded")) {
                *on = ansi_highlight_yellow();
                *off = ansi_highlight_off();
        } else
                *on = *off = "";
}

static void setup_state_to_color(const char *state, const char **on, const char **off) {
        assert(on);
        assert(off);

        if (streq_ptr(state, "configured")) {
                *on = ansi_highlight_green();
                *off = ansi_highlight_off();
        } else if (streq_ptr(state, "configuring")) {
                *on = ansi_highlight_yellow();
                *off = ansi_highlight_off();
        } else if (streq_ptr(state, "failed") || streq_ptr(state, "linger")) {
                *on = ansi_highlight_red();
                *off = ansi_highlight_off();
        } else
                *on = *off = "";
}

static int list_links(char **args, unsigned n) {
        _cleanup_rtnl_message_unref_ sd_rtnl_message *req = NULL, *reply = NULL;
        _cleanup_udev_unref_ struct udev *udev = NULL;
        _cleanup_rtnl_unref_ sd_rtnl *rtnl = NULL;
        _cleanup_free_ LinkInfo *links = NULL;
        int r, c, i;

        pager_open_if_enabled();

        r = sd_rtnl_open(&rtnl, 0);
        if (r < 0) {
                log_error("Failed to connect to netlink: %s", strerror(-r));
                return r;
        }

        udev = udev_new();
        if (!udev) {
                log_error("Failed to connect to udev: %m");
                return -errno;
        }

        r = sd_rtnl_message_new_link(rtnl, &req, RTM_GETLINK, 0);
        if (r < 0)
                return rtnl_log_create_error(r);

        r = sd_rtnl_message_request_dump(req, true);
        if (r < 0)
                return rtnl_log_create_error(r);

        r = sd_rtnl_call(rtnl, req, 0, &reply);
        if (r < 0) {
                log_error("Failed to enumerate links: %s", strerror(-r));
                return r;
        }

        if (arg_legend)
                printf("%3s %-16s %-18s %-11s %-10s\n", "IDX", "LINK", "TYPE", "OPERATIONAL", "SETUP");

        c = decode_and_sort_links(reply, &links);
        if (c < 0)
                return rtnl_log_parse_error(c);

        for (i = 0; i < c; i++) {
                _cleanup_free_ char *setup_state = NULL, *operational_state = NULL;
                _cleanup_udev_device_unref_ struct udev_device *d = NULL;
                const char *on_color_operational, *off_color_operational,
                           *on_color_setup, *off_color_setup;
                 char devid[2 + DECIMAL_STR_MAX(int)];
                _cleanup_free_ char *t = NULL;

                sd_network_link_get_operational_state(links[i].ifindex, &operational_state);
                operational_state_to_color(operational_state, &on_color_operational, &off_color_operational);

                sd_network_link_get_setup_state(links[i].ifindex, &setup_state);
                setup_state_to_color(setup_state, &on_color_setup, &off_color_setup);

                sprintf(devid, "n%i", links[i].ifindex);
                d = udev_device_new_from_device_id(udev, devid);

                link_get_type_string(links[i].iftype, d, &t);

                printf("%3i %-16s %-18s %s%-11s%s %s%-10s%s\n",
                       links[i].ifindex, links[i].name, strna(t),
                       on_color_operational, strna(operational_state), off_color_operational,
                       on_color_setup, strna(setup_state), off_color_setup);
        }

        if (arg_legend)
                printf("\n%i links listed.\n", c);

        return 0;
}

static int dump_addresses(sd_rtnl *rtnl, const char *prefix, int ifindex) {
        _cleanup_free_ struct local_address *local = NULL;
        int r, n, i;

        n = local_addresses(rtnl, ifindex, &local);
        if (n < 0)
                return n;

        for (i = 0; i < n; i++) {
                _cleanup_free_ char *pretty = NULL;

                r = in_addr_to_string(local[i].family, &local[i].address, &pretty);
                if (r < 0)
                        return r;

                printf("%*s%s\n",
                       (int) strlen(prefix),
                       i == 0 ? prefix : "",
                       pretty);
        }

        return 0;
}

static void dump_list(const char *prefix, char **l) {
        char **i;

        STRV_FOREACH(i, l) {
                printf("%*s%s\n",
                       (int) strlen(prefix),
                       i == l ? prefix : "",
                       *i);
        }
}

static int link_status_one(sd_rtnl *rtnl, struct udev *udev, const char *name) {
        _cleanup_strv_free_ char **dns = NULL, **ntp = NULL, **domains = NULL;
        _cleanup_free_ char *setup_state = NULL, *operational_state = NULL;
        _cleanup_rtnl_message_unref_ sd_rtnl_message *req = NULL, *reply = NULL;
        _cleanup_udev_device_unref_ struct udev_device *d = NULL;
        char devid[2 + DECIMAL_STR_MAX(int)];
        _cleanup_free_ char *t = NULL;
        const char *driver = NULL, *path = NULL, *vendor = NULL, *model = NULL;
        const char *on_color_operational, *off_color_operational,
                   *on_color_setup, *off_color_setup;
        struct ether_addr e;
        unsigned iftype;
        int r, ifindex;
        bool have_mac;
        uint32_t mtu;

        assert(rtnl);
        assert(udev);
        assert(name);

        if (safe_atoi(name, &ifindex) >= 0 && ifindex > 0)
                r = sd_rtnl_message_new_link(rtnl, &req, RTM_GETLINK, ifindex);
        else {
                r = sd_rtnl_message_new_link(rtnl, &req, RTM_GETLINK, 0);
                if (r < 0)
                        return rtnl_log_create_error(r);

                r = sd_rtnl_message_append_string(req, IFLA_IFNAME, name);
        }

        if (r < 0)
                return rtnl_log_create_error(r);

        r = sd_rtnl_call(rtnl, req, 0, &reply);
        if (r < 0) {
                log_error("Failed to query link: %s", strerror(-r));
                return r;
        }

        r = sd_rtnl_message_link_get_ifindex(reply, &ifindex);
        if (r < 0)
                return rtnl_log_parse_error(r);

        r = sd_rtnl_message_read_string(reply, IFLA_IFNAME, &name);
        if (r < 0)
                return rtnl_log_parse_error(r);

        r = sd_rtnl_message_link_get_type(reply, &iftype);
        if (r < 0)
                return rtnl_log_parse_error(r);

        have_mac = sd_rtnl_message_read_ether_addr(reply, IFLA_ADDRESS, &e) >= 0;

        if (have_mac) {
                const uint8_t *p;
                bool all_zeroes = true;

                for (p = (uint8_t*) &e; p < (uint8_t*) &e + sizeof(e); p++)
                        if (*p != 0) {
                                all_zeroes = false;
                                break;
                        }

                if (all_zeroes)
                        have_mac = false;
        }

        sd_rtnl_message_read_u32(reply, IFLA_MTU, &mtu);

        sd_network_link_get_operational_state(ifindex, &operational_state);
        operational_state_to_color(operational_state, &on_color_operational, &off_color_operational);

        sd_network_link_get_setup_state(ifindex, &setup_state);
        setup_state_to_color(setup_state, &on_color_setup, &off_color_setup);

        sd_network_link_get_dns(ifindex, &dns);
        sd_network_link_get_ntp(ifindex, &ntp);
        sd_network_link_get_domains(ifindex, &domains);
        r = sd_network_link_get_wildcard_domain(ifindex);
        if (r > 0) {
                char *wildcard;

                wildcard = strdup("*");
                if (!wildcard)
                        return log_oom();

                if (strv_consume(&domains, wildcard) < 0)
                        return log_oom();
        }

        sprintf(devid, "n%i", ifindex);
        d = udev_device_new_from_device_id(udev, devid);

        link_get_type_string(iftype, d, &t);

        if (d) {
                driver = udev_device_get_property_value(d, "ID_NET_DRIVER");
                path = udev_device_get_property_value(d, "ID_PATH");

                vendor = udev_device_get_property_value(d, "ID_VENDOR_FROM_DATABASE");
                if (!vendor)
                        vendor = udev_device_get_property_value(d, "ID_VENDOR");

                model = udev_device_get_property_value(d, "ID_MODEL_FROM_DATABASE");
                if (!model)
                        model = udev_device_get_property_value(d, "ID_MODEL");
        }


        printf("%s%s%s %i: %s\n", on_color_operational, draw_special_char(DRAW_BLACK_CIRCLE), off_color_operational, ifindex, name);

        printf("        Type: %s\n"
               "       State: %s%s%s (%s%s%s)\n",
               strna(t),
               on_color_operational, strna(operational_state), off_color_operational,
               on_color_setup, strna(setup_state), off_color_setup);

        if (path)
                printf("        Path: %s\n", path);
        if (driver)
                printf("      Driver: %s\n", driver);
        if (vendor)
                printf("      Vendor: %s\n", vendor);
        if (model)
                printf("       Model: %s\n", model);

        if (have_mac) {
                char ea[ETHER_ADDR_TO_STRING_MAX];
                printf("  HW Address: %s\n", ether_addr_to_string(&e, ea));
        }

        if (mtu > 0)
                printf("         MTU: %u\n", mtu);

        dump_addresses(rtnl, "     Address: ", ifindex);

        if (!strv_isempty(dns))
                dump_list("         DNS: ", dns);
        if (!strv_isempty(domains))
                dump_list("      Domain: ", domains);
        if (!strv_isempty(ntp))
                dump_list("         NTP: ", ntp);

        return 0;
}

static int link_status(char **args, unsigned n) {
        _cleanup_udev_unref_ struct udev *udev = NULL;
        _cleanup_rtnl_unref_ sd_rtnl *rtnl = NULL;
        char **name;
        int r;

        r = sd_rtnl_open(&rtnl, 0);
        if (r < 0) {
                log_error("Failed to connect to netlink: %s", strerror(-r));
                return r;
        }

        udev = udev_new();
        if (!udev) {
                log_error("Failed to connect to udev: %m");
                return -errno;
        }

        if (n <= 1 && !arg_all) {
                _cleanup_free_ char *operational_state = NULL;
                _cleanup_strv_free_ char **dns = NULL, **ntp = NULL, **domains = NULL;
                _cleanup_free_ struct local_address *addresses = NULL;
                const char *on_color_operational, *off_color_operational;
                int i, c;

                sd_network_get_operational_state(&operational_state);
                operational_state_to_color(operational_state, &on_color_operational, &off_color_operational);

                printf("       State: %s%s%s\n", on_color_operational, strna(operational_state), off_color_operational);

                c = local_addresses(rtnl, 0, &addresses);
                for (i = 0; i < c; i++) {
                        _cleanup_free_ char *pretty = NULL;

                        r = in_addr_to_string(addresses[i].family, &addresses[i].address, &pretty);
                        if (r < 0)
                                return log_oom();

                        printf("%13s %s\n",
                               i > 0 ? "" : "Address:", pretty);
                }

                sd_network_get_dns(&dns);
                if (!strv_isempty(dns))
                        dump_list("         DNS: ", dns);

                sd_network_get_domains(&domains);
                if (!strv_isempty(domains))
                        dump_list("      Domain: ", domains);

                sd_network_get_dns(&ntp);
                if (!strv_isempty(ntp))
                        dump_list("         NTP: ", ntp);

                return 0;
        }

        pager_open_if_enabled();

        if (arg_all) {
                _cleanup_rtnl_message_unref_ sd_rtnl_message *req = NULL, *reply = NULL;
                _cleanup_free_ LinkInfo *links = NULL;
                int c, i;

                r = sd_rtnl_message_new_link(rtnl, &req, RTM_GETLINK, 0);
                if (r < 0)
                        return rtnl_log_create_error(r);

                r = sd_rtnl_message_request_dump(req, true);
                if (r < 0)
                        return rtnl_log_create_error(r);

                r = sd_rtnl_call(rtnl, req, 0, &reply);
                if (r < 0) {
                        log_error("Failed to enumerate links: %s", strerror(-r));
                        return r;
                }

                c = decode_and_sort_links(reply, &links);
                if (c < 0)
                        return rtnl_log_parse_error(c);

                for (i = 0; i < c; i++) {
                        if (i > 0)
                                fputc('\n', stdout);

                        link_status_one(rtnl, udev, links[i].name);
                }
        }

        STRV_FOREACH(name, args + 1) {
                if (name != args+1)
                        fputc('\n', stdout);

                link_status_one(rtnl, udev, *name);
        }

        return 0;
}

static void help(void) {
        printf("%s [OPTIONS...]\n\n"
               "Query and control the networking subsystem.\n\n"
               "  -h --help             Show this help\n"
               "     --version          Show package version\n"
               "     --no-pager         Do not pipe output into a pager\n"
               "     --no-legend        Do not show the headers and footers\n"
               "  -a --all              Show status for all links\n\n"
               "Commands:\n"
               "  list                  List links\n"
               "  status LINK           Show link status\n"
               , program_invocation_short_name);
}

static int parse_argv(int argc, char *argv[]) {

        enum {
                ARG_VERSION = 0x100,
                ARG_NO_PAGER,
                ARG_NO_LEGEND,
        };

        static const struct option options[] = {
                { "help",      no_argument,       NULL, 'h'           },
                { "version",   no_argument,       NULL, ARG_VERSION   },
                { "no-pager",  no_argument,       NULL, ARG_NO_PAGER  },
                { "no-legend", no_argument,       NULL, ARG_NO_LEGEND },
                { "all",       no_argument,       NULL, 'a'           },
                {}
        };

        int c;

        assert(argc >= 0);
        assert(argv);

        while ((c = getopt_long(argc, argv, "ha", options, NULL)) >= 0) {

                switch (c) {

                case 'h':
                        help();
                        return 0;

                case ARG_VERSION:
                        puts(PACKAGE_STRING);
                        puts(SYSTEMD_FEATURES);
                        return 0;

                case ARG_NO_PAGER:
                        arg_no_pager = true;
                        break;

                case ARG_NO_LEGEND:
                        arg_legend = false;
                        break;

                case 'a':
                        arg_all = true;
                        break;

                case '?':
                        return -EINVAL;

                default:
                        assert_not_reached("Unhandled option");
                }
        }

        return 1;
}

static int networkctl_main(int argc, char *argv[]) {

        static const struct {
                const char* verb;
                const enum {
                        MORE,
                        LESS,
                        EQUAL
                } argc_cmp;
                const int argc;
                int (* const dispatch)(char **args, unsigned n);
        } verbs[] = {
                { "list",   LESS, 1, list_links  },
                { "status", MORE, 1, link_status },
        };

        int left;
        unsigned i;

        assert(argc >= 0);
        assert(argv);

        left = argc - optind;

        if (left <= 0)
                /* Special rule: no arguments means "list" */
                i = 0;
        else {
                if (streq(argv[optind], "help")) {
                        help();
                        return 0;
                }

                for (i = 0; i < ELEMENTSOF(verbs); i++)
                        if (streq(argv[optind], verbs[i].verb))
                                break;

                if (i >= ELEMENTSOF(verbs)) {
                        log_error("Unknown operation %s", argv[optind]);
                        return -EINVAL;
                }
        }

        switch (verbs[i].argc_cmp) {

        case EQUAL:
                if (left != verbs[i].argc) {
                        log_error("Invalid number of arguments.");
                        return -EINVAL;
                }

                break;

        case MORE:
                if (left < verbs[i].argc) {
                        log_error("Too few arguments.");
                        return -EINVAL;
                }

                break;

        case LESS:
                if (left > verbs[i].argc) {
                        log_error("Too many arguments.");
                        return -EINVAL;
                }

                break;

        default:
                assert_not_reached("Unknown comparison operator.");
        }

        return verbs[i].dispatch(argv + optind, left);
}

int main(int argc, char* argv[]) {
        int r;

        log_parse_environment();
        log_open();

        r = parse_argv(argc, argv);
        if (r <= 0)
                goto finish;

        r = networkctl_main(argc, argv);

finish:
        pager_close();

        return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
