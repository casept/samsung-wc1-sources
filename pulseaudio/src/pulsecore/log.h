#ifndef foologhfoo
#define foologhfoo

/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
  Copyright 2006 Pierre Ossman <ossman@cendio.se> for Cendio AB

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <stdarg.h>
#include <stdlib.h>

#include <pulsecore/macro.h>
#include <pulse/gccmacro.h>

#ifdef USE_DLOG

#include <dlog.h>
#ifndef DLOG_TAG
#define DLOG_TAG    "PULSEAUDIO"
#endif
#define USE_DLOG_DIRECT

#define COLOR_BLACK     30
#define COLOR_RED       31
#define COLOR_GREEN     32
#define COLOR_BLUE      34
#define COLOR_MAGENTA   35
#define COLOR_CYAN      36
#define COLOR_WHITE     97
#define COLOR_B_GRAY    100
#define COLOR_B_RED     101
#define COLOR_B_GREEN   102
#define COLOR_B_YELLOW  103
#define COLOR_B_BLUE    104
#define COLOR_B_MAGENTA 105
#define COLOR_B_CYAN    106
#define COLOR_REVERSE   7

#endif /* USE_DLOG */

/* A simple logging subsystem */

/* Where to log to */
typedef enum pa_log_target {
    PA_LOG_STDERR,  /* default */
    PA_LOG_SYSLOG,
#ifdef USE_DLOG
    PA_LOG_DLOG,
    PA_LOG_DLOG_COLOR,
#endif
    PA_LOG_NULL,    /* to /dev/null */
    PA_LOG_FD,      /* to a file descriptor, e.g. a char device */
    PA_LOG_TARGET_MAX
} pa_log_target_t;

typedef enum pa_log_level {
    PA_LOG_ERROR  = 0,    /* Error messages */
    PA_LOG_WARN   = 1,    /* Warning messages */
    PA_LOG_NOTICE = 2,    /* Notice messages */
    PA_LOG_INFO   = 3,    /* Info messages */
    PA_LOG_DEBUG  = 4,    /* debug message */
#ifdef __TIZEN_LOG__
    PA_LOG_VERBOSE = 5,
#endif
    PA_LOG_LEVEL_MAX
} pa_log_level_t;

typedef enum pa_log_flags {
    PA_LOG_COLORS      = 0x01, /* Show colorful output */
    PA_LOG_PRINT_TIME  = 0x02, /* Show time */
    PA_LOG_PRINT_FILE  = 0x04, /* Show source file */
    PA_LOG_PRINT_META  = 0x08, /* Show extended location information */
    PA_LOG_PRINT_LEVEL = 0x10, /* Show log level prefix */
} pa_log_flags_t;

typedef enum pa_log_merge {
    PA_LOG_SET,
    PA_LOG_UNSET,
    PA_LOG_RESET
} pa_log_merge_t;

/* Set an identification for the current daemon. Used when logging to syslog. */
void pa_log_set_ident(const char *p);

/* Set a log target. */
void pa_log_set_target(pa_log_target_t t);

/* Maximal log level */
void pa_log_set_level(pa_log_level_t l);

/* Set flags */
void pa_log_set_flags(pa_log_flags_t flags, pa_log_merge_t merge);

/* Set the file descriptor of the logging device.
   Daemon conf is in charge of opening this device */
void pa_log_set_fd(int fd);

/* Enable backtrace */
void pa_log_set_show_backtrace(unsigned nlevels);

/* Skip the first backtrace frames */
void pa_log_set_skip_backtrace(unsigned nlevels);

void pa_log_level_meta(
        pa_log_level_t level,
        const char*file,
        int line,
        const char *func,
        const char *format, ...) PA_GCC_PRINTF_ATTR(5,6);

void pa_log_levelv_meta(
        pa_log_level_t level,
        const char*file,
        int line,
        const char *func,
        const char *format,
        va_list ap);

void pa_log_level(
        pa_log_level_t level,
        const char *format, ...) PA_GCC_PRINTF_ATTR(2,3);

void pa_log_levelv(
        pa_log_level_t level,
        const char *format,
        va_list ap);

#if __STDC_VERSION__ >= 199901L

/* ISO varargs available */

#ifdef __TIZEN_LOG__
#define pa_log_debug_verbose(...)   pa_log_level_meta(PA_LOG_VERBOSE,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define pa_log_info_verbose(...)    pa_log_level_meta(PA_LOG_VERBOSE,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define pa_log_info_debug(...)      pa_log_level_meta(PA_LOG_DEBUG,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define pa_log_debug_verbose(...)   pa_log_level_meta(PA_LOG_DEBUG,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define pa_log_info_verbose(...)    pa_log_level_meta(PA_LOG_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define pa_log_info_debug(...)      pa_log_level_meta(PA_LOG_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#endif
#define pa_log_debug(...)  pa_log_level_meta(PA_LOG_DEBUG,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define pa_log_info(...)   pa_log_level_meta(PA_LOG_INFO,   __FILE__, __LINE__, __func__, __VA_ARGS__)
#define pa_log_notice(...) pa_log_level_meta(PA_LOG_NOTICE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define pa_log_warn(...)   pa_log_level_meta(PA_LOG_WARN,   __FILE__, __LINE__, __func__, __VA_ARGS__)
#define pa_log_error(...)  pa_log_level_meta(PA_LOG_ERROR,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define pa_logl(level, ...)  pa_log_level_meta(level,  __FILE__, __LINE__, __func__, __VA_ARGS__)

#else

#define LOG_FUNC(suffix, level) \
PA_GCC_UNUSED static void pa_log_##suffix(const char *format, ...) { \
    va_list ap; \
    va_start(ap, format); \
    pa_log_levelv_meta(level, NULL, 0, NULL, format, ap); \
    va_end(ap); \
}

#ifdef __TIZEN_LOG__
LOG_FUNC(debug_verbose, PA_LOG_VERBOSE)
LOG_FUNC(info_verbose, PA_LOG_VERBOSE)
LOG_FUNC(info_debug, PA_LOG_DEBUG)
#else
LOG_FUNC(debug_verbose, PA_LOG_DEBUG)
LOG_FUNC(info_verbose, PA_LOG_INFO)
LOG_FUNC(info_debug, PA_LOG_INFO)
#endif
LOG_FUNC(debug, PA_LOG_DEBUG)
LOG_FUNC(info, PA_LOG_INFO)
LOG_FUNC(notice, PA_LOG_NOTICE)
LOG_FUNC(warn, PA_LOG_WARN)
LOG_FUNC(error, PA_LOG_ERROR)

#endif

#define pa_log pa_log_error

pa_bool_t pa_log_ratelimit(pa_log_level_t level);

#endif
