/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering

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

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <pwd.h>
#include <locale.h>

#include "sd-bus.h"
#include "bus-util.h"
#include "bus-error.h"
#include "log.h"
#include "util.h"
#include "macro.h"
#include "pager.h"
#include "build.h"
#include "strv.h"
#include "unit-name.h"
#include "sysfs-show.h"
#include "cgroup-show.h"
#include "cgroup-util.h"
#include "spawn-polkit-agent.h"

static char **arg_property = NULL;
static bool arg_all = false;
static bool arg_full = false;
static bool arg_no_pager = false;
static bool arg_legend = true;
static const char *arg_kill_who = NULL;
static int arg_signal = SIGTERM;
static BusTransport arg_transport = BUS_TRANSPORT_LOCAL;
static bool arg_ask_password = true;
static char *arg_host = NULL;

static void pager_open_if_enabled(void) {

        if (arg_no_pager)
                return;

        pager_open(false);
}

static void polkit_agent_open_if_enabled(void) {

        /* Open the polkit agent as a child process if necessary */

        if (!arg_ask_password)
                return;

        if (arg_transport != BUS_TRANSPORT_LOCAL)
                return;

        polkit_agent_open();
}

static int list_sessions(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_bus_message_unref_ sd_bus_message *reply = NULL;
        const char *id, *user, *seat, *object;
        unsigned k = 0;
        uint32_t uid;
        int r;

        pager_open_if_enabled();

        r = sd_bus_call_method(
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        "ListSessions",
                        &error, &reply,
                        "");
        if (r < 0) {
                log_error("Failed to list sessions: %s", bus_error_message(&error, r));
                return r;
        }

        r = sd_bus_message_enter_container(reply, 'a', "(susso)");
        if (r < 0)
                return bus_log_parse_error(r);

        if (arg_legend)
                printf("%10s %10s %-16s %-16s\n", "SESSION", "UID", "USER", "SEAT");

        while ((r = sd_bus_message_read(reply, "(susso)", &id, &uid, &user, &seat, &object)) > 0) {
                printf("%10s %10u %-16s %-16s\n", id, (unsigned) uid, user, seat);
                k++;
        }
        if (r < 0)
                return bus_log_parse_error(r);

        if (arg_legend)
                printf("\n%u sessions listed.\n", k);

        return 0;
}

static int list_users(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_bus_message_unref_ sd_bus_message *reply = NULL;
        const char *user, *object;
        unsigned k = 0;
        uint32_t uid;
        int r;

        pager_open_if_enabled();

        r = sd_bus_call_method(
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        "ListUsers",
                        &error, &reply,
                        "");
        if (r < 0) {
                log_error("Failed to list users: %s", bus_error_message(&error, r));
                return r;
        }

        r = sd_bus_message_enter_container(reply, 'a', "(uso)");
        if (r < 0)
                return bus_log_parse_error(r);

        if (arg_legend)
                printf("%10s %-16s\n", "UID", "USER");

        while ((r = sd_bus_message_read(reply, "(uso)", &uid, &user, &object)) > 0) {
                printf("%10u %-16s\n", (unsigned) uid, user);
                k++;
        }
        if (r < 0)
                return bus_log_parse_error(r);

        if (arg_legend)
                printf("\n%u users listed.\n", k);

        return 0;
}

static int list_seats(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_bus_message_unref_ sd_bus_message *reply = NULL;
        const char *seat, *object;
        unsigned k = 0;
        int r;

        pager_open_if_enabled();

        r = sd_bus_call_method(
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        "ListSeats",
                        &error, &reply,
                        "");
        if (r < 0) {
                log_error("Failed to list seats: %s", bus_error_message(&error, r));
                return r;
        }

        r = sd_bus_message_enter_container(reply, 'a', "(so)");
        if (r < 0)
                return bus_log_parse_error(r);

        if (arg_legend)
                printf("%-16s\n", "SEAT");

        while ((r = sd_bus_message_read(reply, "(so)", &seat, &object)) > 0) {
                printf("%-16s\n", seat);
                k++;
        }
        if (r < 0)
                return bus_log_parse_error(r);

        if (arg_legend)
                printf("\n%u seats listed.\n", k);

        return 0;
}

static int show_unit_cgroup(sd_bus *bus, const char *interface, const char *unit, pid_t leader) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_bus_message_unref_ sd_bus_message *reply = NULL;
        _cleanup_free_ char *path = NULL;
        const char *cgroup;
        int r, output_flags;
        unsigned c;

        assert(bus);
        assert(unit);

        if (arg_transport != BUS_TRANSPORT_LOCAL)
                return 0;

        path = unit_dbus_path_from_name(unit);
        if (!path)
                return -ENOMEM;

        r = sd_bus_get_property(
                        bus,
                        "org.freedesktop.systemd1",
                        path,
                        interface,
                        "ControlGroup",
                        &error, &reply, "s");
        if (r < 0)
                return r;

        r = sd_bus_message_read(reply, "s", &cgroup);
        if (r < 0)
                return r;

        if (isempty(cgroup))
                return 0;

        if (cg_is_empty_recursive(SYSTEMD_CGROUP_CONTROLLER, cgroup, false) != 0 && leader <= 0)
                return 0;

        output_flags =
                arg_all * OUTPUT_SHOW_ALL |
                arg_full * OUTPUT_FULL_WIDTH;

        c = columns();
        if (c > 18)
                c -= 18;
        else
                c = 0;

        show_cgroup_and_extra(SYSTEMD_CGROUP_CONTROLLER, cgroup, "\t\t  ", c, false, &leader, leader > 0, output_flags);
        return 0;
}

typedef struct SessionStatusInfo {
        const char *id;
        uid_t uid;
        const char *name;
        usec_t timestamp;
        unsigned int vtnr;
        const char *seat;
        const char *tty;
        const char *display;
        bool remote;
        const char *remote_host;
        const char *remote_user;
        const char *service;
        pid_t leader;
        const char *type;
        const char *class;
        const char *state;
        const char *scope;
        const char *desktop;
} SessionStatusInfo;

typedef struct UserStatusInfo {
        uid_t uid;
        const char *name;
        usec_t timestamp;
        const char *state;
        char **sessions;
        const char *display;
        const char *slice;
} UserStatusInfo;

typedef struct SeatStatusInfo {
        const char *id;
        const char *active_session;
        char **sessions;
} SeatStatusInfo;

static int prop_map_first_of_struct(sd_bus *bus, const char *member, sd_bus_message *m, sd_bus_error *error, void *userdata) {
        const char *contents;
        int r;

        r = sd_bus_message_peek_type(m, NULL, &contents);
        if (r < 0)
                return r;

        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_STRUCT, contents);
        if (r < 0)
                return r;

        if (contents[0] == 's' || contents[0] == 'o') {
                const char *s;
                char **p = (char **) userdata;

                r = sd_bus_message_read_basic(m, contents[0], &s);
                if (r < 0)
                        return r;

                free(*p);
                *p = strdup(s);

                if (!*p)
                        return -ENOMEM;
        } else {
                r = sd_bus_message_read_basic(m, contents[0], userdata);
                if (r < 0)
                        return r;
        }

        r = sd_bus_message_skip(m, contents+1);
        if (r < 0)
                return r;

        r = sd_bus_message_exit_container(m);
        if (r < 0)
                return r;

        return 0;
}

static int prop_map_sessions_strv(sd_bus *bus, const char *member, sd_bus_message *m, sd_bus_error *error, void *userdata) {
        const char *name;
        int r;

        assert(bus);
        assert(m);

        r = sd_bus_message_enter_container(m, 'a', "(so)");
        if (r < 0)
                return r;

        while ((r = sd_bus_message_read(m, "(so)", &name, NULL)) > 0) {
                r = strv_extend(userdata, name);
                if (r < 0)
                        return r;
        }
        if (r < 0)
                return r;

        return sd_bus_message_exit_container(m);
}

static int print_session_status_info(sd_bus *bus, const char *path, bool *new_line) {

        static const struct bus_properties_map map[]  = {
                { "Id",         "s", NULL, offsetof(SessionStatusInfo, id) },
                { "Name",       "s", NULL, offsetof(SessionStatusInfo, name) },
                { "TTY",        "s", NULL, offsetof(SessionStatusInfo, tty) },
                { "Display",    "s", NULL, offsetof(SessionStatusInfo, display) },
                { "RemoteHost", "s", NULL, offsetof(SessionStatusInfo, remote_host) },
                { "RemoteUser", "s", NULL, offsetof(SessionStatusInfo, remote_user) },
                { "Service",    "s", NULL, offsetof(SessionStatusInfo, service) },
                { "Desktop",    "s", NULL, offsetof(SessionStatusInfo, desktop) },
                { "Type",       "s", NULL, offsetof(SessionStatusInfo, type) },
                { "Class",      "s", NULL, offsetof(SessionStatusInfo, class) },
                { "Scope",      "s", NULL, offsetof(SessionStatusInfo, scope) },
                { "State",      "s", NULL, offsetof(SessionStatusInfo, state) },
                { "VTNr",       "u", NULL, offsetof(SessionStatusInfo, vtnr) },
                { "Leader",     "u", NULL, offsetof(SessionStatusInfo, leader) },
                { "Remote",     "b", NULL, offsetof(SessionStatusInfo, remote) },
                { "Timestamp",  "t", NULL, offsetof(SessionStatusInfo, timestamp) },
                { "User",       "(uo)", prop_map_first_of_struct, offsetof(SessionStatusInfo, uid) },
                { "Seat",       "(so)", prop_map_first_of_struct, offsetof(SessionStatusInfo, seat) },
                {}
        };

        char since1[FORMAT_TIMESTAMP_RELATIVE_MAX], *s1;
        char since2[FORMAT_TIMESTAMP_MAX], *s2;
        SessionStatusInfo i = {};
        int r;

        r = bus_map_all_properties(bus, "org.freedesktop.login1", path, map, &i);
        if (r < 0) {
                log_error("Could not get properties: %s", strerror(-r));
                return r;
        }

        if (*new_line)
                printf("\n");

        *new_line = true;

        printf("%s - ", strna(i.id));

        if (i.name)
                printf("%s (%u)\n", i.name, (unsigned) i.uid);
        else
                printf("%u\n", (unsigned) i.uid);

        s1 = format_timestamp_relative(since1, sizeof(since1), i.timestamp);
        s2 = format_timestamp(since2, sizeof(since2), i.timestamp);

        if (s1)
                printf("\t   Since: %s; %s\n", s2, s1);
        else if (s2)
                printf("\t   Since: %s\n", s2);

        if (i.leader > 0) {
                _cleanup_free_ char *t = NULL;

                printf("\t  Leader: %u", (unsigned) i.leader);

                get_process_comm(i.leader, &t);
                if (t)
                        printf(" (%s)", t);

                printf("\n");
        }

        if (!isempty(i.seat)) {
                printf("\t    Seat: %s", i.seat);

                if (i.vtnr > 0)
                        printf("; vc%i", i.vtnr);

                printf("\n");
        }

        if (i.tty)
                printf("\t     TTY: %s\n", i.tty);
        else if (i.display)
                printf("\t Display: %s\n", i.display);

        if (i.remote_host && i.remote_user)
                printf("\t  Remote: %s@%s\n", i.remote_user, i.remote_host);
        else if (i.remote_host)
                printf("\t  Remote: %s\n", i.remote_host);
        else if (i.remote_user)
                printf("\t  Remote: user %s\n", i.remote_user);
        else if (i.remote)
                printf("\t  Remote: Yes\n");

        if (i.service) {
                printf("\t Service: %s", i.service);

                if (i.type)
                        printf("; type %s", i.type);

                if (i.class)
                        printf("; class %s", i.class);

                printf("\n");
        } else if (i.type) {
                printf("\t    Type: %s", i.type);

                if (i.class)
                        printf("; class %s", i.class);

                printf("\n");
        } else if (i.class)
                printf("\t   Class: %s\n", i.class);

        if (!isempty(i.desktop))
                printf("\t Desktop: %s\n", i.desktop);

        if (i.state)
                printf("\t   State: %s\n", i.state);

        if (i.scope) {
                printf("\t    Unit: %s\n", i.scope);
                show_unit_cgroup(bus, "org.freedesktop.systemd1.Scope", i.scope, i.leader);
        }

        return 0;
}

static int print_user_status_info(sd_bus *bus, const char *path, bool *new_line) {

        static const struct bus_properties_map map[]  = {
                { "Name",       "s",     NULL, offsetof(UserStatusInfo, name) },
                { "Slice",      "s",     NULL, offsetof(UserStatusInfo, slice) },
                { "State",      "s",     NULL, offsetof(UserStatusInfo, state) },
                { "UID",        "u",     NULL, offsetof(UserStatusInfo, uid) },
                { "Timestamp",  "t",     NULL, offsetof(UserStatusInfo, timestamp) },
                { "Display",    "(so)",  prop_map_first_of_struct, offsetof(UserStatusInfo, display) },
                { "Sessions",   "a(so)", prop_map_sessions_strv,   offsetof(UserStatusInfo, sessions) },
                {}
        };

        char since1[FORMAT_TIMESTAMP_RELATIVE_MAX], *s1;
        char since2[FORMAT_TIMESTAMP_MAX], *s2;
        UserStatusInfo i = {};
        int r;

        r = bus_map_all_properties(bus, "org.freedesktop.login1", path, map, &i);
        if (r < 0) {
                log_error("Could not get properties: %s", strerror(-r));
                goto finish;
        }

        if (*new_line)
                printf("\n");

        *new_line = true;

        if (i.name)
                printf("%s (%u)\n", i.name, (unsigned) i.uid);
        else
                printf("%u\n", (unsigned) i.uid);

        s1 = format_timestamp_relative(since1, sizeof(since1), i.timestamp);
        s2 = format_timestamp(since2, sizeof(since2), i.timestamp);

        if (s1)
                printf("\t   Since: %s; %s\n", s2, s1);
        else if (s2)
                printf("\t   Since: %s\n", s2);

        if (!isempty(i.state))
                printf("\t   State: %s\n", i.state);

        if (!strv_isempty(i.sessions)) {
                char **l;
                printf("\tSessions:");

                STRV_FOREACH(l, i.sessions) {
                        if (streq_ptr(*l, i.display))
                                printf(" *%s", *l);
                        else
                                printf(" %s", *l);
                }

                printf("\n");
        }

        if (i.slice) {
                printf("\t    Unit: %s\n", i.slice);
                show_unit_cgroup(bus, "org.freedesktop.systemd1.Slice", i.slice, 0);
        }

finish:
        strv_free(i.sessions);

        return r;
}

static int print_seat_status_info(sd_bus *bus, const char *path, bool *new_line) {

        static const struct bus_properties_map map[]  = {
                { "Id",            "s",     NULL, offsetof(SeatStatusInfo, id) },
                { "ActiveSession", "(so)",  prop_map_first_of_struct, offsetof(SeatStatusInfo, active_session) },
                { "Sessions",      "a(so)", prop_map_sessions_strv, offsetof(SeatStatusInfo, sessions) },
                {}
        };

        SeatStatusInfo i = {};
        int r;

        r = bus_map_all_properties(bus, "org.freedesktop.login1", path, map, &i);
        if (r < 0) {
                log_error("Could not get properties: %s", strerror(-r));
                goto finish;
        }

        if (*new_line)
                printf("\n");

        *new_line = true;

        printf("%s\n", strna(i.id));

        if (!strv_isempty(i.sessions)) {
                char **l;
                printf("\tSessions:");

                STRV_FOREACH(l, i.sessions) {
                        if (streq_ptr(*l, i.active_session))
                                printf(" *%s", *l);
                        else
                                printf(" %s", *l);
                }

                printf("\n");
        }

        if (arg_transport == BUS_TRANSPORT_LOCAL) {
                unsigned c;

                c = columns();
                if (c > 21)
                        c -= 21;
                else
                        c = 0;

                printf("\t Devices:\n");

                show_sysfs(i.id, "\t\t  ", c);
        }

finish:
        strv_free(i.sessions);

        return r;
}

static int show_properties(sd_bus *bus, const char *path, bool *new_line) {
        int r;

        if (*new_line)
                printf("\n");

        *new_line = true;

        r = bus_print_all_properties(bus, "org.freedesktop.login1", path, arg_property, arg_all);
        if (r < 0)
                log_error("Could not get properties: %s", strerror(-r));

        return r;
}

static int show_session(sd_bus *bus, char **args, unsigned n) {
        bool properties, new_line = false;
        unsigned i;
        int r;

        assert(bus);
        assert(args);

        properties = !strstr(args[0], "status");

        pager_open_if_enabled();

        if (properties && n <= 1) {
                /* If not argument is specified inspect the manager
                 * itself */
                return show_properties(bus, "/org/freedesktop/login1", &new_line);
        }

        for (i = 1; i < n; i++) {
                _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
                _cleanup_bus_message_unref_ sd_bus_message * reply = NULL;
                const char *path = NULL;

                r = sd_bus_call_method(
                                bus,
                                "org.freedesktop.login1",
                                "/org/freedesktop/login1",
                                "org.freedesktop.login1.Manager",
                                "GetSession",
                                &error, &reply,
                                "s", args[i]);
                if (r < 0) {
                        log_error("Failed to get session: %s", bus_error_message(&error, r));
                        return r;
                }

                r = sd_bus_message_read(reply, "o", &path);
                if (r < 0)
                        return bus_log_parse_error(r);

                if (properties)
                        r = show_properties(bus, path, &new_line);
                else
                        r = print_session_status_info(bus, path, &new_line);

                if (r < 0)
                        return r;
        }

        return 0;
}

static int show_user(sd_bus *bus, char **args, unsigned n) {
        bool properties, new_line = false;
        unsigned i;
        int r;

        assert(bus);
        assert(args);

        properties = !strstr(args[0], "status");

        pager_open_if_enabled();

        if (properties && n <= 1) {
                /* If not argument is specified inspect the manager
                 * itself */
                return show_properties(bus, "/org/freedesktop/login1", &new_line);
        }

        for (i = 1; i < n; i++) {
                _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
                _cleanup_bus_message_unref_ sd_bus_message * reply = NULL;
                const char *path = NULL;
                uid_t uid;

                r = get_user_creds((const char**) (args+i), &uid, NULL, NULL, NULL);
                if (r < 0) {
                        log_error("Failed to look up user %s: %s", args[i], strerror(-r));
                        return r;
                }

                r = sd_bus_call_method(
                                bus,
                                "org.freedesktop.login1",
                                "/org/freedesktop/login1",
                                "org.freedesktop.login1.Manager",
                                "GetUser",
                                &error, &reply,
                                "u", (uint32_t) uid);
                if (r < 0) {
                        log_error("Failed to get user: %s", bus_error_message(&error, r));
                        return r;
                }

                r = sd_bus_message_read(reply, "o", &path);
                if (r < 0)
                        return bus_log_parse_error(r);

                if (properties)
                        r = show_properties(bus, path, &new_line);
                else
                        r = print_user_status_info(bus, path, &new_line);

                if (r < 0)
                        return r;
        }

        return 0;
}

static int show_seat(sd_bus *bus, char **args, unsigned n) {
        bool properties, new_line = false;
        unsigned i;
        int r;

        assert(bus);
        assert(args);

        properties = !strstr(args[0], "status");

        pager_open_if_enabled();

        if (properties && n <= 1) {
                /* If not argument is specified inspect the manager
                 * itself */
                return show_properties(bus, "/org/freedesktop/login1", &new_line);
        }

        for (i = 1; i < n; i++) {
                _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
                _cleanup_bus_message_unref_ sd_bus_message * reply = NULL;
                const char *path = NULL;

                r = sd_bus_call_method(
                                bus,
                                "org.freedesktop.login1",
                                "/org/freedesktop/login1",
                                "org.freedesktop.login1.Manager",
                                "GetSeat",
                                &error, &reply,
                                "s", args[i]);
                if (r < 0) {
                        log_error("Failed to get seat: %s", bus_error_message(&error, r));
                        return r;
                }

                r = sd_bus_message_read(reply, "o", &path);
                if (r < 0)
                        return bus_log_parse_error(r);

                if (properties)
                        r = show_properties(bus, path, &new_line);
                else
                        r = print_seat_status_info(bus, path, &new_line);

                if (r < 0)
                        return r;
        }

        return 0;
}

static int activate(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        unsigned i;
        int r;

        assert(args);

        for (i = 1; i < n; i++) {

                r = sd_bus_call_method (
                                bus,
                                "org.freedesktop.login1",
                                "/org/freedesktop/login1",
                                "org.freedesktop.login1.Manager",
                                streq(args[0], "lock-session")      ? "LockSession" :
                                streq(args[0], "unlock-session")    ? "UnlockSession" :
                                streq(args[0], "terminate-session") ? "TerminateSession" :
                                                                      "ActivateSession",
                                &error, NULL,
                                "s", args[i]);
                if (r < 0) {
                        log_error("Failed to issue method call: %s", bus_error_message(&error, -r));
                        return r;
                }
        }

        return 0;
}

static int kill_session(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        unsigned i;
        int r;

        assert(args);

        if (!arg_kill_who)
                arg_kill_who = "all";

        for (i = 1; i < n; i++) {

                r = sd_bus_call_method (
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        "KillSession",
                        &error, NULL,
                        "ssi", args[i], arg_kill_who, arg_signal);
                if (r < 0) {
                        log_error("Could not kill session: %s", bus_error_message(&error, -r));
                        return r;
                }
        }

        return 0;
}

static int enable_linger(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        unsigned i;
        bool b;
        int r;

        assert(args);

        polkit_agent_open_if_enabled();

        b = streq(args[0], "enable-linger");

        for (i = 1; i < n; i++) {
                uid_t uid;

                r = get_user_creds((const char**) (args+i), &uid, NULL, NULL, NULL);
                if (r < 0) {
                        log_error("Failed to look up user %s: %s", args[i], strerror(-r));
                        return r;
                }

                r = sd_bus_call_method (
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        "SetUserLinger",
                        &error, NULL,
                        "ubb", (uint32_t) uid, b, true);
                if (r < 0) {
                        log_error("Could not enable linger: %s", bus_error_message(&error, -r));
                        return r;
                }
        }

        return 0;
}

static int terminate_user(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        unsigned i;
        int r;

        assert(args);

        for (i = 1; i < n; i++) {
                uid_t uid;

                r = get_user_creds((const char**) (args+i), &uid, NULL, NULL, NULL);
                if (r < 0) {
                        log_error("Failed to look up user %s: %s", args[i], strerror(-r));
                        return r;
                }

                r = sd_bus_call_method (
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        "TerminateUser",
                        &error, NULL,
                        "u", (uint32_t) uid);
                if (r < 0) {
                        log_error("Could not terminate user: %s", bus_error_message(&error, -r));
                        return r;
                }
        }

        return 0;
}

static int kill_user(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        unsigned i;
        int r;

        assert(args);

        if (!arg_kill_who)
                arg_kill_who = "all";

        for (i = 1; i < n; i++) {
                uid_t uid;

                r = get_user_creds((const char**) (args+i), &uid, NULL, NULL, NULL);
                if (r < 0) {
                        log_error("Failed to look up user %s: %s", args[i], strerror(-r));
                        return r;
                }

                r = sd_bus_call_method (
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        "KillUser",
                        &error, NULL,
                        "ui", (uint32_t) uid, arg_signal);
                if (r < 0) {
                        log_error("Could not kill user: %s", bus_error_message(&error, -r));
                        return r;
                }
        }

        return 0;
}

static int attach(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        unsigned i;
        int r;

        assert(args);

        polkit_agent_open_if_enabled();

        for (i = 2; i < n; i++) {

                r = sd_bus_call_method (
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        "AttachDevice",
                        &error, NULL,
                        "ssb", args[1], args[i], true);

                if (r < 0) {
                        log_error("Could not attach device: %s", bus_error_message(&error, -r));
                        return r;
                }
        }

        return 0;
}

static int flush_devices(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        int r;

        assert(args);

        polkit_agent_open_if_enabled();

        r = sd_bus_call_method (
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        "FlushDevices",
                        &error, NULL,
                        "b", true);
        if (r < 0)
                log_error("Could not flush devices: %s", bus_error_message(&error, -r));

        return r;
}

static int lock_sessions(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        int r;

        assert(args);

        r = sd_bus_call_method (
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        streq(args[0], "lock-sessions") ? "LockSessions" : "UnlockSessions",
                        &error, NULL,
                        NULL);
        if (r < 0)
                log_error("Could not lock sessions: %s", bus_error_message(&error, -r));

        return r;
}

static int terminate_seat(sd_bus *bus, char **args, unsigned n) {
        _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;
        unsigned i;
        int r;

        assert(args);

        for (i = 1; i < n; i++) {

                r = sd_bus_call_method (
                        bus,
                        "org.freedesktop.login1",
                        "/org/freedesktop/login1",
                        "org.freedesktop.login1.Manager",
                        "TerminateSeat",
                        &error, NULL,
                        "s", args[i]);
                if (r < 0) {
                        log_error("Could not terminate seat: %s", bus_error_message(&error, -r));
                        return r;
                }
        }

        return 0;
}

static void help(void) {
        printf("%s [OPTIONS...] {COMMAND} ...\n\n"
               "Send control commands to or query the login manager.\n\n"
               "  -h --help              Show this help\n"
               "     --version           Show package version\n"
               "     --no-pager          Do not pipe output into a pager\n"
               "     --no-legend         Do not show the headers and footers\n"
               "     --no-ask-password   Don't prompt for password\n"
               "  -H --host=[USER@]HOST  Operate on remote host\n"
               "  -M --machine=CONTAINER Operate on local container\n"
               "  -p --property=NAME     Show only properties by this name\n"
               "  -a --all               Show all properties, including empty ones\n"
               "  -l --full              Do not ellipsize output\n"
               "     --kill-who=WHO      Who to send signal to\n"
               "  -s --signal=SIGNAL     Which signal to send\n\n"
               "Commands:\n"
               "  list-sessions            List sessions\n"
               "  session-status ID...     Show session status\n"
               "  show-session [ID...]     Show properties of sessions or the manager\n"
               "  activate ID              Activate a session\n"
               "  lock-session ID...       Screen lock one or more sessions\n"
               "  unlock-session ID...     Screen unlock one or more sessions\n"
               "  lock-sessions            Screen lock all current sessions\n"
               "  unlock-sessions          Screen unlock all current sessions\n"
               "  terminate-session ID...  Terminate one or more sessions\n"
               "  kill-session ID...       Send signal to processes of a session\n"
               "  list-users               List users\n"
               "  user-status USER...      Show user status\n"
               "  show-user [USER...]      Show properties of users or the manager\n"
               "  enable-linger USER...    Enable linger state of one or more users\n"
               "  disable-linger USER...   Disable linger state of one or more users\n"
               "  terminate-user USER...   Terminate all sessions of one or more users\n"
               "  kill-user USER...        Send signal to processes of a user\n"
               "  list-seats               List seats\n"
               "  seat-status NAME...      Show seat status\n"
               "  show-seat NAME...        Show properties of one or more seats\n"
               "  attach NAME DEVICE...    Attach one or more devices to a seat\n"
               "  flush-devices            Flush all device associations\n"
               "  terminate-seat NAME...   Terminate all sessions on one or more seats\n"
               , program_invocation_short_name);
}

static int parse_argv(int argc, char *argv[]) {

        enum {
                ARG_VERSION = 0x100,
                ARG_NO_PAGER,
                ARG_NO_LEGEND,
                ARG_KILL_WHO,
                ARG_NO_ASK_PASSWORD,
        };

        static const struct option options[] = {
                { "help",            no_argument,       NULL, 'h'                 },
                { "version",         no_argument,       NULL, ARG_VERSION         },
                { "property",        required_argument, NULL, 'p'                 },
                { "all",             no_argument,       NULL, 'a'                 },
                { "full",            no_argument,       NULL, 'l'                 },
                { "no-pager",        no_argument,       NULL, ARG_NO_PAGER        },
                { "no-legend",       no_argument,       NULL, ARG_NO_LEGEND       },
                { "kill-who",        required_argument, NULL, ARG_KILL_WHO        },
                { "signal",          required_argument, NULL, 's'                 },
                { "host",            required_argument, NULL, 'H'                 },
                { "machine",         required_argument, NULL, 'M'                 },
                { "no-ask-password", no_argument,       NULL, ARG_NO_ASK_PASSWORD },
                {}
        };

        int c, r;

        assert(argc >= 0);
        assert(argv);

        while ((c = getopt_long(argc, argv, "hp:als:H:M:", options, NULL)) >= 0)

                switch (c) {

                case 'h':
                        help();
                        return 0;

                case ARG_VERSION:
                        puts(PACKAGE_STRING);
                        puts(SYSTEMD_FEATURES);
                        return 0;

                case 'p': {
                        r = strv_extend(&arg_property, optarg);
                        if (r < 0)
                                return log_oom();

                        /* If the user asked for a particular
                         * property, show it to him, even if it is
                         * empty. */
                        arg_all = true;
                        break;
                }

                case 'a':
                        arg_all = true;
                        break;

                case 'l':
                        arg_full = true;
                        break;

                case ARG_NO_PAGER:
                        arg_no_pager = true;
                        break;

                case ARG_NO_LEGEND:
                        arg_legend = false;
                        break;

                case ARG_NO_ASK_PASSWORD:
                        arg_ask_password = false;
                        break;

                case ARG_KILL_WHO:
                        arg_kill_who = optarg;
                        break;

                case 's':
                        arg_signal = signal_from_string_try_harder(optarg);
                        if (arg_signal < 0) {
                                log_error("Failed to parse signal string %s.", optarg);
                                return -EINVAL;
                        }
                        break;

                case 'H':
                        arg_transport = BUS_TRANSPORT_REMOTE;
                        arg_host = optarg;
                        break;

                case 'M':
                        arg_transport = BUS_TRANSPORT_CONTAINER;
                        arg_host = optarg;
                        break;

                case '?':
                        return -EINVAL;

                default:
                        assert_not_reached("Unhandled option");
                }

        return 1;
}

static int loginctl_main(sd_bus *bus, int argc, char *argv[]) {

        static const struct {
                const char* verb;
                const enum {
                        MORE,
                        LESS,
                        EQUAL
                } argc_cmp;
                const int argc;
                int (* const dispatch)(sd_bus *bus, char **args, unsigned n);
        } verbs[] = {
                { "list-sessions",         LESS,   1, list_sessions     },
                { "session-status",        MORE,   2, show_session      },
                { "show-session",          MORE,   1, show_session      },
                { "activate",              EQUAL,  2, activate          },
                { "lock-session",          MORE,   2, activate          },
                { "unlock-session",        MORE,   2, activate          },
                { "lock-sessions",         EQUAL,  1, lock_sessions     },
                { "unlock-sessions",       EQUAL,  1, lock_sessions     },
                { "terminate-session",     MORE,   2, activate          },
                { "kill-session",          MORE,   2, kill_session      },
                { "list-users",            EQUAL,  1, list_users        },
                { "user-status",           MORE,   2, show_user         },
                { "show-user",             MORE,   1, show_user         },
                { "enable-linger",         MORE,   2, enable_linger     },
                { "disable-linger",        MORE,   2, enable_linger     },
                { "terminate-user",        MORE,   2, terminate_user    },
                { "kill-user",             MORE,   2, kill_user         },
                { "list-seats",            EQUAL,  1, list_seats        },
                { "seat-status",           MORE,   2, show_seat         },
                { "show-seat",             MORE,   1, show_seat         },
                { "attach",                MORE,   3, attach            },
                { "flush-devices",         EQUAL,  1, flush_devices     },
                { "terminate-seat",        MORE,   2, terminate_seat    },
        };

        int left;
        unsigned i;

        assert(argc >= 0);
        assert(argv);

        left = argc - optind;

        if (left <= 0)
                /* Special rule: no arguments means "list-sessions" */
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

        return verbs[i].dispatch(bus, argv + optind, left);
}

int main(int argc, char *argv[]) {
        _cleanup_bus_close_unref_ sd_bus *bus = NULL;
        int r;

        setlocale(LC_ALL, "");
        log_parse_environment();
        log_open();

        r = parse_argv(argc, argv);
        if (r <= 0)
                goto finish;

        r = bus_open_transport(arg_transport, arg_host, false, &bus);
        if (r < 0) {
                log_error("Failed to create bus connection: %s", strerror(-r));
                goto finish;
        }

        r = loginctl_main(bus, argc, argv);

finish:
        pager_close();

        strv_free(arg_property);

        return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
