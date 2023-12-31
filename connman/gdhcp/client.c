/*
 *
 *  DHCP client library with GLib integration
 *
 *  Copyright (C) 2009-2012  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <netpacket/packet.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>

#include <linux/if.h>
#include <linux/filter.h>

#include <glib.h>
#if defined TIZEN_EXT
#include <connman/setting.h>
#endif

#include "gdhcp.h"
#include "common.h"
#include "ipv4ll.h"
#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
#include "tizen/rtctimer.h"
#endif

#define DISCOVER_TIMEOUT 3
#define DISCOVER_RETRIES 10

#define REQUEST_TIMEOUT 3
#define REQUEST_RETRIES 5

typedef enum _listen_mode {
	L_NONE,
	L2,
	L3,
	L_ARP,
} ListenMode;

typedef enum _dhcp_client_state {
	INIT_SELECTING,
	REQUESTING,
	BOUND,
	RENEWING,
	REBINDING,
	RELEASED,
	IPV4LL_PROBE,
	IPV4LL_ANNOUNCE,
	IPV4LL_MONITOR,
	IPV4LL_DEFEND,
	INFORMATION_REQ,
	SOLICITATION,
	REQUEST,
	CONFIRM,
	RENEW,
	REBIND,
	RELEASE,
} ClientState;

struct _GDHCPClient {
	int ref_count;
	GDHCPType type;
	ClientState state;
	int ifindex;
	char *interface;
	uint8_t mac_address[6];
	uint32_t xid;
	uint32_t server_ip;
	uint32_t requested_ip;
	char *assigned_ip;
	time_t start;
	uint32_t lease_seconds;
	ListenMode listen_mode;
	int listener_sockfd;
	uint8_t retry_times;
	uint8_t ack_retry_times;
	uint8_t conflicts;
	guint timeout;
#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
	int rtc_timeout;
#endif
	guint listener_watch;
	GIOChannel *listener_channel;
	GList *require_list;
	GList *request_list;
	GHashTable *code_value_hash;
	GHashTable *send_value_hash;
	GDHCPClientEventFunc lease_available_cb;
	gpointer lease_available_data;
	GDHCPClientEventFunc ipv4ll_available_cb;
	gpointer ipv4ll_available_data;
	GDHCPClientEventFunc no_lease_cb;
	gpointer no_lease_data;
	GDHCPClientEventFunc lease_lost_cb;
	gpointer lease_lost_data;
	GDHCPClientEventFunc ipv4ll_lost_cb;
	gpointer ipv4ll_lost_data;
	GDHCPClientEventFunc address_conflict_cb;
	gpointer address_conflict_data;
	GDHCPDebugFunc debug_func;
	gpointer debug_data;
	GDHCPClientEventFunc information_req_cb;
	gpointer information_req_data;
	GDHCPClientEventFunc solicitation_cb;
	gpointer solicitation_data;
	GDHCPClientEventFunc advertise_cb;
	gpointer advertise_data;
	GDHCPClientEventFunc request_cb;
	gpointer request_data;
	GDHCPClientEventFunc renew_cb;
	gpointer renew_data;
	GDHCPClientEventFunc rebind_cb;
	gpointer rebind_data;
	GDHCPClientEventFunc release_cb;
	gpointer release_data;
	GDHCPClientEventFunc confirm_cb;
	gpointer confirm_data;
	char *last_address;
	unsigned char *duid;
	int duid_len;
	unsigned char *server_duid;
	int server_duid_len;
	uint16_t status_code;
	uint32_t iaid;
	uint32_t T1, T2;
	struct in6_addr ia_na;
	struct in6_addr ia_ta;
	time_t last_renew;
	time_t last_rebind;
	time_t expire;
	gboolean retransmit;
	struct timeval start_time;
#if defined TIZEN_EXT
	gboolean init_reboot;
#endif
};

static inline void debug(GDHCPClient *client, const char *format, ...)
{
	char str[256];
	va_list ap;

	if (client->debug_func == NULL)
		return;

	va_start(ap, format);

	if (vsnprintf(str, sizeof(str), format, ap) > 0)
		client->debug_func(str, client->debug_data);

	va_end(ap);
}

/* Initialize the packet with the proper defaults */
static void init_packet(GDHCPClient *dhcp_client, gpointer pkt, char type)
{
	if (dhcp_client->type == G_DHCP_IPV6)
		dhcpv6_init_header(pkt, type);
	else {
		struct dhcp_packet *packet = pkt;

		dhcp_init_header(packet, type);
		memcpy(packet->chaddr, dhcp_client->mac_address, 6);
	}
}

static void add_request_options(GDHCPClient *dhcp_client,
				struct dhcp_packet *packet)
{
	int len = 0;
	GList *list;
	uint8_t code;
	int end = dhcp_end_option(packet->options);

	for (list = dhcp_client->request_list; list; list = list->next) {
		code = (uint8_t) GPOINTER_TO_INT(list->data);

		packet->options[end + OPT_DATA + len] = code;
		len++;
	}

	if (len) {
		packet->options[end + OPT_CODE] = DHCP_PARAM_REQ;
		packet->options[end + OPT_LEN] = len;
		packet->options[end + OPT_DATA + len] = DHCP_END;
	}
}

struct hash_params {
	unsigned char *buf;
	int max_buf;
	unsigned char **ptr_buf;
};

static void add_dhcpv6_binary_option(gpointer key, gpointer value,
					gpointer user_data)
{
	uint8_t *option = value;
	uint16_t len;
	struct hash_params *params = user_data;

	/* option[0][1] contains option code */
	len = option[2] << 8 | option[3];

	if ((*params->ptr_buf + len + 2 + 2) > (params->buf + params->max_buf))
		return;

	memcpy(*params->ptr_buf, option, len + 2 + 2);
	(*params->ptr_buf) += len + 2 + 2;
}

static void add_dhcpv6_send_options(GDHCPClient *dhcp_client,
				unsigned char *buf, int max_buf,
				unsigned char **ptr_buf)
{
	struct hash_params params = {
		.buf = buf,
		.max_buf = max_buf,
		.ptr_buf = ptr_buf
	};

	if (dhcp_client->type == G_DHCP_IPV4)
		return;

	g_hash_table_foreach(dhcp_client->send_value_hash,
				add_dhcpv6_binary_option, &params);

	*ptr_buf = *params.ptr_buf;
}

static void copy_option(uint8_t *buf, uint16_t code, uint16_t len,
			uint8_t *msg)
{
	buf[0] = code >> 8;
	buf[1] = code & 0xff;
	buf[2] = len >> 8;
	buf[3] = len & 0xff;
	if (len > 0 && msg != NULL)
		memcpy(&buf[4], msg, len);
}

static int32_t get_time_diff(struct timeval *tv)
{
	struct timeval now;
	int32_t hsec;

	gettimeofday(&now, NULL);

	hsec = (now.tv_sec - tv->tv_sec) * 100;
	hsec += (now.tv_usec - tv->tv_usec) / 10000;

	return hsec;
}

static void remove_timeouts(GDHCPClient *dhcp_client)
{
	if (dhcp_client->timeout > 0)
		g_source_remove(dhcp_client->timeout);

	dhcp_client->timeout = 0;
}

static void add_dhcpv6_request_options(GDHCPClient *dhcp_client,
				struct dhcpv6_packet *packet,
				unsigned char *buf, int max_buf,
				unsigned char **ptr_buf)
{
	GList *list;
	uint16_t code, value;
	int32_t diff;
	int len;

	if (dhcp_client->type == G_DHCP_IPV4)
		return;

	for (list = dhcp_client->request_list; list; list = list->next) {
		code = (uint16_t) GPOINTER_TO_INT(list->data);

		switch (code) {
		case G_DHCPV6_CLIENTID:
			if (dhcp_client->duid == NULL)
				return;

			len = 2 + 2 + dhcp_client->duid_len;
			if ((*ptr_buf + len) > (buf + max_buf)) {
				debug(dhcp_client, "Too long dhcpv6 message "
					"when writing client id option");
				return;
			}

			copy_option(*ptr_buf, G_DHCPV6_CLIENTID,
				dhcp_client->duid_len, dhcp_client->duid);
			(*ptr_buf) += len;
			break;

		case G_DHCPV6_SERVERID:
			if (dhcp_client->server_duid == NULL)
				return;

			len = 2 + 2 + dhcp_client->server_duid_len;
			if ((*ptr_buf + len) > (buf + max_buf)) {
				debug(dhcp_client, "Too long dhcpv6 message "
					"when writing server id option");
				return;
			}

			copy_option(*ptr_buf, G_DHCPV6_SERVERID,
				dhcp_client->server_duid_len,
				dhcp_client->server_duid);
			(*ptr_buf) += len;
			break;

		case G_DHCPV6_RAPID_COMMIT:
			len = 2 + 2;
			if ((*ptr_buf + len) > (buf + max_buf)) {
				debug(dhcp_client, "Too long dhcpv6 message "
					"when writing rapid commit option");
				return;
			}

			copy_option(*ptr_buf, G_DHCPV6_RAPID_COMMIT, 0, 0);
			(*ptr_buf) += len;
			break;

		case G_DHCPV6_ORO:
			break;

		case G_DHCPV6_ELAPSED_TIME:
			if (dhcp_client->retransmit == FALSE) {
				/*
				 * Initial message, elapsed time is 0.
				 */
				diff = 0;
			} else {
				diff = get_time_diff(&dhcp_client->start_time);
				if (diff < 0 || diff > 0xffff)
					diff = 0xffff;
			}

			len = 2 + 2 + 2;
			if ((*ptr_buf + len) > (buf + max_buf)) {
				debug(dhcp_client, "Too long dhcpv6 message "
					"when writing elapsed time option");
				return;
			}

			value = htons((uint16_t)diff);
			copy_option(*ptr_buf, G_DHCPV6_ELAPSED_TIME,
				2, (uint8_t *)&value);
			(*ptr_buf) += len;
			break;

		case G_DHCPV6_DNS_SERVERS:
			break;

		case G_DHCPV6_SNTP_SERVERS:
			break;

		default:
			break;
		}
	}
}

static void add_binary_option(gpointer key, gpointer value, gpointer user_data)
{
	uint8_t *option = value;
	struct dhcp_packet *packet = user_data;

	dhcp_add_binary_option(packet, option);
}

static void add_send_options(GDHCPClient *dhcp_client,
				struct dhcp_packet *packet)
{
	g_hash_table_foreach(dhcp_client->send_value_hash,
				add_binary_option, packet);
}

/*
 * Return an RFC 951- and 2131-complaint BOOTP 'secs' value that
 * represents the number of seconds elapsed from the start of
 * attempting DHCP to satisfy some DHCP servers that allow for an
 * "authoritative" reply before responding.
 */
static uint16_t dhcp_attempt_secs(GDHCPClient *dhcp_client)
{
	return htons(MIN(time(NULL) - dhcp_client->start, UINT16_MAX));
}

static int send_discover(GDHCPClient *dhcp_client, uint32_t requested)
{
	struct dhcp_packet packet;

	debug(dhcp_client, "sending DHCP discover request");

	init_packet(dhcp_client, &packet, DHCPDISCOVER);

	packet.xid = dhcp_client->xid;
	packet.secs = dhcp_attempt_secs(dhcp_client);

	if (requested)
		dhcp_add_option_uint32(&packet, DHCP_REQUESTED_IP, requested);

	/* Explicitly saying that we want RFC-compliant packets helps
	 * some buggy DHCP servers to NOT send bigger packets */
	dhcp_add_option_uint16(&packet, DHCP_MAX_SIZE, 576);

	add_request_options(dhcp_client, &packet);

	add_send_options(dhcp_client, &packet);

	return dhcp_send_raw_packet(&packet, INADDR_ANY, CLIENT_PORT,
					INADDR_BROADCAST, SERVER_PORT,
					MAC_BCAST_ADDR, dhcp_client->ifindex);
}

static int send_select(GDHCPClient *dhcp_client)
{
	struct dhcp_packet packet;

	debug(dhcp_client, "sending DHCP select request");

	init_packet(dhcp_client, &packet, DHCPREQUEST);

	packet.xid = dhcp_client->xid;
#if defined TIZEN_EXT
	if (dhcp_client->init_reboot != TRUE)
#endif
	packet.secs = dhcp_attempt_secs(dhcp_client);

	dhcp_add_option_uint32(&packet, DHCP_REQUESTED_IP,
						dhcp_client->requested_ip);
#if defined TIZEN_EXT
	if (dhcp_client->init_reboot != TRUE)
#endif
	dhcp_add_option_uint32(&packet, DHCP_SERVER_ID,
						dhcp_client->server_ip);

	add_request_options(dhcp_client, &packet);

	add_send_options(dhcp_client, &packet);

	return dhcp_send_raw_packet(&packet, INADDR_ANY, CLIENT_PORT,
					INADDR_BROADCAST, SERVER_PORT,
					MAC_BCAST_ADDR, dhcp_client->ifindex);
}

static int send_renew(GDHCPClient *dhcp_client)
{
	struct dhcp_packet packet;

	debug(dhcp_client, "sending DHCP renew request");

	init_packet(dhcp_client , &packet, DHCPREQUEST);
	packet.xid = dhcp_client->xid;
	packet.ciaddr = htonl(dhcp_client->requested_ip);

	add_request_options(dhcp_client, &packet);

	add_send_options(dhcp_client, &packet);

	return dhcp_send_kernel_packet(&packet,
		dhcp_client->requested_ip, CLIENT_PORT,
		dhcp_client->server_ip, SERVER_PORT);
}

static int send_rebound(GDHCPClient *dhcp_client)
{
	struct dhcp_packet packet;

	debug(dhcp_client, "sending DHCP rebound request");

	init_packet(dhcp_client , &packet, DHCPREQUEST);
	packet.xid = dhcp_client->xid;
	packet.ciaddr = htonl(dhcp_client->requested_ip);

	add_request_options(dhcp_client, &packet);

	add_send_options(dhcp_client, &packet);

	return dhcp_send_raw_packet(&packet, INADDR_ANY, CLIENT_PORT,
					INADDR_BROADCAST, SERVER_PORT,
					MAC_BCAST_ADDR, dhcp_client->ifindex);
}

static int send_release(GDHCPClient *dhcp_client,
			uint32_t server, uint32_t ciaddr)
{
	struct dhcp_packet packet;

#if defined TIZEN_EXT
	if (connman_setting_get_bool("WiFiDHCPRelease") != TRUE)
		return 0;
#endif
	debug(dhcp_client, "sending DHCP release request");

	init_packet(dhcp_client, &packet, DHCPRELEASE);
	packet.xid = rand();
	packet.ciaddr = htonl(ciaddr);

	dhcp_add_option_uint32(&packet, DHCP_SERVER_ID, server);

	return dhcp_send_kernel_packet(&packet, ciaddr, CLIENT_PORT,
						server, SERVER_PORT);
}

static gboolean ipv4ll_probe_timeout(gpointer dhcp_data);
static int switch_listening_mode(GDHCPClient *dhcp_client,
					ListenMode listen_mode);

static gboolean send_probe_packet(gpointer dhcp_data)
{
	GDHCPClient *dhcp_client;
	guint timeout;

	dhcp_client = dhcp_data;
	/* if requested_ip is not valid, pick a new address*/
	if (dhcp_client->requested_ip == 0) {
		debug(dhcp_client, "pick a new random address");
		dhcp_client->requested_ip = ipv4ll_random_ip(0);
	}

	debug(dhcp_client, "sending IPV4LL probe request");

	if (dhcp_client->retry_times == 1) {
		dhcp_client->state = IPV4LL_PROBE;
		switch_listening_mode(dhcp_client, L_ARP);
	}
	ipv4ll_send_arp_packet(dhcp_client->mac_address, 0,
			dhcp_client->requested_ip, dhcp_client->ifindex);

	if (dhcp_client->retry_times < PROBE_NUM) {
		/*add a random timeout in range of PROBE_MIN to PROBE_MAX*/
		timeout = ipv4ll_random_delay_ms(PROBE_MAX-PROBE_MIN);
		timeout += PROBE_MIN*1000;
	} else
		timeout = (ANNOUNCE_WAIT * 1000);

	dhcp_client->timeout = g_timeout_add_full(G_PRIORITY_HIGH,
						 timeout,
						 ipv4ll_probe_timeout,
						 dhcp_client,
						 NULL);
	return FALSE;
}

static gboolean ipv4ll_announce_timeout(gpointer dhcp_data);
static gboolean ipv4ll_defend_timeout(gpointer dhcp_data);

static gboolean send_announce_packet(gpointer dhcp_data)
{
	GDHCPClient *dhcp_client;

	dhcp_client = dhcp_data;

	debug(dhcp_client, "sending IPV4LL announce request");

	ipv4ll_send_arp_packet(dhcp_client->mac_address,
				dhcp_client->requested_ip,
				dhcp_client->requested_ip,
				dhcp_client->ifindex);

	remove_timeouts(dhcp_client);

	if (dhcp_client->state == IPV4LL_DEFEND) {
		dhcp_client->timeout =
			g_timeout_add_seconds_full(G_PRIORITY_HIGH,
						DEFEND_INTERVAL,
						ipv4ll_defend_timeout,
						dhcp_client,
						NULL);
		return TRUE;
	} else
		dhcp_client->timeout =
			g_timeout_add_seconds_full(G_PRIORITY_HIGH,
						ANNOUNCE_INTERVAL,
						ipv4ll_announce_timeout,
						dhcp_client,
						NULL);
	return TRUE;
}

static void get_interface_mac_address(int index, uint8_t *mac_address)
{
	struct ifreq ifr;
	int sk, err;

	sk = socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (sk < 0) {
		perror("Open socket error");
		return;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_ifindex = index;

	err = ioctl(sk, SIOCGIFNAME, &ifr);
	if (err < 0) {
		perror("Get interface name error");
		goto done;
	}

	err = ioctl(sk, SIOCGIFHWADDR, &ifr);
	if (err < 0) {
		perror("Get mac address error");
		goto done;
	}

	memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);

done:
	close(sk);
}

void g_dhcpv6_client_set_retransmit(GDHCPClient *dhcp_client)
{
	if (dhcp_client == NULL)
		return;

	dhcp_client->retransmit = TRUE;
}

void g_dhcpv6_client_clear_retransmit(GDHCPClient *dhcp_client)
{
	if (dhcp_client == NULL)
		return;

	dhcp_client->retransmit = FALSE;
}

int g_dhcpv6_create_duid(GDHCPDuidType duid_type, int index, int type,
			unsigned char **duid, int *duid_len)
{
	time_t duid_time;

	switch (duid_type) {
	case G_DHCPV6_DUID_LLT:
		*duid_len = 2 + 2 + 4 + ETH_ALEN;
		*duid = g_try_malloc(*duid_len);
		if (*duid == NULL)
			return -ENOMEM;

		(*duid)[0] = 0;
		(*duid)[1] = 1;
		get_interface_mac_address(index, &(*duid)[2 + 2 + 4]);
		(*duid)[2] = 0;
		(*duid)[3] = type;
		duid_time = time(NULL) - DUID_TIME_EPOCH;
		(*duid)[4] = duid_time >> 24;
		(*duid)[5] = duid_time >> 16;
		(*duid)[6] = duid_time >> 8;
		(*duid)[7] = duid_time & 0xff;
		break;
	case G_DHCPV6_DUID_EN:
		return -EINVAL;
	case G_DHCPV6_DUID_LL:
		*duid_len = 2 + 2 + ETH_ALEN;
		*duid = g_try_malloc(*duid_len);
		if (*duid == NULL)
			return -ENOMEM;

		(*duid)[0] = 0;
		(*duid)[1] = 3;
		get_interface_mac_address(index, &(*duid)[2 + 2]);
		(*duid)[2] = 0;
		(*duid)[3] = type;
		break;
	}

	return 0;
}

int g_dhcpv6_client_set_duid(GDHCPClient *dhcp_client, unsigned char *duid,
			int duid_len)
{
	if (dhcp_client == NULL || dhcp_client->type == G_DHCP_IPV4)
		return -EINVAL;

	g_free(dhcp_client->duid);

	dhcp_client->duid = duid;
	dhcp_client->duid_len = duid_len;

	return 0;
}

uint32_t g_dhcpv6_client_get_iaid(GDHCPClient *dhcp_client)
{
	if (dhcp_client == NULL || dhcp_client->type == G_DHCP_IPV4)
		return 0;

	return dhcp_client->iaid;
}

void g_dhcpv6_client_create_iaid(GDHCPClient *dhcp_client, int index,
				unsigned char *iaid)
{
	uint8_t buf[6];

	get_interface_mac_address(index, buf);

	memcpy(iaid, &buf[2], 4);
	dhcp_client->iaid = iaid[0] << 24 |
			iaid[1] << 16 | iaid[2] << 8 | iaid[3];
}

int g_dhcpv6_client_get_timeouts(GDHCPClient *dhcp_client,
				uint32_t *T1, uint32_t *T2,
				time_t *last_renew, time_t *last_rebind,
				time_t *expire)
{
	if (dhcp_client == NULL || dhcp_client->type == G_DHCP_IPV4)
		return -EINVAL;

	if (T1 != NULL)
		*T1 = dhcp_client->T1;

	if (T2 != NULL)
		*T2 = dhcp_client->T2;

	if (last_renew != NULL)
		*last_renew = dhcp_client->last_renew;

	if (last_rebind != NULL)
		*last_rebind = dhcp_client->last_rebind;

	if (expire != NULL)
		*expire = dhcp_client->expire;

	return 0;
}

static uint8_t *create_iaaddr(GDHCPClient *dhcp_client, uint8_t *buf,
				uint16_t len)
{
	buf[0] = 0;
	buf[1] = G_DHCPV6_IAADDR;
	buf[2] = 0;
	buf[3] = len;
	memcpy(&buf[4], &dhcp_client->ia_na, 16);
	memset(&buf[20], 0, 4); /* preferred */
	memset(&buf[24], 0, 4); /* valid */
	return buf;
}

static void put_iaid(GDHCPClient *dhcp_client, int index, uint8_t *buf)
{
	uint32_t iaid;

	iaid = g_dhcpv6_client_get_iaid(dhcp_client);
	if (iaid == 0) {
		g_dhcpv6_client_create_iaid(dhcp_client, index, buf);
		return;
	}

	buf[0] = iaid >> 24;
	buf[1] = iaid >> 16;
	buf[2] = iaid >> 8;
	buf[3] = iaid;
}

int g_dhcpv6_client_set_ia(GDHCPClient *dhcp_client, int index,
			int code, uint32_t *T1, uint32_t *T2,
			gboolean add_iaaddr, const char *ia_na)
{
	if (code == G_DHCPV6_IA_TA) {
		uint8_t ia_options[4];

		put_iaid(dhcp_client, index, ia_options);

		g_dhcp_client_set_request(dhcp_client, G_DHCPV6_IA_TA);
		g_dhcpv6_client_set_send(dhcp_client, G_DHCPV6_IA_TA,
					ia_options, sizeof(ia_options));

	} else if (code == G_DHCPV6_IA_NA) {
		struct in6_addr addr;

		g_dhcp_client_set_request(dhcp_client, G_DHCPV6_IA_NA);

		/*
		 * If caller has specified the IPv6 address it wishes to
		 * to use (ia_na != NULL and address is valid), then send
		 * the address to server.
		 * If caller did not specify the address (ia_na == NULL) and
		 * if the current address is not set, then we should not send
		 * the address sub-option.
		 */
		if (add_iaaddr == TRUE && ((ia_na == NULL &&
			IN6_IS_ADDR_UNSPECIFIED(&dhcp_client->ia_na) == FALSE)
			|| (ia_na != NULL &&
				inet_pton(AF_INET6, ia_na, &addr) == 1))) {
#define IAADDR_LEN (16+4+4)
			uint8_t ia_options[4+4+4+2+2+IAADDR_LEN];

			if (ia_na != NULL)
				memcpy(&dhcp_client->ia_na, &addr,
						sizeof(struct in6_addr));

			put_iaid(dhcp_client, index, ia_options);

			if (T1 != NULL) {
				ia_options[4] = *T1 >> 24;
				ia_options[5] = *T1 >> 16;
				ia_options[6] = *T1 >> 8;
				ia_options[7] = *T1;
			} else
				memset(&ia_options[4], 0x00, 4);

			if (T2 != NULL) {
				ia_options[8] = *T2 >> 24;
				ia_options[9] = *T2 >> 16;
				ia_options[10] = *T2 >> 8;
				ia_options[11] = *T2;
			} else
				memset(&ia_options[8], 0x00, 4);

			create_iaaddr(dhcp_client, &ia_options[12],
					IAADDR_LEN);

			g_dhcpv6_client_set_send(dhcp_client, G_DHCPV6_IA_NA,
					ia_options, sizeof(ia_options));
		} else {
			uint8_t ia_options[4+4+4];

			put_iaid(dhcp_client, index, ia_options);

			memset(&ia_options[4], 0x00, 4); /* T1 (4 bytes) */
			memset(&ia_options[8], 0x00, 4); /* T2 (4 bytes) */

			g_dhcpv6_client_set_send(dhcp_client, G_DHCPV6_IA_NA,
					ia_options, sizeof(ia_options));
		}

	} else
		return -EINVAL;

	return 0;
}

int g_dhcpv6_client_set_oro(GDHCPClient *dhcp_client, int args, ...)
{
	va_list va;
	int i, j, len = sizeof(uint16_t) * args;
	uint8_t *values;

	values = g_try_malloc(len);
	if (values == NULL)
		return -ENOMEM;

	va_start(va, args);
	for (i = 0, j = 0; i < args; i++) {
		uint16_t value = va_arg(va, int);
		values[j++] = value >> 8;
		values[j++] = value & 0xff;
	}
	va_end(va);

	g_dhcpv6_client_set_send(dhcp_client, G_DHCPV6_ORO, values, len);

	g_free(values);

	return 0;
}

static int send_dhcpv6_msg(GDHCPClient *dhcp_client, int type, char *msg)
{
	struct dhcpv6_packet *packet;
	uint8_t buf[MAX_DHCPV6_PKT_SIZE];
	unsigned char *ptr;
	int ret, max_buf;

	memset(buf, 0, sizeof(buf));
	packet = (struct dhcpv6_packet *)&buf[0];
	ptr = buf + sizeof(struct dhcpv6_packet);

	init_packet(dhcp_client, packet, type);

	if (dhcp_client->retransmit == FALSE) {
		dhcp_client->xid = packet->transaction_id[0] << 16 |
				packet->transaction_id[1] << 8 |
				packet->transaction_id[2];
		gettimeofday(&dhcp_client->start_time, NULL);
	} else {
		packet->transaction_id[0] = dhcp_client->xid >> 16;
		packet->transaction_id[1] = dhcp_client->xid >> 8 ;
		packet->transaction_id[2] = dhcp_client->xid;
	}

	g_dhcp_client_set_request(dhcp_client, G_DHCPV6_ELAPSED_TIME);

	debug(dhcp_client, "sending DHCPv6 %s message xid 0x%04x", msg,
							dhcp_client->xid);

	max_buf = MAX_DHCPV6_PKT_SIZE - sizeof(struct dhcpv6_packet);

	add_dhcpv6_request_options(dhcp_client, packet, buf, max_buf, &ptr);

	add_dhcpv6_send_options(dhcp_client, buf, max_buf, &ptr);

	ret = dhcpv6_send_packet(dhcp_client->ifindex, packet, ptr - buf);

	debug(dhcp_client, "sent %d pkt %p len %d", ret, packet, ptr - buf);
	return ret;
}

static int send_solicitation(GDHCPClient *dhcp_client)
{
	return send_dhcpv6_msg(dhcp_client, DHCPV6_SOLICIT, "solicit");
}

static int send_dhcpv6_request(GDHCPClient *dhcp_client)
{
	return send_dhcpv6_msg(dhcp_client, DHCPV6_REQUEST, "request");
}

static int send_dhcpv6_confirm(GDHCPClient *dhcp_client)
{
	return send_dhcpv6_msg(dhcp_client, DHCPV6_CONFIRM, "confirm");
}

static int send_dhcpv6_renew(GDHCPClient *dhcp_client)
{
	return send_dhcpv6_msg(dhcp_client, DHCPV6_RENEW, "renew");
}

static int send_dhcpv6_rebind(GDHCPClient *dhcp_client)
{
	return send_dhcpv6_msg(dhcp_client, DHCPV6_REBIND, "rebind");
}

static int send_dhcpv6_release(GDHCPClient *dhcp_client)
{
	return send_dhcpv6_msg(dhcp_client, DHCPV6_RELEASE, "release");
}

static int send_information_req(GDHCPClient *dhcp_client)
{
	return send_dhcpv6_msg(dhcp_client, DHCPV6_INFORMATION_REQ,
				"information-req");
}

static void remove_value(gpointer data, gpointer user_data)
{
	char *value = data;
	g_free(value);
}

static void remove_option_value(gpointer data)
{
	GList *option_value = data;

	g_list_foreach(option_value, remove_value, NULL);
}

GDHCPClient *g_dhcp_client_new(GDHCPType type,
			int ifindex, GDHCPClientError *error)
{
	GDHCPClient *dhcp_client;

	if (ifindex < 0) {
		*error = G_DHCP_CLIENT_ERROR_INVALID_INDEX;
		return NULL;
	}

	dhcp_client = g_try_new0(GDHCPClient, 1);
	if (dhcp_client == NULL) {
		*error = G_DHCP_CLIENT_ERROR_NOMEM;
		return NULL;
	}

	dhcp_client->interface = get_interface_name(ifindex);
	if (dhcp_client->interface == NULL) {
		*error = G_DHCP_CLIENT_ERROR_INTERFACE_UNAVAILABLE;
		goto error;
	}

	if (interface_is_up(ifindex) == FALSE) {
		*error = G_DHCP_CLIENT_ERROR_INTERFACE_DOWN;
		goto error;
	}

	get_interface_mac_address(ifindex, dhcp_client->mac_address);

	dhcp_client->listener_sockfd = -1;
	dhcp_client->listener_channel = NULL;
	dhcp_client->listen_mode = L_NONE;
	dhcp_client->ref_count = 1;
	dhcp_client->type = type;
	dhcp_client->ifindex = ifindex;
	dhcp_client->lease_available_cb = NULL;
	dhcp_client->ipv4ll_available_cb = NULL;
	dhcp_client->no_lease_cb = NULL;
	dhcp_client->lease_lost_cb = NULL;
	dhcp_client->ipv4ll_lost_cb = NULL;
	dhcp_client->address_conflict_cb = NULL;
	dhcp_client->listener_watch = 0;
	dhcp_client->retry_times = 0;
	dhcp_client->ack_retry_times = 0;
	dhcp_client->code_value_hash = g_hash_table_new_full(g_direct_hash,
				g_direct_equal, NULL, remove_option_value);
	dhcp_client->send_value_hash = g_hash_table_new_full(g_direct_hash,
				g_direct_equal, NULL, g_free);
	dhcp_client->request_list = NULL;
	dhcp_client->require_list = NULL;
	dhcp_client->duid = NULL;
	dhcp_client->duid_len = 0;
	dhcp_client->last_renew = dhcp_client->last_rebind = time(NULL);
	dhcp_client->expire = 0;

	*error = G_DHCP_CLIENT_ERROR_NONE;

	return dhcp_client;

error:
	g_free(dhcp_client->interface);
	g_free(dhcp_client);
	return NULL;
}

#define SERVER_AND_CLIENT_PORTS  ((67 << 16) + 68)

static int dhcp_l2_socket(int ifindex)
{
	int fd;
	struct sockaddr_ll sock;

	/*
	 * Comment:
	 *
	 *	I've selected not to see LL header, so BPF doesn't see it, too.
	 *	The filter may also pass non-IP and non-ARP packets, but we do
	 *	a more complete check when receiving the message in userspace.
	 *
	 * and filter shamelessly stolen from:
	 *
	 *	http://www.flamewarmaster.de/software/dhcpclient/
	 *
	 * There are a few other interesting ideas on that page (look under
	 * "Motivation").  Use of netlink events is most interesting.  Think
	 * of various network servers listening for events and reconfiguring.
	 * That would obsolete sending HUP signals and/or make use of restarts.
	 *
	 * Copyright: 2006, 2007 Stefan Rompf <sux@loplof.de>.
	 * License: GPL v2.
	 *
	 * TODO: make conditional?
	 */
	static const struct sock_filter filter_instr[] = {
		/* check for udp */
		BPF_STMT(BPF_LD|BPF_B|BPF_ABS, 9),
		/* L5, L1, is UDP? */
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, IPPROTO_UDP, 2, 0),
		/* ugly check for arp on ethernet-like and IPv4 */
		BPF_STMT(BPF_LD|BPF_W|BPF_ABS, 2), /* L1: */
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 0x08000604, 3, 4),/* L3, L4 */
		/* skip IP header */
		BPF_STMT(BPF_LDX|BPF_B|BPF_MSH, 0), /* L5: */
		/* check udp source and destination ports */
		BPF_STMT(BPF_LD|BPF_W|BPF_IND, 0),
		/* L3, L4 */
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, SERVER_AND_CLIENT_PORTS, 0, 1),
		/* returns */
		BPF_STMT(BPF_RET|BPF_K, 0x0fffffff), /* L3: pass */
		BPF_STMT(BPF_RET|BPF_K, 0), /* L4: reject */
	};

	static const struct sock_fprog filter_prog = {
		.len = sizeof(filter_instr) / sizeof(filter_instr[0]),
		/* casting const away: */
		.filter = (struct sock_filter *) filter_instr,
	};

	fd = socket(PF_PACKET, SOCK_DGRAM | SOCK_CLOEXEC, htons(ETH_P_IP));
	if (fd < 0)
		return -errno;

	if (SERVER_PORT == 67 && CLIENT_PORT == 68)
		/* Use only if standard ports are in use */
		setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter_prog,
							sizeof(filter_prog));

	memset(&sock, 0, sizeof(sock));
	sock.sll_family = AF_PACKET;
	sock.sll_protocol = htons(ETH_P_IP);
	sock.sll_ifindex = ifindex;

	if (bind(fd, (struct sockaddr *) &sock, sizeof(sock)) != 0) {
		int err = -errno;
		close(fd);
		return err;
	}

	return fd;
}

static gboolean sanity_check(struct ip_udp_dhcp_packet *packet, int bytes)
{
	if (packet->ip.protocol != IPPROTO_UDP)
		return FALSE;

	if (packet->ip.version != IPVERSION)
		return FALSE;

	if (packet->ip.ihl != sizeof(packet->ip) >> 2)
		return FALSE;

	if (packet->udp.dest != htons(CLIENT_PORT))
		return FALSE;

	if (ntohs(packet->udp.len) != (uint16_t)(bytes - sizeof(packet->ip)))
		return FALSE;

	return TRUE;
}

static int dhcp_recv_l2_packet(struct dhcp_packet *dhcp_pkt, int fd)
{
	int bytes;
	struct ip_udp_dhcp_packet packet;
	uint16_t check;

	memset(&packet, 0, sizeof(packet));

	bytes = read(fd, &packet, sizeof(packet));
	if (bytes < 0)
		return -1;

	if (bytes < (int) (sizeof(packet.ip) + sizeof(packet.udp)))
		return -1;

	if (bytes < ntohs(packet.ip.tot_len))
		/* packet is bigger than sizeof(packet), we did partial read */
		return -1;

	/* ignore any extra garbage bytes */
	bytes = ntohs(packet.ip.tot_len);

	if (sanity_check(&packet, bytes) == FALSE)
		return -1;

	check = packet.ip.check;
	packet.ip.check = 0;
	if (check != dhcp_checksum(&packet.ip, sizeof(packet.ip)))
		return -1;

	/* verify UDP checksum. IP header has to be modified for this */
	memset(&packet.ip, 0, offsetof(struct iphdr, protocol));
	/* ip.xx fields which are not memset: protocol, check, saddr, daddr */
	packet.ip.tot_len = packet.udp.len; /* yes, this is needed */
	check = packet.udp.check;
	packet.udp.check = 0;
	if (check && check != dhcp_checksum(&packet, bytes))
		return -1;

	memcpy(dhcp_pkt, &packet.data, bytes - (sizeof(packet.ip) +
							sizeof(packet.udp)));

	if (dhcp_pkt->cookie != htonl(DHCP_MAGIC))
		return -1;

	return bytes - (sizeof(packet.ip) + sizeof(packet.udp));
}

static void ipv4ll_start(GDHCPClient *dhcp_client)
{
	guint timeout;
	int seed;

	remove_timeouts(dhcp_client);

	switch_listening_mode(dhcp_client, L_NONE);
	dhcp_client->type = G_DHCP_IPV4LL;
	dhcp_client->retry_times = 0;
	dhcp_client->requested_ip = 0;

	/*try to start with a based mac address ip*/
	seed = (dhcp_client->mac_address[4] << 8 | dhcp_client->mac_address[4]);
	dhcp_client->requested_ip = ipv4ll_random_ip(seed);

	/*first wait a random delay to avoid storm of arp request on boot*/
	timeout = ipv4ll_random_delay_ms(PROBE_WAIT);

	dhcp_client->retry_times++;
	dhcp_client->timeout = g_timeout_add_full(G_PRIORITY_HIGH,
						timeout,
						send_probe_packet,
						dhcp_client,
						NULL);
}

static void ipv4ll_stop(GDHCPClient *dhcp_client)
{

	switch_listening_mode(dhcp_client, L_NONE);

	remove_timeouts(dhcp_client);

	if (dhcp_client->listener_watch > 0) {
		g_source_remove(dhcp_client->listener_watch);
		dhcp_client->listener_watch = 0;
	}

	dhcp_client->state = IPV4LL_PROBE;
	dhcp_client->retry_times = 0;
	dhcp_client->requested_ip = 0;

	g_free(dhcp_client->assigned_ip);
	dhcp_client->assigned_ip = NULL;
}

static int ipv4ll_recv_arp_packet(GDHCPClient *dhcp_client)
{
	int bytes;
	struct ether_arp arp;
	uint32_t ip_requested;
	int source_conflict;
	int target_conflict;

	memset(&arp, 0, sizeof(arp));
	bytes = read(dhcp_client->listener_sockfd, &arp, sizeof(arp));
	if (bytes < 0)
		return bytes;

	if (arp.arp_op != htons(ARPOP_REPLY) &&
			arp.arp_op != htons(ARPOP_REQUEST))
		return -EINVAL;

	ip_requested = htonl(dhcp_client->requested_ip);
	source_conflict = !memcmp(arp.arp_spa, &ip_requested,
						sizeof(ip_requested));

	target_conflict = !memcmp(arp.arp_tpa, &ip_requested,
				sizeof(ip_requested));

	if (!source_conflict && !target_conflict)
		return 0;

	dhcp_client->conflicts++;

	debug(dhcp_client, "IPV4LL conflict detected");

	if (dhcp_client->state == IPV4LL_MONITOR) {
		if (!source_conflict)
			return 0;
		dhcp_client->state = IPV4LL_DEFEND;
		debug(dhcp_client, "DEFEND mode conflicts : %d",
			dhcp_client->conflicts);
		/*Try to defend with a single announce*/
		send_announce_packet(dhcp_client);
		return 0;
	}

	if (dhcp_client->state == IPV4LL_DEFEND) {
		if (!source_conflict)
			return 0;
		else if (dhcp_client->ipv4ll_lost_cb != NULL)
			dhcp_client->ipv4ll_lost_cb(dhcp_client,
						dhcp_client->ipv4ll_lost_data);
	}

	ipv4ll_stop(dhcp_client);

	if (dhcp_client->conflicts < MAX_CONFLICTS) {
		/*restart whole state machine*/
		dhcp_client->retry_times++;
		dhcp_client->timeout =
			g_timeout_add_full(G_PRIORITY_HIGH,
					ipv4ll_random_delay_ms(PROBE_WAIT),
					send_probe_packet,
					dhcp_client,
					NULL);
	}
	/* Here we got a lot of conflicts, RFC3927 states that we have
	 * to wait RATE_LIMIT_INTERVAL before retrying,
	 * but we just report failure.
	 */
	else if (dhcp_client->no_lease_cb != NULL)
			dhcp_client->no_lease_cb(dhcp_client,
						dhcp_client->no_lease_data);

	return 0;
}

static gboolean check_package_owner(GDHCPClient *dhcp_client, gpointer pkt)
{
	if (dhcp_client->type == G_DHCP_IPV6) {
		struct dhcpv6_packet *packet6 = pkt;
		uint32_t xid;

		if (packet6 == NULL)
			return FALSE;

		xid = packet6->transaction_id[0] << 16 |
			packet6->transaction_id[1] << 8 |
			packet6->transaction_id[2];

		if (xid != dhcp_client->xid)
			return FALSE;
	} else {
		struct dhcp_packet *packet = pkt;

		if (packet->xid != dhcp_client->xid)
			return FALSE;

		if (packet->hlen != 6)
			return FALSE;

		if (memcmp(packet->chaddr, dhcp_client->mac_address, 6))
			return FALSE;
	}

	return TRUE;
}

static void start_request(GDHCPClient *dhcp_client);

static gboolean request_timeout(gpointer user_data)
{
	GDHCPClient *dhcp_client = user_data;

#if defined TIZEN_EXT
	if (dhcp_client->init_reboot == TRUE) {
		debug(dhcp_client, "DHCPREQUEST of INIT-REBOOT has failed");

		/* Start DHCPDISCOVERY when DHCPREQUEST of INIT-REBOOT has failed */
		g_dhcp_client_set_address_known(dhcp_client, FALSE);

		dhcp_client->retry_times = 0;
		dhcp_client->requested_ip = 0;

		g_dhcp_client_start(dhcp_client, dhcp_client->last_address);

		return FALSE;
	}
#endif
	debug(dhcp_client, "request timeout (retries %d)",
					dhcp_client->retry_times);

	dhcp_client->retry_times++;

	start_request(dhcp_client);

	return FALSE;
}

static gboolean listener_event(GIOChannel *channel, GIOCondition condition,
							gpointer user_data);

static int switch_listening_mode(GDHCPClient *dhcp_client,
					ListenMode listen_mode)
{
	GIOChannel *listener_channel;
	int listener_sockfd;

	if (dhcp_client->listen_mode == listen_mode)
		return 0;

	debug(dhcp_client, "switch listening mode (%d ==> %d)",
				dhcp_client->listen_mode, listen_mode);

	if (dhcp_client->listen_mode != L_NONE) {
		if (dhcp_client->listener_watch > 0)
			g_source_remove(dhcp_client->listener_watch);
		dhcp_client->listener_channel = NULL;
		dhcp_client->listen_mode = L_NONE;
		dhcp_client->listener_sockfd = -1;
		dhcp_client->listener_watch = 0;
	}

	if (listen_mode == L_NONE)
		return 0;

	if (listen_mode == L2)
		listener_sockfd = dhcp_l2_socket(dhcp_client->ifindex);
	else if (listen_mode == L3) {
		if (dhcp_client->type == G_DHCP_IPV6)
			listener_sockfd = dhcp_l3_socket(DHCPV6_CLIENT_PORT,
							dhcp_client->interface,
							AF_INET6);
		else
			listener_sockfd = dhcp_l3_socket(CLIENT_PORT,
							dhcp_client->interface,
							AF_INET);
	} else if (listen_mode == L_ARP)
		listener_sockfd = ipv4ll_arp_socket(dhcp_client->ifindex);
	else
		return -EIO;

	if (listener_sockfd < 0)
		return -EIO;

	listener_channel = g_io_channel_unix_new(listener_sockfd);
	if (listener_channel == NULL) {
		/* Failed to create listener channel */
		close(listener_sockfd);
		return -EIO;
	}

	dhcp_client->listen_mode = listen_mode;
	dhcp_client->listener_sockfd = listener_sockfd;
	dhcp_client->listener_channel = listener_channel;

	g_io_channel_set_close_on_unref(listener_channel, TRUE);
	dhcp_client->listener_watch =
			g_io_add_watch_full(listener_channel, G_PRIORITY_HIGH,
				G_IO_IN | G_IO_NVAL | G_IO_ERR | G_IO_HUP,
						listener_event, dhcp_client,
								NULL);
	g_io_channel_unref(dhcp_client->listener_channel);

	return 0;
}

static void start_request(GDHCPClient *dhcp_client)
{
	debug(dhcp_client, "start request (retries %d)",
					dhcp_client->retry_times);

	if (dhcp_client->retry_times == REQUEST_RETRIES) {
		dhcp_client->state = INIT_SELECTING;
		ipv4ll_start(dhcp_client);

		return;
	}

	if (dhcp_client->retry_times == 0) {
		dhcp_client->state = REQUESTING;
		switch_listening_mode(dhcp_client, L2);
	}

	send_select(dhcp_client);

	dhcp_client->timeout = g_timeout_add_seconds_full(G_PRIORITY_HIGH,
							REQUEST_TIMEOUT,
							request_timeout,
							dhcp_client,
							NULL);
}

static uint32_t get_lease(struct dhcp_packet *packet)
{
	uint8_t *option;
	uint32_t lease_seconds;

	option = dhcp_get_option(packet, DHCP_LEASE_TIME);
	if (option == NULL)
		return 3600;

	lease_seconds = get_be32(option);
	/* paranoia: must not be prone to overflows */
	lease_seconds &= 0x0fffffff;
	if (lease_seconds < 10)
		lease_seconds = 10;

	return lease_seconds;
}

static void restart_dhcp(GDHCPClient *dhcp_client, int retry_times)
{
	debug(dhcp_client, "restart DHCP (retries %d)", retry_times);

	remove_timeouts(dhcp_client);
#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
	if (dhcp_client->rtc_timeout > 0) {
		rtc_timeout_remove(dhcp_client->rtc_timeout);
		dhcp_client->rtc_timeout = 0;
	}
#endif

	dhcp_client->retry_times = retry_times;
	dhcp_client->requested_ip = 0;
	dhcp_client->state = INIT_SELECTING;
	switch_listening_mode(dhcp_client, L2);

	g_dhcp_client_start(dhcp_client, dhcp_client->last_address);
}

static gboolean start_rebound_timeout(gpointer user_data)
{
	GDHCPClient *dhcp_client = user_data;

	debug(dhcp_client, "start rebound timeout");

	switch_listening_mode(dhcp_client, L2);

	dhcp_client->lease_seconds >>= 1;

	/* We need to have enough time to receive ACK package*/
	if (dhcp_client->lease_seconds <= 6) {

		/* ip need to be cleared */
		if (dhcp_client->lease_lost_cb != NULL)
			dhcp_client->lease_lost_cb(dhcp_client,
					dhcp_client->lease_lost_data);

		restart_dhcp(dhcp_client, 0);
	} else {
		send_rebound(dhcp_client);

#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
		dhcp_client->rtc_timeout = rtc_timeout_add_seconds(
				dhcp_client->lease_seconds >> 1,
				start_rebound_timeout,
				dhcp_client,
				NULL);
#else
		dhcp_client->timeout =
				g_timeout_add_seconds_full(G_PRIORITY_HIGH,
						dhcp_client->lease_seconds >> 1,
							start_rebound_timeout,
								dhcp_client,
								NULL);
#endif
	}

	return FALSE;
}

static void start_rebound(GDHCPClient *dhcp_client)
{
	debug(dhcp_client, "start rebound");

	dhcp_client->state = REBINDING;

#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
	dhcp_client->rtc_timeout = rtc_timeout_add_seconds(
			dhcp_client->lease_seconds >> 1,
			start_rebound_timeout,
			dhcp_client,
			NULL);
#else
	dhcp_client->timeout = g_timeout_add_seconds_full(G_PRIORITY_HIGH,
						dhcp_client->lease_seconds >> 1,
							start_rebound_timeout,
								dhcp_client,
								NULL);
#endif
}

static gboolean start_renew_timeout(gpointer user_data)
{
	GDHCPClient *dhcp_client = user_data;

	debug(dhcp_client, "start renew timeout");

	dhcp_client->state = RENEWING;

	dhcp_client->lease_seconds >>= 1;

	switch_listening_mode(dhcp_client, L3);
	if (dhcp_client->lease_seconds <= 60)
		start_rebound(dhcp_client);
	else {
		send_renew(dhcp_client);

		if (dhcp_client->timeout > 0)
			g_source_remove(dhcp_client->timeout);
#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
		dhcp_client->timeout = 0;

		if (dhcp_client->rtc_timeout > 0)
			rtc_timeout_remove(dhcp_client->rtc_timeout);
		dhcp_client->rtc_timeout = 0;
#endif

#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
		dhcp_client->rtc_timeout = rtc_timeout_add_seconds(
				dhcp_client->lease_seconds >> 1,
				start_renew_timeout,
				dhcp_client,
				NULL);
#else
		dhcp_client->timeout =
				g_timeout_add_seconds_full(G_PRIORITY_HIGH,
						dhcp_client->lease_seconds >> 1,
							start_renew_timeout,
								dhcp_client,
								NULL);
#endif
	}

	return FALSE;
}

static void start_bound(GDHCPClient *dhcp_client)
{
	debug(dhcp_client, "start bound");

	dhcp_client->state = BOUND;

	if (dhcp_client->timeout > 0)
		g_source_remove(dhcp_client->timeout);
#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
	dhcp_client->timeout = 0;

	if (dhcp_client->rtc_timeout > 0)
		rtc_timeout_remove(dhcp_client->rtc_timeout);
	dhcp_client->rtc_timeout = 0;
#endif

#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
	dhcp_client->rtc_timeout = rtc_timeout_add_seconds(
			dhcp_client->lease_seconds >> 1,
			start_renew_timeout,
			dhcp_client,
			NULL);
#else
	dhcp_client->timeout = g_timeout_add_seconds_full(G_PRIORITY_HIGH,
					dhcp_client->lease_seconds >> 1,
					start_renew_timeout, dhcp_client,
							NULL);
#endif
}

static gboolean restart_dhcp_timeout(gpointer user_data)
{
	GDHCPClient *dhcp_client = user_data;

	debug(dhcp_client, "restart DHCP timeout");

	dhcp_client->ack_retry_times++;

	restart_dhcp(dhcp_client, dhcp_client->ack_retry_times);

	return FALSE;
}

static char *get_ip(uint32_t ip)
{
	struct in_addr addr;

	addr.s_addr = ip;

	return g_strdup(inet_ntoa(addr));
}

/* get a rough idea of how long an option will be */
static const uint8_t len_of_option_as_string[] = {
	[OPTION_IP] = sizeof("255.255.255.255 "),
	[OPTION_STRING] = 1,
	[OPTION_U8] = sizeof("255 "),
	[OPTION_U16] = sizeof("65535 "),
	[OPTION_U32] = sizeof("4294967295 "),
};

static int sprint_nip(char *dest, const char *pre, const uint8_t *ip)
{
	return sprintf(dest, "%s%u.%u.%u.%u", pre, ip[0], ip[1], ip[2], ip[3]);
}

/* Create "opt_value1 option_value2 ..." string */
static char *malloc_option_value_string(uint8_t *option, GDHCPOptionType type)
{
	unsigned upper_length;
	int len, optlen;
	char *dest, *ret;

	len = option[OPT_LEN - OPT_DATA];
	type &= OPTION_TYPE_MASK;
	optlen = dhcp_option_lengths[type];
	if (optlen == 0)
		return NULL;
	upper_length = len_of_option_as_string[type] *
			((unsigned)len / (unsigned)optlen);
	dest = ret = g_malloc(upper_length + 1);
	if (ret == NULL)
		return NULL;

	while (len >= optlen) {
		switch (type) {
		case OPTION_IP:
			dest += sprint_nip(dest, "", option);
			break;
		case OPTION_U16: {
			uint16_t val_u16 = get_be16(option);
			dest += sprintf(dest, "%u", val_u16);
			break;
		}
		case OPTION_U32: {
			uint32_t val_u32 = get_be32(option);
			dest += sprintf(dest, "%u", val_u32);
			break;
		}
		case OPTION_STRING:
			memcpy(dest, option, len);
			dest[len] = '\0';
			return ret;
		default:
			break;
		}
		option += optlen;
		len -= optlen;
		if (len <= 0)
			break;
		*dest++ = ' ';
		*dest = '\0';
	}

	return ret;
}

static GList *get_option_value_list(char *value, GDHCPOptionType type)
{
	char *pos = value;
	GList *list = NULL;

	if (pos == NULL)
		return NULL;

	if (type == OPTION_STRING)
		return g_list_append(list, g_strdup(value));

	while ((pos = strchr(pos, ' ')) != NULL) {
		*pos = '\0';

		list = g_list_append(list, g_strdup(value));

		value = ++pos;
	}

	list = g_list_append(list, g_strdup(value));

	return list;
}

static inline uint32_t get_uint32(unsigned char *value)
{
	return value[0] << 24 | value[1] << 16 |
		value[2] << 8 | value[3];
}

static inline uint16_t get_uint16(unsigned char *value)
{
	return value[0] << 8 | value[1];
}

static GList *get_addresses(GDHCPClient *dhcp_client,
				int code, int len,
				unsigned char *value,
				uint16_t *status)
{
	GList *list = NULL;
	struct in6_addr addr;
	uint32_t iaid, T1 = 0, T2 = 0, preferred = 0, valid = 0;
	uint16_t option_len, option_code, st = 0, max_len;
	int addr_count = 0, i, pos;
	uint8_t *option;
	char *str;

	if (value == NULL || len < 4)
		return NULL;

	iaid = get_uint32(&value[0]);
	if (dhcp_client->iaid != iaid)
		return NULL;

	if (code == G_DHCPV6_IA_NA) {
		T1 = get_uint32(&value[4]);
		T2 = get_uint32(&value[8]);

		if (T1 > T2)
			/* RFC 3315, 22.4 */
			return NULL;

		pos = 12;
	} else
		pos = 4;

	if (len <= pos)
		return NULL;

	max_len = len - pos;

	/* We have more sub-options in this packet. */
	do {
		option = dhcpv6_get_sub_option(&value[pos], max_len,
					&option_code, &option_len);

		debug(dhcp_client, "pos %d option %p code %d len %d",
			pos, option, option_code, option_len);

		if (option == NULL)
			break;

		if (pos >= max_len)
			break;

		switch (option_code) {
		case G_DHCPV6_IAADDR:
			i = 0;
			memcpy(&addr, &option[0], sizeof(addr));
			i += sizeof(addr);
			preferred = get_uint32(&option[i]);
			i += 4;
			valid = get_uint32(&option[i]);

			addr_count++;
			break;

		case G_DHCPV6_STATUS_CODE:
			st = get_uint16(&option[0]);
			debug(dhcp_client, "error code %d", st);
			if (option_len > 2) {
				str = g_strndup((gchar *)&option[2],
						option_len - 2);
				debug(dhcp_client, "error text: %s", str);
				g_free(str);
			}

			*status = st;
			break;
		}

		pos += 2 + 2 + option_len;

	} while (option != NULL);

	if (addr_count > 0 && st == 0) {
		/* We only support one address atm */
		char addr_str[INET6_ADDRSTRLEN + 1];

		if (preferred > valid)
			/* RFC 3315, 22.6 */
			return NULL;

		dhcp_client->T1 = T1;
		dhcp_client->T2 = T2;

		inet_ntop(AF_INET6, &addr, addr_str, INET6_ADDRSTRLEN);
		debug(dhcp_client, "count %d addr %s T1 %u T2 %u",
			addr_count, addr_str, T1, T2);

		list = g_list_append(list, g_strdup(addr_str));

		if (code == G_DHCPV6_IA_NA)
			memcpy(&dhcp_client->ia_na, &addr,
						sizeof(struct in6_addr));
		else
			memcpy(&dhcp_client->ia_ta, &addr,
						sizeof(struct in6_addr));

		g_dhcpv6_client_set_expire(dhcp_client, valid);
	}

	return list;
}

static GList *get_dhcpv6_option_value_list(GDHCPClient *dhcp_client,
					int code, int len,
					unsigned char *value,
					uint16_t *status)
{
	GList *list = NULL;
	char *str;
	int i;

	if (value == NULL)
		return NULL;

	switch (code) {
	case G_DHCPV6_DNS_SERVERS:	/* RFC 3646, chapter 3 */
	case G_DHCPV6_SNTP_SERVERS:	/* RFC 4075, chapter 4 */
		if (len % 16) {
			debug(dhcp_client,
				"%s server list length (%d) is invalid",
				code == G_DHCPV6_DNS_SERVERS ? "DNS" : "SNTP",
				len);
			return NULL;
		}
		for (i = 0; i < len; i += 16) {

			str = g_try_malloc0(INET6_ADDRSTRLEN+1);
			if (str == NULL)
				return list;

			if (inet_ntop(AF_INET6, &value[i], str,
					INET6_ADDRSTRLEN) == NULL)
				g_free(str);
			else
				list = g_list_append(list, str);
		}
		break;

	case G_DHCPV6_IA_NA:		/* RFC 3315, chapter 22.4 */
	case G_DHCPV6_IA_TA:		/* RFC 3315, chapter 22.5 */
		list = get_addresses(dhcp_client, code, len, value, status);
		break;

	default:
		break;
	}

	return list;
}

static void get_dhcpv6_request(GDHCPClient *dhcp_client,
				struct dhcpv6_packet *packet,
				uint16_t pkt_len, uint16_t *status)
{
	GList *list, *value_list;
	uint8_t *option;
	uint16_t code;
	uint16_t option_len;

	for (list = dhcp_client->request_list; list; list = list->next) {
		code = (uint16_t) GPOINTER_TO_INT(list->data);

		option = dhcpv6_get_option(packet, pkt_len, code, &option_len,
						NULL);
		if (option == NULL) {
			g_hash_table_remove(dhcp_client->code_value_hash,
						GINT_TO_POINTER((int) code));
			continue;
		}

		value_list = get_dhcpv6_option_value_list(dhcp_client, code,
						option_len, option, status);

		debug(dhcp_client, "code %d %p len %d list %p", code, option,
			option_len, value_list);

		if (value_list == NULL)
			g_hash_table_remove(dhcp_client->code_value_hash,
						GINT_TO_POINTER((int) code));
		else
			g_hash_table_insert(dhcp_client->code_value_hash,
				GINT_TO_POINTER((int) code), value_list);
	}
}

static void get_request(GDHCPClient *dhcp_client, struct dhcp_packet *packet)
{
	GDHCPOptionType type;
	GList *list, *value_list;
	char *option_value;
	uint8_t *option;
	uint8_t code;

	for (list = dhcp_client->request_list; list; list = list->next) {
		code = (uint8_t) GPOINTER_TO_INT(list->data);

		option = dhcp_get_option(packet, code);
		if (option == NULL) {
			g_hash_table_remove(dhcp_client->code_value_hash,
						GINT_TO_POINTER((int) code));
			continue;
		}

		type =  dhcp_get_code_type(code);

		option_value = malloc_option_value_string(option, type);
		if (option_value == NULL)
			g_hash_table_remove(dhcp_client->code_value_hash,
						GINT_TO_POINTER((int) code));

		value_list = get_option_value_list(option_value, type);

		g_free(option_value);

		if (value_list == NULL)
			g_hash_table_remove(dhcp_client->code_value_hash,
						GINT_TO_POINTER((int) code));
		else
			g_hash_table_insert(dhcp_client->code_value_hash,
				GINT_TO_POINTER((int) code), value_list);
	}
}

static gboolean listener_event(GIOChannel *channel, GIOCondition condition,
							gpointer user_data)
{
	GDHCPClient *dhcp_client = user_data;
	struct dhcp_packet packet;
	struct dhcpv6_packet *packet6 = NULL;
	uint8_t *message_type = NULL, *client_id = NULL, *option,
		*server_id = NULL;
	uint16_t option_len = 0, status = 0;
	uint32_t xid = 0;
	gpointer pkt;
	unsigned char buf[MAX_DHCPV6_PKT_SIZE];
	uint16_t pkt_len = 0;
	int count;
	int re;

	if (condition & (G_IO_NVAL | G_IO_ERR | G_IO_HUP)) {
		dhcp_client->listener_watch = 0;
		return FALSE;
	}

	if (dhcp_client->listen_mode == L_NONE)
		return FALSE;

	pkt = &packet;

	dhcp_client->status_code = 0;

	if (dhcp_client->listen_mode == L2)
		re = dhcp_recv_l2_packet(&packet,
					dhcp_client->listener_sockfd);
	else if (dhcp_client->listen_mode == L3) {
		if (dhcp_client->type == G_DHCP_IPV6) {
			re = dhcpv6_recv_l3_packet(&packet6, buf, sizeof(buf),
						dhcp_client->listener_sockfd);
			pkt_len = re;
			pkt = packet6;
			xid = packet6->transaction_id[0] << 16 |
				packet6->transaction_id[1] << 8 |
				packet6->transaction_id[2];
		} else {
			re = dhcp_recv_l3_packet(&packet,
						dhcp_client->listener_sockfd);
			xid = packet.xid;
		}
	} else if (dhcp_client->listen_mode == L_ARP) {
		ipv4ll_recv_arp_packet(dhcp_client);
		return TRUE;
	}
	else
		re = -EIO;

	if (re < 0)
		return TRUE;

	if (check_package_owner(dhcp_client, pkt) == FALSE)
		return TRUE;

	if (dhcp_client->type == G_DHCP_IPV6) {
		if (packet6 == NULL)
			return TRUE;

		count = 0;
		client_id = dhcpv6_get_option(packet6, pkt_len,
				G_DHCPV6_CLIENTID, &option_len,	&count);

		if (client_id == NULL || count == 0 || option_len == 0 ||
				memcmp(dhcp_client->duid, client_id,
					dhcp_client->duid_len) != 0) {
			debug(dhcp_client,
				"client duid error, discarding msg %p/%d/%d",
				client_id, option_len, count);
			return TRUE;
		}

		option = dhcpv6_get_option(packet6, pkt_len,
				G_DHCPV6_STATUS_CODE, &option_len, NULL);
		if (option != 0 && option_len > 0) {
			status = option[0]<<8 | option[1];
			if (status != 0) {
				debug(dhcp_client, "error code %d", status);
				if (option_len > 2) {
					gchar *txt = g_strndup(
						(gchar *)&option[2],
						option_len - 2);
					debug(dhcp_client, "error text: %s",
						txt);
					g_free(txt);
				}
			}
			dhcp_client->status_code = status;
		}
	} else {
		message_type = dhcp_get_option(&packet, DHCP_MESSAGE_TYPE);
		if (message_type == NULL)
			return TRUE;
	}

	if (message_type == NULL && client_id == NULL)
		/* No message type / client id option, ignore package */
		return TRUE;

	debug(dhcp_client, "received DHCP packet xid 0x%04x "
			"(current state %d)", xid, dhcp_client->state);

	switch (dhcp_client->state) {
	case INIT_SELECTING:
		if (*message_type != DHCPOFFER)
			return TRUE;

		remove_timeouts(dhcp_client);
		dhcp_client->timeout = 0;
		dhcp_client->retry_times = 0;

		option = dhcp_get_option(&packet, DHCP_SERVER_ID);
		dhcp_client->server_ip = get_be32(option);
		dhcp_client->requested_ip = ntohl(packet.yiaddr);

		dhcp_client->state = REQUESTING;

		start_request(dhcp_client);

		return TRUE;
	case REQUESTING:
	case RENEWING:
	case REBINDING:
		if (*message_type == DHCPACK) {
			dhcp_client->retry_times = 0;

			remove_timeouts(dhcp_client);
#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
			if (dhcp_client->rtc_timeout > 0)
				rtc_timeout_remove(dhcp_client->rtc_timeout);
			dhcp_client->rtc_timeout = 0;
#endif

			dhcp_client->lease_seconds = get_lease(&packet);

#if defined TIZEN_EXT
			option = dhcp_get_option(&packet, DHCP_SERVER_ID);
			if (option)
				dhcp_client->server_ip = get_be32(option);

			debug(dhcp_client, "lease %d secs", dhcp_client->lease_seconds);
#endif
			get_request(dhcp_client, &packet);

			switch_listening_mode(dhcp_client, L_NONE);

			g_free(dhcp_client->assigned_ip);
			dhcp_client->assigned_ip = get_ip(packet.yiaddr);

			/* Address should be set up here */
			if (dhcp_client->lease_available_cb != NULL)
				dhcp_client->lease_available_cb(dhcp_client,
					dhcp_client->lease_available_data);

			start_bound(dhcp_client);
		} else if (*message_type == DHCPNAK) {
			dhcp_client->retry_times = 0;

			remove_timeouts(dhcp_client);

#if defined TIZEN_EXT
			if (dhcp_client->init_reboot == TRUE) {
				g_dhcp_client_set_address_known(dhcp_client, FALSE);
				dhcp_client->timeout = g_idle_add_full(
								G_PRIORITY_HIGH,
								restart_dhcp_timeout,
								dhcp_client,
								NULL);

				break;
			}
#endif
			dhcp_client->timeout = g_timeout_add_seconds_full(
							G_PRIORITY_HIGH, 3,
							restart_dhcp_timeout,
							dhcp_client,
							NULL);
		}

		break;
	case SOLICITATION:
		if (dhcp_client->type != G_DHCP_IPV6)
			return TRUE;

		if (packet6->message != DHCPV6_REPLY &&
				packet6->message != DHCPV6_ADVERTISE)
			return TRUE;

		count = 0;
		server_id = dhcpv6_get_option(packet6, pkt_len,
				G_DHCPV6_SERVERID, &option_len,	&count);
		if (server_id == NULL || count != 1 || option_len == 0) {
			/* RFC 3315, 15.10 */
			debug(dhcp_client,
				"server duid error, discarding msg %p/%d/%d",
				server_id, option_len, count);
			return TRUE;
		}
		dhcp_client->server_duid = g_try_malloc(option_len);
		if (dhcp_client->server_duid == NULL)
			return TRUE;
		memcpy(dhcp_client->server_duid, server_id, option_len);
		dhcp_client->server_duid_len = option_len;

		if (packet6->message == DHCPV6_REPLY) {
			uint8_t *rapid_commit;
			count = 0;
			option_len = 0;
			rapid_commit = dhcpv6_get_option(packet6, pkt_len,
							G_DHCPV6_RAPID_COMMIT,
							&option_len, &count);
			if (rapid_commit == NULL || option_len == 0 ||
								count != 1)
				/* RFC 3315, 17.1.4 */
				return TRUE;
		}

		switch_listening_mode(dhcp_client, L_NONE);

		if (dhcp_client->status_code == 0)
			get_dhcpv6_request(dhcp_client, packet6, pkt_len,
					&dhcp_client->status_code);

		if (packet6->message == DHCPV6_ADVERTISE) {
			if (dhcp_client->advertise_cb != NULL)
				dhcp_client->advertise_cb(dhcp_client,
						dhcp_client->advertise_data);
			return TRUE;
		}

		if (dhcp_client->solicitation_cb != NULL) {
			/*
			 * The dhcp_client might not be valid after the
			 * callback call so just return immediately.
			 */
			dhcp_client->solicitation_cb(dhcp_client,
					dhcp_client->solicitation_data);
			return TRUE;
		}
		break;
	case INFORMATION_REQ:
	case REQUEST:
	case RENEW:
	case REBIND:
	case RELEASE:
	case CONFIRM:
		if (dhcp_client->type != G_DHCP_IPV6)
			return TRUE;

		if (packet6->message != DHCPV6_REPLY)
			return TRUE;

		count = 0;
		option_len = 0;
		server_id = dhcpv6_get_option(packet6, pkt_len,
				G_DHCPV6_SERVERID, &option_len, &count);
		if (server_id == NULL || count != 1 || option_len == 0 ||
				(dhcp_client->server_duid_len > 0 &&
				memcmp(dhcp_client->server_duid, server_id,
					dhcp_client->server_duid_len) != 0)) {
			/* RFC 3315, 15.10 */
			debug(dhcp_client,
				"server duid error, discarding msg %p/%d/%d",
				server_id, option_len, count);
			return TRUE;
		}

		switch_listening_mode(dhcp_client, L_NONE);

		get_dhcpv6_request(dhcp_client, packet6, pkt_len,
						&dhcp_client->status_code);

		if (dhcp_client->information_req_cb != NULL) {
			/*
			 * The dhcp_client might not be valid after the
			 * callback call so just return immediately.
			 */
			dhcp_client->information_req_cb(dhcp_client,
					dhcp_client->information_req_data);
			return TRUE;
		}
		if (dhcp_client->request_cb != NULL) {
			dhcp_client->request_cb(dhcp_client,
					dhcp_client->request_data);
			return TRUE;
		}
		if (dhcp_client->renew_cb != NULL) {
			dhcp_client->renew_cb(dhcp_client,
					dhcp_client->renew_data);
			return TRUE;
		}
		if (dhcp_client->rebind_cb != NULL) {
			dhcp_client->rebind_cb(dhcp_client,
					dhcp_client->rebind_data);
			return TRUE;
		}
		if (dhcp_client->release_cb != NULL) {
			dhcp_client->release_cb(dhcp_client,
					dhcp_client->release_data);
			return TRUE;
		}
		if (dhcp_client->confirm_cb != NULL) {
			count = 0;
			server_id = dhcpv6_get_option(packet6, pkt_len,
						G_DHCPV6_SERVERID, &option_len,
						&count);
			if (server_id == NULL || count != 1 ||
							option_len == 0) {
				/* RFC 3315, 15.10 */
				debug(dhcp_client,
					"confirm server duid error, "
					"discarding msg %p/%d/%d",
					server_id, option_len, count);
				return TRUE;
			}
			dhcp_client->server_duid = g_try_malloc(option_len);
			if (dhcp_client->server_duid == NULL)
				return TRUE;
			memcpy(dhcp_client->server_duid, server_id, option_len);
			dhcp_client->server_duid_len = option_len;

			dhcp_client->confirm_cb(dhcp_client,
						dhcp_client->confirm_data);
			return TRUE;
		}
		break;
	default:
		break;
	}

	debug(dhcp_client, "processed DHCP packet (new state %d)",
							dhcp_client->state);

	return TRUE;
}

static gboolean discover_timeout(gpointer user_data)
{
	GDHCPClient *dhcp_client = user_data;

	dhcp_client->retry_times++;

	/*
	 * We do not send the REQUESTED IP option if we are retrying because
	 * if the server is non-authoritative it will ignore the request if the
	 * option is present.
	 */
	g_dhcp_client_start(dhcp_client, NULL);

	return FALSE;
}

static gboolean ipv4ll_defend_timeout(gpointer dhcp_data)
{
	GDHCPClient *dhcp_client = dhcp_data;

	debug(dhcp_client, "back to MONITOR mode");

	dhcp_client->conflicts = 0;
	dhcp_client->state = IPV4LL_MONITOR;

	return FALSE;
}

static gboolean ipv4ll_announce_timeout(gpointer dhcp_data)
{
	GDHCPClient *dhcp_client = dhcp_data;
	uint32_t ip;

#if defined TIZEN_EXT
	if (!dhcp_client)
		return FALSE;
#endif

	debug(dhcp_client, "request timeout (retries %d)",
	       dhcp_client->retry_times);

	if (dhcp_client->retry_times != ANNOUNCE_NUM){
		dhcp_client->retry_times++;
		send_announce_packet(dhcp_client);
		return FALSE;
	}

	ip = htonl(dhcp_client->requested_ip);
	debug(dhcp_client, "switching to monitor mode");
	dhcp_client->state = IPV4LL_MONITOR;
	dhcp_client->assigned_ip = get_ip(ip);

	if (dhcp_client->ipv4ll_available_cb != NULL)
		dhcp_client->ipv4ll_available_cb(dhcp_client,
					dhcp_client->ipv4ll_available_data);
	dhcp_client->conflicts = 0;

	return FALSE;
}

static gboolean ipv4ll_probe_timeout(gpointer dhcp_data)
{

	GDHCPClient *dhcp_client = dhcp_data;

	debug(dhcp_client, "IPV4LL probe timeout (retries %d)",
	       dhcp_client->retry_times);

	if (dhcp_client->retry_times == PROBE_NUM) {
		dhcp_client->state = IPV4LL_ANNOUNCE;
		dhcp_client->retry_times = 0;

		dhcp_client->retry_times++;
		send_announce_packet(dhcp_client);
		return FALSE;
	}
	dhcp_client->retry_times++;
	send_probe_packet(dhcp_client);

	return FALSE;
}

int g_dhcp_client_start(GDHCPClient *dhcp_client, const char *last_address)
{
	int re;
	uint32_t addr;

	if (dhcp_client->type == G_DHCP_IPV6) {
		if (dhcp_client->information_req_cb) {
			dhcp_client->state = INFORMATION_REQ;
			re = switch_listening_mode(dhcp_client, L3);
			if (re != 0) {
				switch_listening_mode(dhcp_client, L_NONE);
				dhcp_client->state = 0;
				return re;
			}
			send_information_req(dhcp_client);

		} else if (dhcp_client->solicitation_cb) {
			dhcp_client->state = SOLICITATION;
			re = switch_listening_mode(dhcp_client, L3);
			if (re != 0) {
				switch_listening_mode(dhcp_client, L_NONE);
				dhcp_client->state = 0;
				return re;
			}
			send_solicitation(dhcp_client);

		} else if (dhcp_client->request_cb) {
			dhcp_client->state = REQUEST;
			re = switch_listening_mode(dhcp_client, L3);
			if (re != 0) {
				switch_listening_mode(dhcp_client, L_NONE);
				dhcp_client->state = 0;
				return re;
			}
			send_dhcpv6_request(dhcp_client);

		} else if (dhcp_client->confirm_cb) {
			dhcp_client->state = CONFIRM;
			re = switch_listening_mode(dhcp_client, L3);
			if (re != 0) {
				switch_listening_mode(dhcp_client, L_NONE);
				dhcp_client->state = 0;
				return re;
			}
			send_dhcpv6_confirm(dhcp_client);

		} else if (dhcp_client->renew_cb) {
			dhcp_client->state = RENEW;
			re = switch_listening_mode(dhcp_client, L3);
			if (re != 0) {
				switch_listening_mode(dhcp_client, L_NONE);
				dhcp_client->state = 0;
				return re;
			}
			send_dhcpv6_renew(dhcp_client);

		} else if (dhcp_client->rebind_cb) {
			dhcp_client->state = REBIND;
			re = switch_listening_mode(dhcp_client, L3);
			if (re != 0) {
				switch_listening_mode(dhcp_client, L_NONE);
				dhcp_client->state = 0;
				return re;
			}
			send_dhcpv6_rebind(dhcp_client);

		} else if (dhcp_client->release_cb) {
			dhcp_client->state = RENEW;
			re = switch_listening_mode(dhcp_client, L3);
			if (re != 0) {
				switch_listening_mode(dhcp_client, L_NONE);
				dhcp_client->state = 0;
				return re;
			}
			send_dhcpv6_release(dhcp_client);
		}

		return 0;
	}

	if (dhcp_client->retry_times == DISCOVER_RETRIES) {
		ipv4ll_start(dhcp_client);
		return 0;
	}

	if (dhcp_client->retry_times == 0) {
		g_free(dhcp_client->assigned_ip);
		dhcp_client->assigned_ip = NULL;

		dhcp_client->state = INIT_SELECTING;
		re = switch_listening_mode(dhcp_client, L2);
		if (re != 0)
			return re;

		dhcp_client->xid = rand();
		dhcp_client->start = time(NULL);
	}

	if (last_address == NULL) {
		addr = 0;
	} else {
		addr = ntohl(inet_addr(last_address));
		if (addr == 0xFFFFFFFF) {
			addr = 0;
#if defined TIZEN_EXT
		} else if (dhcp_client->last_address != last_address) {
#else
		} else {
#endif
			g_free(dhcp_client->last_address);
			dhcp_client->last_address = g_strdup(last_address);
		}
	}
#if defined TIZEN_EXT
	if (dhcp_client->init_reboot == TRUE) {
		dhcp_client->requested_ip = addr;

		start_request(dhcp_client);

		return 0;
	}
#endif
	send_discover(dhcp_client, addr);

	dhcp_client->timeout = g_timeout_add_seconds_full(G_PRIORITY_HIGH,
							DISCOVER_TIMEOUT,
							discover_timeout,
							dhcp_client,
							NULL);
	return 0;
}

void g_dhcp_client_stop(GDHCPClient *dhcp_client)
{
	switch_listening_mode(dhcp_client, L_NONE);

	if (dhcp_client->state == BOUND ||
			dhcp_client->state == RENEWING ||
				dhcp_client->state == REBINDING)
		send_release(dhcp_client, dhcp_client->server_ip,
					dhcp_client->requested_ip);

	remove_timeouts(dhcp_client);
#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
	if (dhcp_client->rtc_timeout > 0) {
		rtc_timeout_remove(dhcp_client->rtc_timeout);
		dhcp_client->rtc_timeout = 0;
	}
#endif

	if (dhcp_client->listener_watch > 0) {
		g_source_remove(dhcp_client->listener_watch);
		dhcp_client->listener_watch = 0;
	}

	dhcp_client->listener_channel = NULL;

	dhcp_client->retry_times = 0;
	dhcp_client->ack_retry_times = 0;

	dhcp_client->requested_ip = 0;
	dhcp_client->state = RELEASED;
	dhcp_client->lease_seconds = 0;
}

GList *g_dhcp_client_get_option(GDHCPClient *dhcp_client,
					unsigned char option_code)
{
	return g_hash_table_lookup(dhcp_client->code_value_hash,
					GINT_TO_POINTER((int) option_code));
}

void g_dhcp_client_register_event(GDHCPClient *dhcp_client,
					GDHCPClientEvent event,
					GDHCPClientEventFunc func,
							gpointer data)
{
	switch (event) {
	case G_DHCP_CLIENT_EVENT_LEASE_AVAILABLE:
		dhcp_client->lease_available_cb = func;
		dhcp_client->lease_available_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_IPV4LL_AVAILABLE:
		if (dhcp_client->type == G_DHCP_IPV6)
			return;
		dhcp_client->ipv4ll_available_cb = func;
		dhcp_client->ipv4ll_available_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_NO_LEASE:
		dhcp_client->no_lease_cb = func;
		dhcp_client->no_lease_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_LEASE_LOST:
		dhcp_client->lease_lost_cb = func;
		dhcp_client->lease_lost_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_IPV4LL_LOST:
		if (dhcp_client->type == G_DHCP_IPV6)
			return;
		dhcp_client->ipv4ll_lost_cb = func;
		dhcp_client->ipv4ll_lost_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_ADDRESS_CONFLICT:
		dhcp_client->address_conflict_cb = func;
		dhcp_client->address_conflict_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_INFORMATION_REQ:
		if (dhcp_client->type == G_DHCP_IPV4)
			return;
		dhcp_client->information_req_cb = func;
		dhcp_client->information_req_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_SOLICITATION:
		if (dhcp_client->type == G_DHCP_IPV4)
			return;
		dhcp_client->solicitation_cb = func;
		dhcp_client->solicitation_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_ADVERTISE:
		if (dhcp_client->type == G_DHCP_IPV4)
			return;
		dhcp_client->advertise_cb = func;
		dhcp_client->advertise_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_REQUEST:
		if (dhcp_client->type == G_DHCP_IPV4)
			return;
		dhcp_client->request_cb = func;
		dhcp_client->request_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_RENEW:
		if (dhcp_client->type == G_DHCP_IPV4)
			return;
		dhcp_client->renew_cb = func;
		dhcp_client->renew_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_REBIND:
		if (dhcp_client->type == G_DHCP_IPV4)
			return;
		dhcp_client->rebind_cb = func;
		dhcp_client->rebind_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_RELEASE:
		if (dhcp_client->type == G_DHCP_IPV4)
			return;
		dhcp_client->release_cb = func;
		dhcp_client->release_data = data;
		return;
	case G_DHCP_CLIENT_EVENT_CONFIRM:
		if (dhcp_client->type == G_DHCP_IPV4)
			return;
		dhcp_client->confirm_cb = func;
		dhcp_client->confirm_data = data;
		return;
	}
}

int g_dhcp_client_get_index(GDHCPClient *dhcp_client)
{
	return dhcp_client->ifindex;
}

char *g_dhcp_client_get_address(GDHCPClient *dhcp_client)
{
	return g_strdup(dhcp_client->assigned_ip);
}

char *g_dhcp_client_get_netmask(GDHCPClient *dhcp_client)
{
	GList *option = NULL;

	if (dhcp_client->type == G_DHCP_IPV6)
		return NULL;

	switch (dhcp_client->state) {
	case IPV4LL_DEFEND:
	case IPV4LL_MONITOR:
		return g_strdup("255.255.0.0");
	case BOUND:
	case RENEWING:
	case REBINDING:
		option = g_dhcp_client_get_option(dhcp_client, G_DHCP_SUBNET);
		if (option != NULL)
			return g_strdup(option->data);
	case INIT_SELECTING:
	case REQUESTING:
	case RELEASED:
	case IPV4LL_PROBE:
	case IPV4LL_ANNOUNCE:
	case INFORMATION_REQ:
	case SOLICITATION:
	case REQUEST:
	case CONFIRM:
	case RENEW:
	case REBIND:
	case RELEASE:
		break;
	}
	return NULL;
}

GDHCPClientError g_dhcp_client_set_request(GDHCPClient *dhcp_client,
						unsigned int option_code)
{
	if (g_list_find(dhcp_client->request_list,
			GINT_TO_POINTER((int) option_code)) == NULL)
		dhcp_client->request_list = g_list_prepend(
					dhcp_client->request_list,
					(GINT_TO_POINTER((int) option_code)));

	return G_DHCP_CLIENT_ERROR_NONE;
}

void g_dhcp_client_clear_requests(GDHCPClient *dhcp_client)
{
	g_list_free(dhcp_client->request_list);
	dhcp_client->request_list = NULL;
}

void g_dhcp_client_clear_values(GDHCPClient *dhcp_client)
{
	g_hash_table_remove_all(dhcp_client->send_value_hash);
}

static uint8_t *alloc_dhcp_option(int code, const uint8_t *data, unsigned size)
{
	uint8_t *storage;

	storage = g_try_malloc(size + OPT_DATA);
	if (storage == NULL)
		return NULL;

	storage[OPT_CODE] = code;
	storage[OPT_LEN] = size;
	memcpy(&storage[OPT_DATA], data, size);

	return storage;
}

static uint8_t *alloc_dhcp_data_option(int code, const uint8_t *data, unsigned size)
{
	return alloc_dhcp_option(code, data, MIN(size, 255));
}

static uint8_t *alloc_dhcp_string_option(int code, const char *str)
{
	return alloc_dhcp_data_option(code, (const uint8_t *)str, strlen(str));
}

GDHCPClientError g_dhcp_client_set_id(GDHCPClient *dhcp_client)
{
	const unsigned maclen = 6;
	const unsigned idlen = maclen + 1;
	const uint8_t option_code = G_DHCP_CLIENT_ID;
	uint8_t idbuf[idlen];
	uint8_t *data_option;

	idbuf[0] = ARPHRD_ETHER;

	memcpy(&idbuf[1], dhcp_client->mac_address, maclen);

	data_option = alloc_dhcp_data_option(option_code, idbuf, idlen);
	if (data_option == NULL)
		return G_DHCP_CLIENT_ERROR_NOMEM;

	g_hash_table_insert(dhcp_client->send_value_hash,
		GINT_TO_POINTER((int) option_code), data_option);

	return G_DHCP_CLIENT_ERROR_NONE;
}

/* Now only support send hostname */
GDHCPClientError g_dhcp_client_set_send(GDHCPClient *dhcp_client,
		unsigned char option_code, const char *option_value)
{
	uint8_t *binary_option;

	if (option_code == G_DHCP_HOST_NAME && option_value != NULL) {
		binary_option = alloc_dhcp_string_option(option_code,
							option_value);
		if (binary_option == NULL)
			return G_DHCP_CLIENT_ERROR_NOMEM;

		g_hash_table_insert(dhcp_client->send_value_hash,
			GINT_TO_POINTER((int) option_code), binary_option);
	}

	return G_DHCP_CLIENT_ERROR_NONE;
}

static uint8_t *alloc_dhcpv6_option(uint16_t code, uint8_t *option,
				uint16_t len)
{
	uint8_t *storage;

	storage = g_malloc(2 + 2 + len);
	if (storage == NULL)
		return NULL;

	storage[0] = code >> 8;
	storage[1] = code & 0xff;
	storage[2] = len >> 8;
	storage[3] = len & 0xff;
	memcpy(storage + 2 + 2, option, len);

	return storage;
}

void g_dhcpv6_client_set_send(GDHCPClient *dhcp_client,
					uint16_t option_code,
					uint8_t *option_value,
					uint16_t option_len)
{
	if (option_value != NULL) {
		uint8_t *binary_option;

		debug(dhcp_client, "setting option %d to %p len %d",
			option_code, option_value, option_len);

		binary_option = alloc_dhcpv6_option(option_code, option_value,
						option_len);
		if (binary_option != NULL)
			g_hash_table_insert(dhcp_client->send_value_hash,
					GINT_TO_POINTER((int) option_code),
					binary_option);
	}
}

void g_dhcpv6_client_reset_renew(GDHCPClient *dhcp_client)
{
	if (dhcp_client == NULL || dhcp_client->type == G_DHCP_IPV4)
		return;

	dhcp_client->last_renew = time(NULL);
}

void g_dhcpv6_client_reset_rebind(GDHCPClient *dhcp_client)
{
	if (dhcp_client == NULL || dhcp_client->type == G_DHCP_IPV4)
		return;

	dhcp_client->last_rebind = time(NULL);
}

void g_dhcpv6_client_set_expire(GDHCPClient *dhcp_client, uint32_t timeout)
{
	if (dhcp_client == NULL || dhcp_client->type == G_DHCP_IPV4)
		return;

	dhcp_client->expire = time(NULL) + timeout;
}

uint16_t g_dhcpv6_client_get_status(GDHCPClient *dhcp_client)
{
	if (dhcp_client == NULL || dhcp_client->type == G_DHCP_IPV4)
		return 0;

	return dhcp_client->status_code;
}

GDHCPClient *g_dhcp_client_ref(GDHCPClient *dhcp_client)
{
	if (dhcp_client == NULL)
		return NULL;

	__sync_fetch_and_add(&dhcp_client->ref_count, 1);

	return dhcp_client;
}

void g_dhcp_client_unref(GDHCPClient *dhcp_client)
{
	if (dhcp_client == NULL)
		return;

	if (__sync_fetch_and_sub(&dhcp_client->ref_count, 1) != 1)
		return;

	g_dhcp_client_stop(dhcp_client);

	g_free(dhcp_client->interface);
	g_free(dhcp_client->assigned_ip);
	g_free(dhcp_client->last_address);
	g_free(dhcp_client->duid);
	g_free(dhcp_client->server_duid);

	g_list_free(dhcp_client->request_list);
	g_list_free(dhcp_client->require_list);

	g_hash_table_destroy(dhcp_client->code_value_hash);
	g_hash_table_destroy(dhcp_client->send_value_hash);

	g_free(dhcp_client);
#if defined TIZEN_EXT
	dhcp_client = NULL;
#endif
}

void g_dhcp_client_set_debug(GDHCPClient *dhcp_client,
				GDHCPDebugFunc func, gpointer user_data)
{
	if (dhcp_client == NULL)
		return;

	dhcp_client->debug_func = func;
	dhcp_client->debug_data = user_data;
}

#if defined TIZEN_EXT
void g_dhcp_client_set_address_known(GDHCPClient *dhcp_client, gboolean known)
{
	/* DHCPREQUEST during INIT-REBOOT state (rfc2131)
	 * 4.4.3 Initialization with known network address
	 * 4.3.2 DHCPREQUEST generated during INIT-REBOOT state
	 */
	debug(dhcp_client, "known network address (%d)", known);

	if (dhcp_client->init_reboot == known)
		return;

	dhcp_client->init_reboot = known;
}

void g_dhcp_client_start_renew(GDHCPClient *dhcp_client)
{
	start_renew_timeout((gpointer) dhcp_client);
}
#endif
