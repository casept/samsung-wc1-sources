/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011-2014  Intel Corporation
 *  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "src/shared/mainloop.h"

#include "packet.h"
#include "lmp.h"
#include "keys.h"
#include "analyze.h"
#include "ellisys.h"
#include "control.h"

static void signal_callback(int signum, void *user_data)
{
	switch (signum) {
	case SIGINT:
	case SIGTERM:
		mainloop_quit();
		break;
	}
}

static void usage(void)
{
	printf("btmon - Bluetooth monitor\n"
		"Usage:\n");
	printf("\tbtmon [options]\n");
	printf("options:\n"
		"\t-r, --read <file>      Read traces in btsnoop format\n"
		"\t-w, --write <file>     Save traces in btsnoop format\n"
		"\t-a, --analyze <file>   Analyze traces in btsnoop format\n"
		"\t-s, --server <socket>  Start monitor server socket\n"
		"\t-i, --index <num>      Show only specified controller\n"
		"\t-t, --time             Show time instead of time offset\n"
		"\t-T, --date             Show time and date information\n"
		"\t-S, --sco              Dump SCO traffic\n"
		"\t-E, --ellisys [ip]     Send Ellisys HCI Injection\n"
#ifdef __TIZEN_PATCH__
		"\t-C, --count <num>      Save traces by <num> rotation\n"
		"\t-W, --size <num>       Save traces at most <num> size\n"
#endif
		"\t-h, --help             Show help options\n");
}

static const struct option main_options[] = {
	{ "read",    required_argument, NULL, 'r' },
	{ "write",   required_argument, NULL, 'w' },
	{ "analyze", required_argument, NULL, 'a' },
	{ "server",  required_argument, NULL, 's' },
	{ "index",   required_argument, NULL, 'i' },
	{ "time",    no_argument,       NULL, 't' },
	{ "date",    no_argument,       NULL, 'T' },
	{ "sco",     no_argument,	NULL, 'S' },
	{ "ellisys", required_argument, NULL, 'E' },
#ifdef __TIZEN_PATCH__
	{ "count",   required_argument, NULL, 'C' },
	{ "size",    required_argument, NULL, 'W' },
#endif
	{ "todo",    no_argument,       NULL, '#' },
	{ "version", no_argument,       NULL, 'v' },
	{ "help",    no_argument,       NULL, 'h' },
	{ }
};

int main(int argc, char *argv[])
{
	unsigned long filter_mask = 0;
	const char *reader_path = NULL;
	const char *writer_path = NULL;
	const char *analyze_path = NULL;
	const char *ellisys_server = NULL;
	unsigned short ellisys_port = 0;
#ifdef __TIZEN_PATCH__
	int16_t rotate_count = -1;
	ssize_t file_size = -1;
#endif
	const char *str;
	int exit_status;
	sigset_t mask;

	mainloop_init();

	filter_mask |= PACKET_FILTER_SHOW_TIME_OFFSET;

	for (;;) {
		int opt;

#ifdef __TIZEN_PATCH__
		opt = getopt_long(argc, argv, "r:w:a:s:i:tTSE:C:W:vh",
						main_options, NULL);
#else
		opt = getopt_long(argc, argv, "r:w:a:s:i:tTSE:vh",
						main_options, NULL);
#endif
		if (opt < 0)
			break;

		switch (opt) {
		case 'r':
			reader_path = optarg;
			break;
		case 'w':
			writer_path = optarg;
			break;
		case 'a':
			analyze_path = optarg;
			break;
		case 's':
			control_server(optarg);
			break;
		case 'i':
			if (strlen(optarg) > 3 && !strncmp(optarg, "hci", 3))
				str = optarg + 3;
			else
				str = optarg;
			if (!isdigit(*str)) {
				usage();
				return EXIT_FAILURE;
			}
			packet_select_index(atoi(str));
			break;
		case 't':
			filter_mask &= ~PACKET_FILTER_SHOW_TIME_OFFSET;
			filter_mask |= PACKET_FILTER_SHOW_TIME;
			break;
		case 'T':
			filter_mask &= ~PACKET_FILTER_SHOW_TIME_OFFSET;
			filter_mask |= PACKET_FILTER_SHOW_TIME;
			filter_mask |= PACKET_FILTER_SHOW_DATE;
			break;
		case 'S':
			filter_mask |= PACKET_FILTER_SHOW_SCO_DATA;
			break;
		case 'E':
			ellisys_server = optarg;
			ellisys_port = 24352;
			break;
#ifdef __TIZEN_PATCH__
		case 'C':
			rotate_count = atoi(optarg);
			break;
		case 'W':
			file_size = atoll(optarg);
			break;
#endif
		case '#':
			packet_todo();
			lmp_todo();
			return EXIT_SUCCESS;
		case 'v':
			printf("%s\n", VERSION);
			return EXIT_SUCCESS;
		case 'h':
			usage();
			return EXIT_SUCCESS;
		default:
			return EXIT_FAILURE;
		}
	}

	if (argc - optind > 0) {
		fprintf(stderr, "Invalid command line parameters\n");
		return EXIT_FAILURE;
	}

	if (reader_path && analyze_path) {
		fprintf(stderr, "Display and analyze can't be combined\n");
		return EXIT_FAILURE;
	}

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);

	mainloop_set_signal(&mask, signal_callback, NULL, NULL);

	printf("Bluetooth monitor ver %s\n", VERSION);

	keys_setup();

	packet_set_filter(filter_mask);

	if (analyze_path) {
		analyze_trace(analyze_path);
		return EXIT_SUCCESS;
	}

	if (reader_path) {
		if (ellisys_server)
			ellisys_enable(ellisys_server, ellisys_port);

		control_reader(reader_path);
		return EXIT_SUCCESS;
	}

#ifdef __TIZEN_PATCH__
	if (writer_path && !control_writer(writer_path,
				rotate_count, file_size)) {
		printf("Failed to open '%s'\n", writer_path);
		return EXIT_FAILURE;
	}
#else
	if (writer_path && !control_writer(writer_path)) {
		printf("Failed to open '%s'\n", writer_path);
		return EXIT_FAILURE;
	}
#endif

	if (ellisys_server)
		ellisys_enable(ellisys_server, ellisys_port);

	if (control_tracing() < 0)
		return EXIT_FAILURE;

	exit_status = mainloop_run();

	keys_cleanup();

	return exit_status;
}
