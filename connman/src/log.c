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

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <execinfo.h>
#include <dlfcn.h>

#include "connman.h"

static const char *program_exec;
static const char *program_path;

#if defined TIZEN_EXT
#include <sys/stat.h>
#include <sys/time.h>

#define LOG_FILE_PATH "/opt/usr/data/network/connman.log"
#define MAX_LOG_SIZE	1 * 1024 * 1024
#define MAX_LOG_COUNT	3

#define openlog __connman_log_open
#define closelog __connman_log_close
#define vsyslog __connman_log
#define syslog __connman_log_s

static FILE *log_file = NULL;

void __connman_log_open(const char *ident, int option, int facility)
{
	if (log_file == NULL)
		log_file = (FILE *)fopen(LOG_FILE_PATH, "a+");
}

void __connman_log_close(void)
{
	fclose(log_file);
	log_file = NULL;
}

static void __connman_log_update_file_revision(int rev)
{
	int next_log_rev = 0;
	char *log_file = NULL;
	char *next_log_file = NULL;

	next_log_rev = rev + 1;

	log_file = g_strdup_printf("%s.%d", LOG_FILE_PATH, rev);
	next_log_file = g_strdup_printf("%s.%d", LOG_FILE_PATH, next_log_rev);

	if (next_log_rev >= MAX_LOG_COUNT)
		remove(next_log_file);

	if (access(next_log_file, F_OK) == 0)
		__connman_log_update_file_revision(next_log_rev);

	if (rename(log_file, next_log_file) != 0)
		remove(log_file);

	g_free(log_file);
	g_free(next_log_file);
}

static void __connman_log_make_backup(void)
{
	const int rev = 0;
	char *backup = NULL;

	backup = g_strdup_printf("%s.%d", LOG_FILE_PATH, rev);

	if (access(backup, F_OK) == 0)
		__connman_log_update_file_revision(rev);

	if (rename(LOG_FILE_PATH, backup) != 0)
		remove(LOG_FILE_PATH);

	g_free(backup);
}

static void __connman_log_get_local_time(char *strtime, const int size)
{
	struct timeval tv;
	struct tm *local_ptm;
	char buf[32];

	gettimeofday(&tv, NULL);
	local_ptm = localtime(&tv.tv_sec);

	strftime(buf, sizeof(buf), "%m/%d %H:%M:%S", local_ptm);
	snprintf(strtime, size, "%s.%03ld", buf, tv.tv_usec / 1000);
}

void __connman_log(const int log_priority, const char *format, va_list ap)
{
	int log_size = 0;
	struct stat buf;
	char str[256];
	char strtime[40];

	if (log_file == NULL)
		log_file = (FILE *)fopen(LOG_FILE_PATH, "a+");

	if (log_file == NULL)
		return;

	fstat(fileno(log_file), &buf);
	log_size = buf.st_size;

	if (log_size >= MAX_LOG_SIZE) {
		fclose(log_file);
		log_file = NULL;

		__connman_log_make_backup();

		log_file = (FILE *)fopen(LOG_FILE_PATH, "a+");

		if (log_file == NULL)
			return;
	}

	__connman_log_get_local_time(strtime, sizeof(strtime));

	if (vsnprintf(str, sizeof(str), format, ap) > 0)
		fprintf(log_file, "%s %s\n", strtime, str);
}

void __connman_log_s(int log_priority, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	vsyslog(LOG_DEBUG, format, ap);

	va_end(ap);
}
#endif

/**
 * connman_info:
 * @format: format string
 * @Varargs: list of arguments
 *
 * Output general information
 */
void connman_info(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	vsyslog(LOG_INFO, format, ap);

	va_end(ap);
}

/**
 * connman_warn:
 * @format: format string
 * @Varargs: list of arguments
 *
 * Output warning messages
 */
void connman_warn(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	vsyslog(LOG_WARNING, format, ap);

	va_end(ap);
}

/**
 * connman_error:
 * @format: format string
 * @varargs: list of arguments
 *
 * Output error messages
 */
void connman_error(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	vsyslog(LOG_ERR, format, ap);

	va_end(ap);
}

/**
 * connman_debug:
 * @format: format string
 * @varargs: list of arguments
 *
 * Output debug message
 */
void connman_debug(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	vsyslog(LOG_DEBUG, format, ap);

	va_end(ap);
}

#if !defined TIZEN_EXT
static void print_backtrace(unsigned int offset)
{
	void *frames[99];
	size_t n_ptrs;
	unsigned int i;
	int outfd[2], infd[2];
	int pathlen;
	pid_t pid;

	if (program_exec == NULL)
		return;

	pathlen = strlen(program_path);

	n_ptrs = backtrace(frames, G_N_ELEMENTS(frames));
	if (n_ptrs < offset)
		return;

	if (pipe(outfd) < 0)
		return;

	if (pipe(infd) < 0) {
		close(outfd[0]);
		close(outfd[1]);
		return;
	}

	pid = fork();
	if (pid < 0) {
		close(outfd[0]);
		close(outfd[1]);
		close(infd[0]);
		close(infd[1]);
		return;
	}

	if (pid == 0) {
		close(outfd[1]);
		close(infd[0]);

		dup2(outfd[0], STDIN_FILENO);
		dup2(infd[1], STDOUT_FILENO);

		execlp("addr2line", "-C", "-f", "-e", program_exec, NULL);

		exit(EXIT_FAILURE);
	}

	close(outfd[0]);
	close(infd[1]);

	connman_error("++++++++ backtrace ++++++++");

	for (i = offset; i < n_ptrs - 1; i++) {
		Dl_info info;
		char addr[20], buf[PATH_MAX * 2];
		int len, written;
		char *ptr, *pos;

		dladdr(frames[i], &info);

		len = snprintf(addr, sizeof(addr), "%p\n", frames[i]);
		if (len < 0)
			break;

		written = write(outfd[1], addr, len);
		if (written < 0)
			break;

		len = read(infd[0], buf, sizeof(buf) - 1);
		if (len < 0)
			break;

		buf[len] = '\0';

		pos = strchr(buf, '\n');
		*pos++ = '\0';

		if (strcmp(buf, "??") == 0) {
			connman_error("#%-2u %p in %s", i - offset,
						frames[i], info.dli_fname);
			continue;
		}

		ptr = strchr(pos, '\n');
		*ptr++ = '\0';

		if (strncmp(pos, program_path, pathlen) == 0)
			pos += pathlen + 1;

		connman_error("#%-2u %p in %s() at %s", i - offset,
						frames[i], buf, pos);
	}

	connman_error("+++++++++++++++++++++++++++");

	kill(pid, SIGTERM);

	close(outfd[1]);
	close(infd[0]);
}

static void signal_handler(int signo)
{
	connman_error("Aborting (signal %d) [%s]", signo, program_exec);

	print_backtrace(2);

	exit(EXIT_FAILURE);
}

static void signal_setup(sighandler_t handler)
{
	struct sigaction sa;
	sigset_t mask;

	sigemptyset(&mask);
	sa.sa_handler = handler;
	sa.sa_mask = mask;
	sa.sa_flags = 0;
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
}
#endif

extern struct connman_debug_desc __start___debug[];
extern struct connman_debug_desc __stop___debug[];

static gchar **enabled = NULL;

static connman_bool_t is_enabled(struct connman_debug_desc *desc)
{
	int i;

	if (enabled == NULL)
		return FALSE;

	for (i = 0; enabled[i] != NULL; i++) {
		if (desc->name != NULL && g_pattern_match_simple(enabled[i],
							desc->name) == TRUE)
			return TRUE;
		if (desc->file != NULL && g_pattern_match_simple(enabled[i],
							desc->file) == TRUE)
			return TRUE;
	}

	return FALSE;
}

void __connman_log_enable(struct connman_debug_desc *start,
					struct connman_debug_desc *stop)
{
	struct connman_debug_desc *desc;
	const char *name = NULL, *file = NULL;

	if (start == NULL || stop == NULL)
		return;

	for (desc = start; desc < stop; desc++) {
		if (desc->flags & CONNMAN_DEBUG_FLAG_ALIAS) {
			file = desc->file;
			name = desc->name;
			continue;
		}

		if (file != NULL || name != NULL) {
			if (g_strcmp0(desc->file, file) == 0) {
				if (desc->name == NULL)
					desc->name = name;
			} else
				file = NULL;
		}

		if (is_enabled(desc) == TRUE)
			desc->flags |= CONNMAN_DEBUG_FLAG_PRINT;
	}
}

int __connman_log_init(const char *program, const char *debug,
						connman_bool_t detach)
{
	static char path[PATH_MAX];
	int option = LOG_NDELAY | LOG_PID;

	program_exec = program;
	program_path = getcwd(path, sizeof(path));

	if (debug != NULL)
		enabled = g_strsplit_set(debug, ":, ", 0);

	__connman_log_enable(__start___debug, __stop___debug);

	if (detach == FALSE)
		option |= LOG_PERROR;

#if !defined TIZEN_EXT
	signal_setup(signal_handler);
#endif

	openlog(basename(program), option, LOG_DAEMON);

	syslog(LOG_INFO, "Connection Manager version %s", VERSION);

	return 0;
}

void __connman_log_cleanup(void)
{
	syslog(LOG_INFO, "Exit");

	closelog();

#if !defined TIZEN_EXT
	signal_setup(SIG_DFL);
#endif

	g_strfreev(enabled);
}
