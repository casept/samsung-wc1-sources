#ifndef fooauthcookiehfoo
#define fooauthcookiehfoo

/***
  This file is part of PulseAudio.

  Copyright 2008 Lennart Poettering

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

#include <pulsecore/core.h>

typedef struct pa_auth_cookie pa_auth_cookie;

pa_auth_cookie* pa_auth_cookie_get(pa_core *c, const char *cn, pa_bool_t create, size_t size);
pa_auth_cookie* pa_auth_cookie_ref(pa_auth_cookie *c);
void pa_auth_cookie_unref(pa_auth_cookie *c);

const uint8_t* pa_auth_cookie_read(pa_auth_cookie *, size_t size);

#endif
