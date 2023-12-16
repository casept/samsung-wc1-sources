#ifndef foobluez5utilhfoo
#define foobluez5utilhfoo

/***
  This file is part of PulseAudio.

  Copyright 2008-2013 João Paulo Rechi Vita

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <pulsecore/core.h>

#define PA_BLUETOOTH_UUID_A2DP_SOURCE "0000110a-0000-1000-8000-00805f9b34fb"
#define PA_BLUETOOTH_UUID_A2DP_SINK   "0000110b-0000-1000-8000-00805f9b34fb"
#define PA_BLUETOOTH_UUID_HSP_HS      "00001108-0000-1000-8000-00805f9b34fb"
#define PA_BLUETOOTH_UUID_HSP_AG      "00001112-0000-1000-8000-00805f9b34fb"
#define PA_BLUETOOTH_UUID_HFP_HF      "0000111e-0000-1000-8000-00805f9b34fb"
#define PA_BLUETOOTH_UUID_HFP_AG      "0000111f-0000-1000-8000-00805f9b34fb"

typedef struct pa_bluetooth_transport pa_bluetooth_transport;
typedef struct pa_bluetooth_device pa_bluetooth_device;
typedef struct pa_bluetooth_adapter pa_bluetooth_adapter;
typedef struct pa_bluetooth_discovery pa_bluetooth_discovery;

typedef enum pa_bluetooth_hook {
    PA_BLUETOOTH_HOOK_DEVICE_CONNECTION_CHANGED,          /* Call data: pa_bluetooth_device */
    PA_BLUETOOTH_HOOK_TRANSPORT_STATE_CHANGED,            /* Call data: pa_bluetooth_transport */
    PA_BLUETOOTH_HOOK_MAX
} pa_bluetooth_hook_t;

typedef enum profile {
    PA_BLUETOOTH_PROFILE_A2DP_SINK,
    PA_BLUETOOTH_PROFILE_A2DP_SOURCE,
    PA_BLUETOOTH_PROFILE_HEADSET_HEAD_UNIT,
    PA_BLUETOOTH_PROFILE_HEADSET_AUDIO_GATEWAY,
    PA_BLUETOOTH_PROFILE_OFF
} pa_bluetooth_profile_t;
#define PA_BLUETOOTH_PROFILE_COUNT PA_BLUETOOTH_PROFILE_OFF

typedef enum pa_bluetooth_transport_state {
    PA_BLUETOOTH_TRANSPORT_STATE_DISCONNECTED,
    PA_BLUETOOTH_TRANSPORT_STATE_IDLE,
    PA_BLUETOOTH_TRANSPORT_STATE_PLAYING
} pa_bluetooth_transport_state_t;

typedef int (*pa_bluetooth_transport_acquire_cb)(pa_bluetooth_transport *t, bool optional, size_t *imtu, size_t *omtu);
typedef void (*pa_bluetooth_transport_release_cb)(pa_bluetooth_transport *t);

struct pa_bluetooth_transport {
    pa_bluetooth_device *device;

    char *owner;
    char *path;
    pa_bluetooth_profile_t profile;

    uint8_t codec;
    uint8_t *config;
    size_t config_size;

    pa_bluetooth_transport_state_t state;

    pa_bluetooth_transport_acquire_cb acquire;
    pa_bluetooth_transport_release_cb release;
    void *userdata;
};

struct pa_bluetooth_device {
    pa_bluetooth_discovery *discovery;
    pa_bluetooth_adapter *adapter;

    int device_info_valid;      /* 0: no results yet; 1: good results; -1: bad results ... */

    /* Device information */
    char *path;
    char *adapter_path;
    char *alias;
    char *address;
    uint32_t class_of_device;
    pa_hashmap *uuids;

    pa_bluetooth_transport *transports[PA_BLUETOOTH_PROFILE_COUNT];
};

struct pa_bluetooth_adapter {
    pa_bluetooth_discovery *discovery;
    char *path;
    char *address;
};

pa_bluetooth_transport *pa_bluetooth_transport_new(pa_bluetooth_device *d, const char *owner, const char *path,
                                                   pa_bluetooth_profile_t p, const uint8_t *config, size_t size);

void pa_bluetooth_transport_put(pa_bluetooth_transport *t);
void pa_bluetooth_transport_free(pa_bluetooth_transport *t);

bool pa_bluetooth_device_any_transport_connected(const pa_bluetooth_device *d);
#ifdef __TIZEN_BT__
bool pa_bluetooth_device_sink_transport_connected(const pa_bluetooth_device *d);
bool pa_bluetooth_device_source_transport_connected(const pa_bluetooth_device *d);
#endif

pa_bluetooth_device* pa_bluetooth_discovery_get_device_by_path(pa_bluetooth_discovery *y, const char *path);
pa_bluetooth_device* pa_bluetooth_discovery_get_device_by_address(pa_bluetooth_discovery *y, const char *remote, const char *local);

pa_hook* pa_bluetooth_discovery_hook(pa_bluetooth_discovery *y, pa_bluetooth_hook_t hook);

const char *pa_bluetooth_profile_to_string(pa_bluetooth_profile_t profile);

pa_bluetooth_discovery* pa_bluetooth_discovery_get(pa_core *core);
pa_bluetooth_discovery* pa_bluetooth_discovery_ref(pa_bluetooth_discovery *y);
void pa_bluetooth_discovery_unref(pa_bluetooth_discovery *y);

#ifdef BLUETOOTH_APTX_SUPPORT
int pa_load_aptx(const char *aptx_lib_name);
int pa_unload_aptx(void);
void* pa_aptx_get_handle(void);
#endif

#endif
