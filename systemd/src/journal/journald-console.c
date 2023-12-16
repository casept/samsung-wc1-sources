/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2011 Lennart Poettering

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

#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

#include "fileio.h"
#include "journald-server.h"
#include "journald-console.h"

static bool prefix_timestamp(void) {

        static int cached_printk_time = -1;

        if (_unlikely_(cached_printk_time < 0)) {
                _cleanup_free_ char *p = NULL;

                cached_printk_time =
                        read_one_line_file("/sys/module/printk/parameters/time", &p) >= 0
                        && parse_boolean(p) > 0;
        }

        return cached_printk_time;
}

void server_forward_console(
                Server *s,
                int priority,
                const char *identifier,
                const char *message,
                struct ucred *ucred) {

        struct iovec iovec[5];
        char header_pid[16];
        struct timespec ts;
        char tbuf[4 + DECIMAL_STR_MAX(ts.tv_sec) + DECIMAL_STR_MAX(ts.tv_nsec)-3 + 1];
        int n = 0, fd;
        _cleanup_free_ char *ident_buf = NULL;
        const char *tty;

        assert(s);
        assert(message);

        if (LOG_PRI(priority) > s->max_level_console)
                return;

        /* First: timestamp */
        if (prefix_timestamp()) {
                assert_se(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);
                snprintf(tbuf, sizeof(tbuf), "[%5"PRI_TIME".%06ld] ",
                         ts.tv_sec,
                         ts.tv_nsec / 1000);
                IOVEC_SET_STRING(iovec[n++], tbuf);
        }

        /* Second: identifier and PID */
        if (ucred) {
                if (!identifier) {
                        get_process_comm(ucred->pid, &ident_buf);
                        identifier = ident_buf;
                }

                snprintf(header_pid, sizeof(header_pid), "["PID_FMT"]: ", ucred->pid);
                char_array_0(header_pid);

                if (identifier)
                        IOVEC_SET_STRING(iovec[n++], identifier);

                IOVEC_SET_STRING(iovec[n++], header_pid);
        } else if (identifier) {
                IOVEC_SET_STRING(iovec[n++], identifier);
                IOVEC_SET_STRING(iovec[n++], ": ");
        }

        /* Fourth: message */
        IOVEC_SET_STRING(iovec[n++], message);
        IOVEC_SET_STRING(iovec[n++], "\n");

        tty = s->tty_path ? s->tty_path : "/dev/console";

        fd = open_terminal(tty, O_WRONLY|O_NOCTTY|O_CLOEXEC);
        if (fd < 0) {
                log_debug("Failed to open %s for logging: %m", tty);
                return;
        }

        if (writev(fd, iovec, n) < 0)
                log_debug("Failed to write to %s for logging: %m", tty);

        safe_close(fd);
}
