/*
 *
 *  Connection Manager
 *
 *  Copyright (C) 2007-2012  Intel Corporation. All rights reserved.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#if !defined TIZEN_EXT
#include <signal.h>
#include <sys/signalfd.h>
#endif
#include <getopt.h>
#include <sys/stat.h>
#include <net/if.h>
#include <netdb.h>

#include <gdbus.h>

#include "connman.h"

#define DEFAULT_INPUT_REQUEST_TIMEOUT 120 * 1000
#define DEFAULT_BROWSER_LAUNCH_TIMEOUT 300 * 1000

static struct {
	connman_bool_t bg_scan;
	char **pref_timeservers;
	unsigned int *auto_connect;
	unsigned int *preferred_techs;
	char **fallback_nameservers;
	unsigned int timeout_inputreq;
	unsigned int timeout_browserlaunch;
	char **blacklisted_interfaces;
	connman_bool_t single_tech;
#if defined TIZEN_EXT
	char **cellular_interfaces;
	char **online_accessibility;
	unsigned int *wifi_pairing_priority;
	char **preferred_network_names;
	connman_bool_t wifi_dhcp_release;
#endif
} connman_settings  = {
	.bg_scan = TRUE,
	.pref_timeservers = NULL,
	.auto_connect = NULL,
	.preferred_techs = NULL,
	.fallback_nameservers = NULL,
	.timeout_inputreq = DEFAULT_INPUT_REQUEST_TIMEOUT,
	.timeout_browserlaunch = DEFAULT_BROWSER_LAUNCH_TIMEOUT,
	.blacklisted_interfaces = NULL,
#if defined TIZEN_EXT
	.single_tech = TRUE,
	.online_accessibility = NULL,
	.wifi_pairing_priority = NULL,
	.preferred_network_names = NULL,
	.wifi_dhcp_release = FALSE,
#else
	.single_tech = FALSE,
#endif
};

static GKeyFile *load_config(const char *file)
{
	GError *err = NULL;
	GKeyFile *keyfile;

	keyfile = g_key_file_new();

	g_key_file_set_list_separator(keyfile, ',');

	if (!g_key_file_load_from_file(keyfile, file, 0, &err)) {
		if (err->code != G_FILE_ERROR_NOENT) {
			connman_error("Parsing %s failed: %s", file,
								err->message);
		}

		g_error_free(err);
		g_key_file_free(keyfile);
		return NULL;
	}

	return keyfile;
}

static uint *parse_service_types(char **str_list, gsize len)
{
	unsigned int *type_list;
	int i, j;
	enum connman_service_type type;

	type_list = g_try_new0(unsigned int, len + 1);
	if (type_list == NULL)
		return NULL;

	i = 0;
	j = 0;
	while (str_list[i] != NULL)
	{
		type = __connman_service_string2type(str_list[i]);

		if (type != CONNMAN_SERVICE_TYPE_UNKNOWN) {
			type_list[j] = type;
			j += 1;
		}
		i += 1;
	}

	return type_list;
}

static char **parse_fallback_nameservers(char **nameservers, gsize len)
{
	char **servers;
	int i, j;

	servers = g_try_new0(char *, len + 1);
	if (servers == NULL)
		return NULL;

	i = 0;
	j = 0;
	while (nameservers[i] != NULL) {
		if (connman_inet_check_ipaddress(nameservers[i]) > 0) {
			servers[j] = g_strdup(nameservers[i]);
			j += 1;
		}
		i += 1;
	}

	return servers;
}

#if defined TIZEN_EXT
#define TIZEN_CUSTOM_SETTING "/opt/system/csc-default/usr"

static enum connman_service_security convert_wifi_security(const char *security)
{
	if (security == NULL)
		return CONNMAN_SERVICE_SECURITY_UNKNOWN;
	else if (g_str_equal(security, "none") == TRUE)
		return CONNMAN_SERVICE_SECURITY_NONE;
	else if (g_str_equal(security, "wep") == TRUE)
		return CONNMAN_SERVICE_SECURITY_WEP;
	else if (g_str_equal(security, "psk") == TRUE)
		return CONNMAN_SERVICE_SECURITY_PSK;
	else if (g_str_equal(security, "ieee8021x") == TRUE)
		return CONNMAN_SERVICE_SECURITY_8021X;
	else
		return CONNMAN_SERVICE_SECURITY_UNKNOWN;
}

static uint *parse_wifi_security_types(char **str_list, gsize len)
{
	unsigned int *type_list;
	int i, j;
	enum connman_service_security type;

	type_list = g_try_new0(unsigned int, len + 1);
	if (type_list == NULL)
		return NULL;

	i = 0;
	j = 0;
	while (str_list[i] != NULL)
	{
		type = convert_wifi_security(str_list[i]);

		if (type != CONNMAN_SERVICE_SECURITY_UNKNOWN) {
			type_list[j] = type;
			j += 1;
		}
		i += 1;
	}

	return type_list;
}

static void check_Tizen_configuration(GKeyFile *config)
{
	GError *error = NULL;
	gboolean boolean;
	char **cellular_interfaces;
	char **online_accessibility;
	char **str_list;
	gsize len;

	cellular_interfaces = g_key_file_get_string_list(config, "General",
			"NetworkCellularInterfaceList", &len, &error);

	if (error == NULL)
		connman_settings.cellular_interfaces = cellular_interfaces;

	g_clear_error(&error);

	online_accessibility = g_key_file_get_string_list(config, "General",
			"OnlineAccessibility", &len, &error);

	if (error == NULL)
		connman_settings.online_accessibility = online_accessibility;

	g_clear_error(&error);

	str_list = g_key_file_get_string_list(config, "General",
			"WifiPairingPriority", &len, &error);

	if (error == NULL)
		connman_settings.wifi_pairing_priority =
			parse_wifi_security_types(str_list, len);

	g_strfreev(str_list);

	g_clear_error(&error);

	str_list = g_key_file_get_string_list(config, "General",
			"PreferredNetworkNames", &len, &error);

	if (error == NULL)
		connman_settings.preferred_network_names = str_list;

	g_clear_error(&error);

	boolean = g_key_file_get_boolean(config, "General",
			"WifiDhcpRelease", &error);

	if (error == NULL)
		connman_settings.wifi_dhcp_release = boolean;

	g_clear_error(&error);
}
#endif

static void parse_config(GKeyFile *config)
{
	GError *error = NULL;
	gboolean boolean;
	char **timeservers;
	char **interfaces;
	char **str_list;
	gsize len;
	static char *default_auto_connect[] = {
		"wifi",
		"ethernet",
		"cellular",
		NULL
	};
	static char *default_blacklist[] = {
		"vmnet",
		"vboxnet",
		"virbr",
		NULL
	};
	int timeout;

	if (config == NULL) {
		connman_settings.auto_connect =
			parse_service_types(default_auto_connect, 3);
		connman_settings.blacklisted_interfaces = default_blacklist;
		return;
	}

	DBG("parsing main.conf");

	boolean = g_key_file_get_boolean(config, "General",
						"BackgroundScanning", &error);
	if (error == NULL)
		connman_settings.bg_scan = boolean;

	g_clear_error(&error);

	timeservers = g_key_file_get_string_list(config, "General",
						"FallbackTimeservers", NULL, &error);
	if (error == NULL)
		connman_settings.pref_timeservers = timeservers;

	g_clear_error(&error);

	str_list = g_key_file_get_string_list(config, "General",
			"DefaultAutoConnectTechnologies", &len, &error);

	if (error == NULL)
		connman_settings.auto_connect =
			parse_service_types(str_list, len);
	else
		connman_settings.auto_connect =
			parse_service_types(default_auto_connect, 3);

	g_strfreev(str_list);

	g_clear_error(&error);

	str_list = g_key_file_get_string_list(config, "General",
			"PreferredTechnologies", &len, &error);

	if (error == NULL)
		connman_settings.preferred_techs =
			parse_service_types(str_list, len);

	g_strfreev(str_list);

	g_clear_error(&error);

	str_list = g_key_file_get_string_list(config, "General",
			"FallbackNameservers", &len, &error);

	if (error == NULL)
		connman_settings.fallback_nameservers =
			parse_fallback_nameservers(str_list, len);

	g_strfreev(str_list);

	g_clear_error(&error);

	timeout = g_key_file_get_integer(config, "General",
			"InputRequestTimeout", &error);
	if (error == NULL && timeout >= 0)
		connman_settings.timeout_inputreq = timeout * 1000;

	g_clear_error(&error);

	timeout = g_key_file_get_integer(config, "General",
			"BrowserLaunchTimeout", &error);
	if (error == NULL && timeout >= 0)
		connman_settings.timeout_browserlaunch = timeout * 1000;

	g_clear_error(&error);

	interfaces = g_key_file_get_string_list(config, "General",
			"NetworkInterfaceBlacklist", &len, &error);

	if (error == NULL)
		connman_settings.blacklisted_interfaces = interfaces;
	else
		connman_settings.blacklisted_interfaces = default_blacklist;

	g_clear_error(&error);

	boolean = g_key_file_get_boolean(config, "General",
			"SingleConnectedTechnology", &error);
	if (error == NULL)
		connman_settings.single_tech = boolean;

	g_clear_error(&error);

#if defined TIZEN_EXT
	check_Tizen_configuration(config);
#endif
}

static GMainLoop *main_loop = NULL;

#if !defined TIZEN_EXT
static unsigned int __terminated = 0;

static gboolean signal_handler(GIOChannel *channel, GIOCondition cond,
							gpointer user_data)
{
	struct signalfd_siginfo si;
	ssize_t result;
	int fd;

	if (cond & (G_IO_NVAL | G_IO_ERR | G_IO_HUP))
		return FALSE;

	fd = g_io_channel_unix_get_fd(channel);

	result = read(fd, &si, sizeof(si));
	if (result != sizeof(si))
		return FALSE;

	switch (si.ssi_signo) {
	case SIGINT:
	case SIGTERM:
		if (__terminated == 0) {
			connman_info("Terminating");
			g_main_loop_quit(main_loop);
		}

		__terminated = 1;
		break;
	}

	return TRUE;
}

static guint setup_signalfd(void)
{
	GIOChannel *channel;
	guint source;
	sigset_t mask;
	int fd;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
		perror("Failed to set signal mask");
		return 0;
	}

	fd = signalfd(-1, &mask, 0);
	if (fd < 0) {
		perror("Failed to create signal descriptor");
		return 0;
	}

	channel = g_io_channel_unix_new(fd);

	g_io_channel_set_close_on_unref(channel, TRUE);
	g_io_channel_set_encoding(channel, NULL, NULL);
	g_io_channel_set_buffered(channel, FALSE);

	source = g_io_add_watch(channel,
				G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
				signal_handler, NULL);

	g_io_channel_unref(channel);

	return source;
}
#endif

static void disconnect_callback(DBusConnection *conn, void *user_data)
{
	connman_error("D-Bus disconnect");

	g_main_loop_quit(main_loop);
}

static gchar *option_debug = NULL;
static gchar *option_device = NULL;
static gchar *option_plugin = NULL;
static gchar *option_nodevice = NULL;
static gchar *option_noplugin = NULL;
static gchar *option_wifi = NULL;
static gboolean option_detach = TRUE;
static gboolean option_dnsproxy = TRUE;
static gboolean option_compat = FALSE;
static gboolean option_version = FALSE;

static gboolean parse_debug(const char *key, const char *value,
					gpointer user_data, GError **error)
{
	if (value)
		option_debug = g_strdup(value);
	else
		option_debug = g_strdup("*");

	return TRUE;
}

static GOptionEntry options[] = {
	{ "debug", 'd', G_OPTION_FLAG_OPTIONAL_ARG,
				G_OPTION_ARG_CALLBACK, parse_debug,
				"Specify debug options to enable", "DEBUG" },
	{ "device", 'i', 0, G_OPTION_ARG_STRING, &option_device,
			"Specify networking device or interface", "DEV" },
	{ "nodevice", 'I', 0, G_OPTION_ARG_STRING, &option_nodevice,
			"Specify networking interface to ignore", "DEV" },
	{ "plugin", 'p', 0, G_OPTION_ARG_STRING, &option_plugin,
				"Specify plugins to load", "NAME,..." },
	{ "noplugin", 'P', 0, G_OPTION_ARG_STRING, &option_noplugin,
				"Specify plugins not to load", "NAME,..." },
	{ "wifi", 'W', 0, G_OPTION_ARG_STRING, &option_wifi,
				"Specify driver for WiFi/Supplicant", "NAME" },
	{ "nodaemon", 'n', G_OPTION_FLAG_REVERSE,
				G_OPTION_ARG_NONE, &option_detach,
				"Don't fork daemon to background" },
	{ "nodnsproxy", 'r', G_OPTION_FLAG_REVERSE,
				G_OPTION_ARG_NONE, &option_dnsproxy,
				"Don't enable DNS Proxy" },
	{ "compat", 'c', 0, G_OPTION_ARG_NONE, &option_compat,
				"(obsolete)" },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &option_version,
				"Show version information and exit" },
	{ NULL },
};

const char *connman_option_get_string(const char *key)
{
	if (g_strcmp0(key, "wifi") == 0) {
		if (option_wifi == NULL)
			return "nl80211,wext";
		else
			return option_wifi;
	}

	return NULL;
}

connman_bool_t connman_setting_get_bool(const char *key)
{
	if (g_str_equal(key, "BackgroundScanning") == TRUE)
		return connman_settings.bg_scan;

	if (g_str_equal(key, "SingleConnectedTechnology") == TRUE)
		return connman_settings.single_tech;

#if defined TIZEN_EXT
	if (g_str_equal(key, "WiFiDHCPRelease") == TRUE)
		return connman_settings.wifi_dhcp_release;
#endif
	return FALSE;
}

char **connman_setting_get_string_list(const char *key)
{
	if (g_str_equal(key, "FallbackTimeservers") == TRUE)
		return connman_settings.pref_timeservers;

	if (g_str_equal(key, "FallbackNameservers") == TRUE)
		return connman_settings.fallback_nameservers;

	if (g_str_equal(key, "NetworkInterfaceBlacklist") == TRUE)
		return connman_settings.blacklisted_interfaces;

#if defined TIZEN_EXT
	if (g_str_equal(key, "NetworkCellularInterfaceList") == TRUE)
		return connman_settings.cellular_interfaces;

	if (g_str_equal(key, "PreferredNetworkNames") == TRUE)
		return connman_settings.preferred_network_names;
#endif
	return NULL;
}

unsigned int *connman_setting_get_uint_list(const char *key)
{
	if (g_str_equal(key, "DefaultAutoConnectTechnologies") == TRUE)
		return connman_settings.auto_connect;

	if (g_str_equal(key, "PreferredTechnologies") == TRUE)
		return connman_settings.preferred_techs;

#if defined TIZEN_EXT
	if (g_str_equal(key, "WiFiPairingPriority") == TRUE)
		return connman_settings.wifi_pairing_priority;
#endif
	return NULL;
}

unsigned int connman_timeout_input_request(void) {
	return connman_settings.timeout_inputreq;
}

unsigned int connman_timeout_browser_launch(void) {
	return connman_settings.timeout_browserlaunch;
}

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	DBusConnection *conn;
	DBusError err;
	GKeyFile *config;
#if !defined TIZEN_EXT
	guint signal;
#endif

#ifdef NEED_THREADS
#if !GLIB_CHECK_VERSION(2,32,0)
	if (g_thread_supported() == FALSE)
		g_thread_init(NULL);
#endif
#endif

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, options, NULL);

	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE) {
		if (error != NULL) {
			g_printerr("%s\n", error->message);
			g_error_free(error);
		} else
			g_printerr("An unknown error occurred\n");
		exit(1);
	}

	g_option_context_free(context);

	if (option_version == TRUE) {
		printf("%s\n", VERSION);
		exit(0);
	}

	if (option_detach == TRUE) {
		if (daemon(0, 0)) {
			perror("Can't start daemon");
			exit(1);
		}
	}

	if (mkdir(STATEDIR, S_IRUSR | S_IWUSR | S_IXUSR |
				S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0) {
		if (errno != EEXIST)
			perror("Failed to create state directory");
	}

	if (mkdir(STORAGEDIR, S_IRUSR | S_IWUSR | S_IXUSR |
				S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0) {
		if (errno != EEXIST)
			perror("Failed to create storage directory");
	}

	umask(0077);

	main_loop = g_main_loop_new(NULL, FALSE);

#ifdef NEED_THREADS
	if (dbus_threads_init_default() == FALSE) {
		fprintf(stderr, "Can't init usage of threads\n");
		exit(1);
	}
#endif

#if !defined TIZEN_EXT
	signal = setup_signalfd();
#endif

	dbus_error_init(&err);

	conn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, CONNMAN_SERVICE, &err);
	if (conn == NULL) {
		if (dbus_error_is_set(&err) == TRUE) {
			fprintf(stderr, "%s\n", err.message);
			dbus_error_free(&err);
		} else
			fprintf(stderr, "Can't register with system bus\n");
		exit(1);
	}

	g_dbus_set_disconnect_function(conn, disconnect_callback, NULL, NULL);

	__connman_log_init(argv[0], option_debug, option_detach);

	__connman_dbus_init(conn);

#if defined TIZEN_EXT
	config = load_config(TIZEN_CUSTOM_SETTING "/wifi/main.conf");
	if (config == NULL)
#endif
	config = load_config(CONFIGDIR "/main.conf");
	parse_config(config);
	if (config != NULL)
		g_key_file_free(config);

	__connman_technology_init();
	__connman_notifier_init();
	__connman_service_init();
	__connman_provider_init();
	__connman_network_init();
	__connman_device_init(option_device, option_nodevice);

	__connman_agent_init();
	__connman_ippool_init();
	__connman_iptables_init();
	__connman_nat_init();
	__connman_tethering_init();
	__connman_counter_init();
	__connman_manager_init();
	__connman_config_init();
	__connman_stats_init();
	__connman_clock_init();

	__connman_resolver_init(option_dnsproxy);
	__connman_ipconfig_init();
	__connman_rtnl_init();
	__connman_task_init();
	__connman_proxy_init();
	__connman_detect_init();
	__connman_session_init();
#if !defined TIZEN_EXT || defined TIZEN_CONNMAN_NTP
	__connman_timeserver_init();
#endif
	__connman_connection_init();

	__connman_plugin_init(option_plugin, option_noplugin);

	__connman_rtnl_start();
#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
	__connman_rtctimer_init();
#endif
	__connman_dhcp_init();
	__connman_dhcpv6_init();
	__connman_wpad_init();
	__connman_wispr_init();
	__connman_rfkill_init();

	g_free(option_device);
	g_free(option_plugin);
	g_free(option_nodevice);
	g_free(option_noplugin);

	g_main_loop_run(main_loop);

#if !defined TIZEN_EXT
	g_source_remove(signal);
#endif

	__connman_rfkill_cleanup();
	__connman_wispr_cleanup();
	__connman_wpad_cleanup();
	__connman_dhcpv6_cleanup();
	__connman_dhcp_cleanup();
#if defined TIZEN_EXT && defined TIZEN_RTC_TIMER
	__connman_rtctimer_cleanup();
#endif
	__connman_plugin_cleanup();
	__connman_provider_cleanup();
	__connman_connection_cleanup();
#if !defined TIZEN_EXT || defined TIZEN_CONNMAN_NTP
	__connman_timeserver_cleanup();
#endif
	__connman_session_cleanup();
	__connman_detect_cleanup();
	__connman_proxy_cleanup();
	__connman_task_cleanup();
	__connman_rtnl_cleanup();
	__connman_resolver_cleanup();

	__connman_clock_cleanup();
	__connman_stats_cleanup();
	__connman_config_cleanup();
	__connman_manager_cleanup();
	__connman_counter_cleanup();
	__connman_agent_cleanup();
	__connman_tethering_cleanup();
	__connman_nat_cleanup();
	__connman_iptables_cleanup();
	__connman_ippool_cleanup();
	__connman_device_cleanup();
	__connman_network_cleanup();
	__connman_service_cleanup();
	__connman_ipconfig_cleanup();
	__connman_notifier_cleanup();
	__connman_technology_cleanup();

	__connman_dbus_cleanup();

	__connman_log_cleanup();

	dbus_connection_unref(conn);

	g_main_loop_unref(main_loop);

	if (connman_settings.pref_timeservers != NULL)
		g_strfreev(connman_settings.pref_timeservers);

	g_free(connman_settings.auto_connect);
	g_free(connman_settings.preferred_techs);
	g_strfreev(connman_settings.fallback_nameservers);

	g_free(option_debug);

	return 0;
}
