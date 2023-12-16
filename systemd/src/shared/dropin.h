/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

/***
  This file is part of systemd.

  Copyright 2014 Zbigniew Jędrzejewski-Szmek

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

#include "macro.h"

int drop_in_file(const char *dir, const char *unit, unsigned level,
                 const char *name, char **_p, char **_q);

int write_drop_in(const char *dir, const char *unit, unsigned level,
                  const char *name, const char *data);

int write_drop_in_format(const char *dir, const char *unit, unsigned level,
                         const char *name, const char *format, ...) _printf_(5, 6);
