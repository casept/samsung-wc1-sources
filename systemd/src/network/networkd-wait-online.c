
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

#include <getopt.h>

#include "sd-daemon.h"

#include "networkd-wait-online.h"

#include "strv.h"
#include "build.h"

static bool arg_quiet = false;
static char **arg_interfaces = NULL;

static void help(void) {
        printf("%s [OPTIONS...]\n\n"
               "Block until network is configured.\n\n"
               "  -h --help                 Show this help\n"
               "     --version              Print version string\n"
               "  -q --quiet                Do not show status information\n"
               "  -i --interface=INTERFACE  Block until at least these interfaces have appeared\n"
               , program_invocation_short_name);
}

static int parse_argv(int argc, char *argv[]) {

        enum {
                ARG_VERSION = 0x100,
        };

        static const struct option options[] = {
                { "help",            no_argument,       NULL, 'h'         },
                { "version",         no_argument,       NULL, ARG_VERSION },
                { "quiet",           no_argument,       NULL, 'q'         },
                { "interface",       required_argument, NULL, 'i'         },
                {}
        };

        int c;

        assert(argc >= 0);
        assert(argv);

        while ((c = getopt_long(argc, argv, "+hq", options, NULL)) >= 0)

                switch (c) {

                case 'h':
                        help();
                        return 0;

                case 'q':
                        arg_quiet = true;
                        break;

                case ARG_VERSION:
                        puts(PACKAGE_STRING);
                        puts(SYSTEMD_FEATURES);
                        return 0;

                case 'i':
                        if (strv_extend(&arg_interfaces, optarg) < 0)
                                return log_oom();

                        break;

                case '?':
                        return -EINVAL;

                default:
                        assert_not_reached("Unhandled option");
                }

        return 1;
}

int main(int argc, char *argv[]) {
        _cleanup_(manager_freep) Manager *m = NULL;
        int r;

        log_set_target(LOG_TARGET_AUTO);
        log_parse_environment();
        log_open();

        umask(0022);

        r = parse_argv(argc, argv);
        if (r <= 0)
                return r;

        if (arg_quiet)
                log_set_max_level(LOG_WARNING);

        assert_se(sigprocmask_many(SIG_BLOCK, SIGTERM, SIGINT, -1) == 0);

        r = manager_new(&m, arg_interfaces);
        if (r < 0) {
                log_error("Could not create manager: %s", strerror(-r));
                goto finish;
        }

        if (manager_all_configured(m)) {
                r = 0;
                goto finish;
        }

        sd_notify(false,
                  "READY=1\n"
                  "STATUS=Waiting for network connections...");

        r = sd_event_loop(m->event);
        if (r < 0) {
                log_error("Event loop failed: %s", strerror(-r));
                goto finish;
        }

finish:
        sd_notify(false, "STATUS=All interfaces configured...");

        return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
