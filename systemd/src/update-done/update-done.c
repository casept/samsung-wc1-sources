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

#include "util.h"
#include "label.h"

#define MESSAGE                                                         \
        "This file was created by systemd-update-done. Its only \n"     \
        "purpose is to hold a timestamp of the time this directory\n"   \
        "was updated. See systemd-update-done.service(8).\n"

static int apply_timestamp(const char *path, struct timespec *ts) {
        struct timespec twice[2];
        struct stat st;

        assert(path);
        assert(ts);

        if (stat(path, &st) >= 0) {
                /* Is the timestamp file already newer than the OS? If so, there's nothing to do. */
                if (st.st_mtim.tv_sec > ts->tv_sec ||
                    (st.st_mtim.tv_sec == ts->tv_sec && st.st_mtim.tv_nsec >= ts->tv_nsec))
                        return 0;

                /* It is older? Then let's update it */
                twice[0] = *ts;
                twice[1] = *ts;

                if (utimensat(AT_FDCWD, path, twice, AT_SYMLINK_NOFOLLOW) < 0) {

                        if (errno == EROFS) {
                                log_debug("Can't update timestamp file %s, file system is read-only.", path);
                                return 0;
                        }

                        log_error("Failed to update timestamp on %s: %m", path);
                        return -errno;
                }

        } else if (errno == ENOENT) {
                _cleanup_close_ int fd = -1;
                int r;

                /* The timestamp file doesn't exist yet? Then let's create it. */

                r = mac_selinux_context_set(path, S_IFREG);
                if (r < 0) {
                        log_error("Failed to set SELinux context for %s: %s",
                                  path, strerror(-r));
                        return r;
                }

                fd = open(path, O_CREAT|O_EXCL|O_WRONLY|O_TRUNC|O_CLOEXEC|O_NOCTTY|O_NOFOLLOW, 0644);
                mac_selinux_context_clear();

                if (fd < 0) {

                        if (errno == EROFS) {
                                log_debug("Can't create timestamp file %s, file system is read-only.", path);
                                return 0;
                        }

                        log_error("Failed to create timestamp file %s: %m", path);
                        return -errno;
                }

                (void) loop_write(fd, MESSAGE, strlen(MESSAGE), false);

                twice[0] = *ts;
                twice[1] = *ts;

                if (futimens(fd, twice) < 0) {
                        log_error("Failed to update timestamp on %s: %m", path);
                        return -errno;
                }
        } else {
                log_error("Failed to stat() timestamp file %s: %m", path);
                return -errno;
        }

        return 0;
}

int main(int argc, char *argv[]) {
        struct stat st;
        int r, q = 0;

        log_set_target(LOG_TARGET_AUTO);
        log_parse_environment();
        log_open();

        if (stat("/usr", &st) < 0) {
                log_error("Failed to stat /usr: %m");
                return EXIT_FAILURE;
        }

        r = mac_selinux_init(NULL);
        if (r < 0) {
                log_error("SELinux setup failed: %s", strerror(-r));
                goto finish;
        }

        r = apply_timestamp("/etc/.updated", &st.st_mtim);
        q = apply_timestamp("/var/.updated", &st.st_mtim);

finish:
        return r < 0 || q < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
