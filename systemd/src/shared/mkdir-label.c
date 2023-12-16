/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering
  Copyright 2013 Kay Sievers

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

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "label.h"
#include "util.h"
#include "path-util.h"
#include "mkdir.h"

static int label_mkdir(const char *path, mode_t mode) {
        int r;

        if (mac_selinux_use()) {
                r = mac_selinux_mkdir(path, mode);
                if (r < 0)
                        return r;
        }

        if (mac_smack_use()) {
                r = mkdir(path, mode);
                if (r < 0 && errno != EEXIST)
                        return -errno;

                r = mac_smack_relabel_in_dev(path);
                if (r < 0)
                        return r;
        }

        r = mkdir(path, mode);
        if (r < 0 && errno != EEXIST)
                return -errno;

        return 0;
}

int mkdir_label(const char *path, mode_t mode) {
        return label_mkdir(path, mode);
}

int mkdir_safe_label(const char *path, mode_t mode, uid_t uid, gid_t gid) {
        return mkdir_safe_internal(path, mode, uid, gid, label_mkdir);
}

int mkdir_parents_label(const char *path, mode_t mode) {
        return mkdir_parents_internal(NULL, path, mode, label_mkdir);
}

int mkdir_parents_prefix_label(const char *prefix, const char *path, mode_t mode) {
        return mkdir_parents_internal(prefix, path, mode, label_mkdir);
}

int mkdir_p_label(const char *path, mode_t mode) {
        return mkdir_p_internal(NULL, path, mode, label_mkdir);
}
