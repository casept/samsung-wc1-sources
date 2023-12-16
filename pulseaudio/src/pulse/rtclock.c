/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/time.h>

#include <pulse/timeval.h>

#include <pulsecore/core-rtclock.h>

#include "rtclock.h"

pa_usec_t pa_rtclock_now(void) {
    struct timeval tv;

    return pa_timeval_load(pa_rtclock_get(&tv));
}

pa_usec_t pa_rtclock_now_args(pa_usec_t* usec) {
    struct timeval tv;

    *usec = pa_timeval_load(pa_rtclock_get(&tv));

    return *usec;
}
