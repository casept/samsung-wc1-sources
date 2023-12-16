/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

/***
  This file is part of systemd.

  Copyright (C) 2014 Intel Corporation. All rights reserved.

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

#include <net/ethernet.h>
#include <netinet/in.h>

#include "sparse-endian.h"
#include "sd-event.h"
#include "list.h"
#include "macro.h"

typedef struct DHCP6Address DHCP6Address;

struct DHCP6Address {
        LIST_FIELDS(DHCP6Address, addresses);

        struct {
                struct in6_addr address;
                be32_t lifetime_preferred;
                be32_t lifetime_valid;
        } _packed_;
};

struct DHCP6IA {
        uint16_t type;
        struct {
                be32_t id;
                be32_t lifetime_t1;
                be32_t lifetime_t2;
        } _packed_;
        sd_event_source *timeout_t1;
        sd_event_source *timeout_t2;

        LIST_HEAD(DHCP6Address, addresses);
};

typedef struct DHCP6IA DHCP6IA;

#define log_dhcp6_client(p, fmt, ...) log_meta(LOG_DEBUG, __FILE__, __LINE__, __func__, "DHCPv6 CLIENT: " fmt, ##__VA_ARGS__)

int dhcp_network_icmp6_bind_router_solicitation(int index);
int dhcp_network_icmp6_send_router_solicitation(int s, const struct ether_addr *ether_addr);

int dhcp6_option_append(uint8_t **buf, size_t *buflen, uint16_t code,
                        size_t optlen, const void *optval);
int dhcp6_option_append_ia(uint8_t **buf, size_t *buflen, DHCP6IA *ia);
int dhcp6_option_parse(uint8_t **buf, size_t *buflen, uint16_t *optcode,
                       size_t *optlen, uint8_t **optvalue);
int dhcp6_option_parse_ia(uint8_t **buf, size_t *buflen, uint16_t iatype,
                          DHCP6IA *ia);

int dhcp6_network_bind_udp_socket(int index, struct in6_addr *address);
int dhcp6_network_send_udp_socket(int s, struct in6_addr *address,
                                  const void *packet, size_t len);

const char *dhcp6_message_type_to_string(int s) _const_;
int dhcp6_message_type_from_string(const char *s) _pure_;
const char *dhcp6_message_status_to_string(int s) _const_;
int dhcp6_message_status_from_string(const char *s) _pure_;
