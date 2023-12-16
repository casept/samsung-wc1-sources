/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

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

#include <sys/types.h>

typedef struct UidRange {
        uid_t start, nr;
} UidRange;

int uid_range_add(UidRange **p, unsigned *n, uid_t start, uid_t nr);
int uid_range_add_str(UidRange **p, unsigned *n, const char *s);

int uid_range_next_lower(const UidRange *p, unsigned n, uid_t *uid);
bool uid_range_contains(const UidRange *p, unsigned n, uid_t uid);
