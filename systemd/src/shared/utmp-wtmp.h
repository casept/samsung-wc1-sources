/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

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

#include "util.h"

int utmp_get_runlevel(int *runlevel, int *previous);

int utmp_put_shutdown(void);
int utmp_put_reboot(usec_t timestamp);
int utmp_put_runlevel(int runlevel, int previous);

int utmp_put_dead_process(const char *id, pid_t pid, int code, int status);
int utmp_put_init_process(const char *id, pid_t pid, pid_t sid, const char *line);

int utmp_wall(const char *message, const char *username, bool (*match_tty)(const char *tty));
