/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

/***
 This file is part of systemd.

 Copyright (C) 2013 Tom Gundersen <teg@jklm.no>

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

#include "util.h"
#include "sd-rtnl.h"

int rtnl_message_new_synthetic_error(int error, uint32_t serial, sd_rtnl_message **ret);
uint32_t rtnl_message_get_serial(sd_rtnl_message *m);
void rtnl_message_seal(sd_rtnl_message *m);

bool rtnl_message_type_is_link(uint16_t type);
bool rtnl_message_type_is_addr(uint16_t type);
bool rtnl_message_type_is_route(uint16_t type);

int rtnl_set_link_name(sd_rtnl *rtnl, int ifindex, const char *name);
int rtnl_set_link_properties(sd_rtnl *rtnl, int ifindex, const char *alias, const struct ether_addr *mac, unsigned mtu);

int rtnl_log_parse_error(int r);
int rtnl_log_create_error(int r);

DEFINE_TRIVIAL_CLEANUP_FUNC(sd_rtnl*, sd_rtnl_unref);
DEFINE_TRIVIAL_CLEANUP_FUNC(sd_rtnl_message*, sd_rtnl_message_unref);

#define _cleanup_rtnl_unref_ _cleanup_(sd_rtnl_unrefp)
#define _cleanup_rtnl_message_unref_ _cleanup_(sd_rtnl_message_unrefp)
