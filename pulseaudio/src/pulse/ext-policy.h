#ifndef foopulseextpolicyhfoo
#define foopulseextpolicyhfoo

/***
  This file is part of PulseAudio.

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

#include <pulse/context.h>
#include <pulse/version.h>

/** \file
 *
 * Routines for controlling module-policy
 */

PA_C_DECL_BEGIN

enum {
    SUBCOMMAND_TEST,
    SUBCOMMAND_PLAY_SAMPLE,
    SUBCOMMAND_PLAY_SAMPLE_CONTINUOUSLY,
    SUBCOMMAND_MONO,
    SUBCOMMAND_BALANCE,
    SUBCOMMAND_MUTEALL,
    SUBCOMMAND_SVOICE_WAKEUP_ENABLE,
    SUBCOMMAND_SVOICE_SEAMLESS_ONOFF,
    SUBCOMMAND_SVOICE_SET_PARAM,
    SUBCOMMAND_SET_USE_CASE,
    SUBCOMMAND_SET_SESSION, // 10
    SUBCOMMAND_SET_SUBSESSION,
    SUBCOMMAND_SET_ACTIVE_DEVICE,
    SUBCOMMAND_RESET,
    SUBCOMMAND_GET_VOLUME_LEVEL_MAX,
    SUBCOMMAND_GET_VOLUME_LEVEL,
    SUBCOMMAND_SET_VOLUME_LEVEL,
    SUBCOMMAND_UPDATE_VOLUME,
    SUBCOMMAND_GET_MUTE,
    SUBCOMMAND_SET_MUTE,
    SUBCOMMAND_VOLUME_FADE, // 20
    SUBCOMMAND_IS_AVAILABLE_HIGH_LATENCY,
    SUBCOMMAND_UNLOAD_HDMI,

    // call settings
    SUBCOMMAND_SET_CALL_NETWORK_TYPE,
    SUBCOMMAND_SET_CALL_NREC,
    SUBCOMMAND_SET_CALL_EXTRA_VOLUME,
    SUBCOMMAND_SET_BLUETOOTH_BANDWIDTH,
    SUBCOMMAND_SET_BLUETOOTH_NREC,

    // audio filters
    SUBCOMMAND_VSP_SPEED,
    SUBCOMMAND_SA_FILTER_ACTION,
    SUBCOMMAND_SA_PRESET_MODE,
    SUBCOMMAND_SA_EQ,
    SUBCOMMAND_SA_EXTEND,
    SUBCOMMAND_SA_DEVICE,
    SUBCOMMAND_SA_SQUARE,
    SUBCOMMAND_DHA_PARAM,
};

typedef enum {
    PA_TIZEN_SESSION_MEDIA,
    PA_TIZEN_SESSION_VOICECALL,
    PA_TIZEN_SESSION_VIDEOCALL,
    PA_TIZEN_SESSION_VOIP,
    PA_TIZEN_SESSION_FMRADIO,
    PA_TIZEN_SESSION_CAMCORDER,
    PA_TIZEN_SESSION_NOTIFICATION,
    PA_TIZEN_SESSION_ALARM,
    PA_TIZEN_SESSION_EMERGENCY,
    PA_TIZEN_SESSION_VOICE_RECOGNITION,
    PA_TIZEN_SESSION_MAX
} pa_tizen_session_t;

typedef enum {
    PA_TIZEN_SUBSESSION_NONE,
    PA_TIZEN_SUBSESSION_VOICE,
    PA_TIZEN_SUBSESSION_RINGTONE,
    PA_TIZEN_SUBSESSION_MEDIA,
    PA_TIZEN_SUBSESSION_VR_INIT,
    PA_TIZEN_SUBSESSION_VR_NORMAL,
    PA_TIZEN_SUBSESSION_VR_DRIVE,
    PA_TIZEN_SUBSESSION_STEREO_REC,
    PA_TIZEN_SUBSESSION_STEREO_INTERVIEW_REC,
    PA_TIZEN_SUBSESSION_STEREO_CONVERSATION_REC,
    PA_TIZEN_SUBSESSION_MONO_REC,
    PA_TIZEN_SUBSESSION_VOICE_3G,
    PA_TIZEN_SUBSESSION_AM_PLAY,
    PA_TIZEN_SUBSESSION_AM_REC,
    PA_TIZEN_SUBSESSION_VC_FORWARDING,
    PA_TIZEN_SUBSESSION_MAX
} pa_tizen_subsession_t;

enum {
    PA_TIZEN_SUBSESSION_OPT_SVOICE          = 0x00000001,
    PA_TIZEN_SUBSESSION_OPT_WAKEUP          = 0x00000010,
    PA_TIZEN_SUBSESSION_OPT_COMMAND         = 0x00000020,
};

typedef enum {
    PA_TIZEN_DEVICE_IN_NONE,
    PA_TIZEN_DEVICE_IN_MIC,                 /**< Device builtin mic. */
    PA_TIZEN_DEVICE_IN_WIRED_ACCESSORY,     /**< Wired input devices */
    PA_TIZEN_DEVICE_IN_BT_SCO,              /**< Bluetooth SCO device */
} pa_tizen_device_in_t;

typedef enum pa_tizen_device_out {
    PA_TIZEN_DEVICE_OUT_NONE,
    PA_TIZEN_DEVICE_OUT_SPEAKER,            /**< Device builtin speaker */
    PA_TIZEN_DEVICE_OUT_RECEIVER,           /**< Device builtin receiver */
    PA_TIZEN_DEVICE_OUT_WIRED_ACCESSORY,    /**< Wired output devices such as headphone, headset, and so on. */
    PA_TIZEN_DEVICE_OUT_BT_SCO,             /**< Bluetooth SCO device */
    PA_TIZEN_DEVICE_OUT_BT_A2DP,            /**< Bluetooth A2DP device */
    PA_TIZEN_DEVICE_OUT_DOCK,               /**< DOCK device */
    PA_TIZEN_DEVICE_OUT_HDMI,               /**< HDMI device */
    PA_TIZEN_DEVICE_OUT_MIRRORING,          /**< MIRRORING device */
    PA_TIZEN_DEVICE_OUT_USB_AUDIO,          /**< USB Audio device */
    PA_TIZEN_DEVICE_OUT_MULTIMEDIA_DOCK,    /**< Multimedia DOCK device */
} pa_tizen_device_out_t;

typedef enum pa_tizen_volume_type {
    PA_TIZEN_VOLUME_TYPE_SYSTEM,            /**< System volume type */
    PA_TIZEN_VOLUME_TYPE_NOTIFICATION,      /**< Notification volume type */
    PA_TIZEN_VOLUME_TYPE_ALARM,             /**< Alarm volume type */
    PA_TIZEN_VOLUME_TYPE_RINGTONE,          /**< Ringtone volume type */
    PA_TIZEN_VOLUME_TYPE_MEDIA,             /**< Media volume type */
    PA_TIZEN_VOLUME_TYPE_CALL,              /**< Call volume type */
    PA_TIZEN_VOLUME_TYPE_VOIP,              /**< VOIP volume type */
    PA_TIZEN_VOLUME_TYPE_VOICE,             /**< VOICE volume type */
    PA_TIZEN_VOLUME_TYPE_SVOICE,            /**< SVOICE volume type */
    PA_TIZEN_VOLUME_TYPE_FIXED,             /**< Volume type for fixed acoustic level */
    PA_TIZEN_VOLUME_TYPE_MAX,               /**< Volume type count */
    PA_TIZEN_VOLUME_TYPE_DEFAULT = PA_TIZEN_VOLUME_TYPE_SYSTEM,
} pa_tizen_volume_type_t;

typedef enum pa_tizen_gain_type {
    PA_TIZEN_GAIN_TYPE_DEFAULT,
    PA_TIZEN_GAIN_TYPE_DIALER,
    PA_TIZEN_GAIN_TYPE_TOUCH,
    PA_TIZEN_GAIN_TYPE_AF,
    PA_TIZEN_GAIN_TYPE_SHUTTER1,
    PA_TIZEN_GAIN_TYPE_SHUTTER2,
    PA_TIZEN_GAIN_TYPE_CAMCODING,
    PA_TIZEN_GAIN_TYPE_MIDI,
    PA_TIZEN_GAIN_TYPE_BOOTING,
    PA_TIZEN_GAIN_TYPE_VIDEO,
    PA_TIZEN_GAIN_TYPE_TTS,
    PA_TIZEN_GAIN_TYPE_TONE,
    PA_TIZEN_GAIN_TYPE_MAX,
} pa_tizen_gain_type_t;

typedef enum pa_tizen_fade_type {
    PA_TIZEN_FADEDOWN_REQUEST,
    PA_TIZEN_FADEUP_REQUEST
} pa_tizen_fade_type_t;

#define PA_TIZEN_VOLUME_TYPE_LEVEL_MAX           15

/** Callback prototype for pa_ext_policy_test(). \since 0.9.21 */
typedef void (*pa_ext_policy_test_cb_t)(
        pa_context *c,
        uint32_t version,
        void *userdata);

/** Test if this extension module is available in the server. \since 0.9.21 */
pa_operation *pa_ext_policy_test(
        pa_context *c,
        pa_ext_policy_test_cb_t cb,
        void *userdata);

/** Callback prototype for pa_ext_policy_play_sample(). \since 0.9.21 */
typedef void (*pa_ext_policy_play_sample_cb_t)(
        pa_context *c,
        uint32_t stream_index,
        void *userdata);

/* Similar with pa_context_play_sample, but can apply volume type */
pa_operation *pa_ext_policy_play_sample (
        pa_context *c,
        const char *name,
        uint32_t volume_type,
        uint32_t gain_type,
        uint32_t volume_level,
        pa_ext_policy_play_sample_cb_t cb,
        void *userdata);

/* make one sink-input and then push burst cam-shutter sound. */
pa_operation *pa_ext_policy_play_sample_continuously (
        pa_context *c,
        const char *name,
        int start, /* start/stop */
        uint32_t volume_type,
        uint32_t gain_type,
        uint32_t volume_level,
        pa_usec_t interval, /* timeout_cb interval */
        pa_ext_policy_play_sample_cb_t cb,
        void *userdata);

/** Enable the mono mode. \since 0.9.21 */
pa_operation *pa_ext_policy_set_mono (
        pa_context *c,
        int enable,
        pa_context_success_cb_t cb,
        void *userdata);

/** Enable the balance mode. \since 0.9.21 */
pa_operation *pa_ext_policy_set_balance (
        pa_context *c,
        double *balance,
        pa_context_success_cb_t cb,
        void *userdata);

/** Enable the muteall mode. \since 0.9.21 */
pa_operation *pa_ext_policy_set_muteall (
        pa_context *c,
        int enable,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_svoice_wakeup_enable (
        pa_context *c,
        int enable,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_svoice_seamless_onoff (
        pa_context *c,
        int onoff,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_svoice_set_param (
        pa_context *c,
        const char * param_str,
        int param_value,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_set_use_case (
        pa_context *c,
        const char *verb,
        const char *devices[],
        const int num_devices,
        const char *modifiers[],
        const int num_modifiers,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_set_session (
        pa_context *c,
        uint32_t session,
        uint32_t start,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_set_subsession (
        pa_context *c,
        uint32_t subsession,
        uint32_t subsession_opt,
        pa_context_success_cb_t cb,
        void *userdata);

typedef void (*pa_ext_policy_set_active_device_cb_t)(
        pa_context *c,
        int success,
        uint32_t need_update,
        void *userdata);

typedef void (*pa_ext_policy_send_message_cb_t)(
        pa_context *c,
        int success,
        uint32_t extra_values,
        void *userdata);

pa_operation *pa_ext_policy_set_active_device (
        pa_context *c,
        uint32_t device_in,
        uint32_t device_out,
        pa_ext_policy_set_active_device_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_reset (
        pa_context *c,
        pa_context_success_cb_t cb,
        void *userdata);

/** Callback prototype for pa_ext_policy_get_volume_level_max(). \since 0.9.21 */
typedef void (*pa_ext_policy_get_volume_level_max_cb_t)(
        pa_context *c,
        uint32_t volume_level,
        void *userdata);

pa_operation *pa_ext_policy_get_volume_level_max (
        pa_context *c,
        uint32_t volume_type,
        pa_ext_policy_get_volume_level_max_cb_t cb,
        void *userdata);

/** Callback prototype for pa_ext_policy_get_volume_level(). \since 0.9.21 */
typedef void (*pa_ext_policy_get_volume_level_cb_t)(
        pa_context *c,
        uint32_t volume_level,
        void *userdata);

pa_operation *pa_ext_policy_get_volume_level (
        pa_context *c,
        uint32_t stream_idx,
        uint32_t volume_type,
        pa_ext_policy_get_volume_level_max_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_set_volume_level (
        pa_context *c,
        uint32_t stream_idx,
        uint32_t volume_type,
        uint32_t volume_level,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_update_volume (
        pa_context *c,
        pa_context_success_cb_t cb,
        void *userdata);

/** Callback prototype for pa_ext_policy_get_mute(). \since 0.9.21 */
typedef void (*pa_ext_policy_get_mute_cb_t)(
        pa_context *c,
        uint32_t mute,
        void *userdata);

pa_operation *pa_ext_policy_get_mute (
        pa_context *c,
        uint32_t stream_idx,
        uint32_t volume_type,
        uint32_t direction,
        pa_ext_policy_get_mute_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_set_mute (
        pa_context *c,
        uint32_t stream_idx,
        uint32_t volume_type,
        uint32_t direction,
        uint32_t mute,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_volume_fade (
        pa_context *c,
        uint32_t stream_idx,
        pa_tizen_fade_type_t up_down, // 0 : fadedown, 1:fadeup
        int duration, // secs
        pa_context_success_cb_t cb,
        void *userdata);

/** Callback prototype for pa_ext_policy_get_mute(). \since 0.9.21 */
typedef void (*pa_ext_policy_is_available_high_latency_cb_t)(
        pa_context *c,
        uint32_t available,
        void *userdata);

pa_operation *pa_ext_policy_is_available_high_latency (
        pa_context *c,
        pa_ext_policy_is_available_high_latency_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_vsp_set_speed (
        pa_context *c,
        uint32_t stream_idx,
        int value,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_sa_set_filter_action(
        pa_context *c,
        uint32_t stream_idx,
        int value,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_sa_set_preset_mode(
        pa_context *c,
        uint32_t stream_idx,
        int value,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_sa_set_eq(
        pa_context *c,
        uint32_t stream_idx,
        int eq[],
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_sa_set_extend(
        pa_context *c,
        uint32_t stream_idx,
        int ext[],
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_sa_set_device(
        pa_context *c,
        uint32_t stream_idx,
        int dev,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_sa_set_square(
        pa_context *c,
        uint32_t stream_idx,
        int row,
        int col,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_dha_set_param (
        pa_context *c,
        uint32_t stream_idx,
        int onoff,
        int gain[],
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_unload_hdmi (
        pa_context *c,
        pa_context_success_cb_t cb,
        void *userdata);

pa_operation *pa_ext_policy_send_message (
        pa_context *c,
        pa_ext_policy_send_message_cb_t cb,
        int command,
        void *userdata,
        const char* types,
        ...);

PA_C_DECL_END

#endif
