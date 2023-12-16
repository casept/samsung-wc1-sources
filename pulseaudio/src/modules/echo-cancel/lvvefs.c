/***
    This file is part of PulseAudio.

    Copyright 2011 KwangHui Cho <kwanghui.cho@samsung.com>

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/modargs.h>
#include "echo-cancel.h"

#define LVVEFS_NO_FILEx /* suppresses the file name print out */
#define LVVEFS_CONFIGURATION_CSC_PATH   "/opt/system/csc-default/usr/tuning/"
#define LVVEFS_CONFIGURATION_PATH       "/usr/etc/"

#define DEV_ID_RX_PARAM_HANDSET     1
#define DEV_ID_TX_PARAM_HANDSET     1
#define DEV_ID_RX_PARAM_SPEAKER     2
#define DEV_ID_TX_PARAM_SPEAKER     2
#define DEV_ID_RX_PARAM_HEADSET     4
#define DEV_ID_TX_PARAM_HEADSET     4
#define DEV_ID_RX_PARAM_HEADPHONE   8
#define DEV_ID_TX_PARAM_HEADPHONE   8
#define DEV_ID_RX_PARAM_BLUETOOTH   32
#define DEV_ID_TX_PARAM_BLUETOOTH   32

#define DEFAULT_TRACE_LEVEL 1
#define DEFAULT_BAND_WIDTH (LVVEFS_VOICEMODE_WIDEBAND)

#define SOURCE_CHANNELS (1)
#define SINK_CHANNELS (1)

static const LVM_INT32 SAMPLE_RATE[9] = /* Sample rate supported by NXP library */
{
    8000,
    11025,
    12000,
    16000,
    22050,
    24000,
    32000,
    44100,
    48000
};


/* General */
static const LVM_INT16 BUFFER_SIZE[9] = /* Size of buffer in 16-bit words */
{
    320,
    441,
    480,
    640,
    882,
    960,
    1280,
    1764,
    1920
};

static const char* const valid_modargs[] = {
    "trace_level",
    "wide_band",
    NULL
};


void LVVEFS_trace_func(LVM_INT32   line,
                    LVM_INT8*   fileName,
                    LVM_INT32   level,
                    LVM_INT8*   message )
{
    int i = 0;
    int lvl = 0;
    LVM_INT8 indent[10] ={0,};

    /* do some formatting of the output print */
    if(level > sizeof(indent)-1)
        lvl = sizeof(indent)-1;
    else
        lvl = level;
    for (i =0 ; i < lvl; i ++) {
        indent[i] = ' ';
    }
    indent[i] = '\0' ;

    /* do the actual print */
    pa_log_debug("%sLVVEFS_TRACE: %s at %ld in %s",indent,message,line,fileName);


}


void LVVEFS_debug_func(    LVM_INT32   line,
                        LVM_INT8*   fileName,
                        LVM_INT32   level,
                        LVM_INT8*   stringCondition,
                        LVM_INT8*   message,
                        LVM_INT32   error)
{
    int i = 0;
    int lvl = 0;
    LVM_INT8 indent[10] ={0,};

    /* try to "indent" the resulting traces depending on the level */
    if(level > sizeof(indent)-1)
        lvl = sizeof(indent)-1;
    else
        lvl = level;
    for (i =0 ; i < lvl; i ++) {
        indent[i] = ' ';
    }
    indent[i] = '\0' ;


    pa_log("%sLVVEFS_ERROR: %ld, on %s: %s Line %ld in: %s",indent, error, stringCondition, message, line, fileName);


}

static void pa_lvvefs_ec_fixate_spec(pa_sample_spec *rec_ss, pa_channel_map *rec_map,
                                     pa_sample_spec *play_ss, pa_channel_map *play_map,
                                     pa_sample_spec *out_ss, pa_channel_map *out_map,
                                     LVVEFS_Rx_Config_st *LVVEFS_Rx_Config, LVVEFS_Tx_Config_st *LVVEFS_Tx_Config, int smpl_rate)
{
    // make pulseaudio sample spec. and channel map
    out_ss->format = PA_SAMPLE_S16NE;
    out_ss->channels = SOURCE_CHANNELS; //add for SLP
    out_ss->rate = SAMPLE_RATE[smpl_rate];//add for SLP
    pa_channel_map_init_mono(out_map);//add for SLP
    *play_ss = *out_ss;
    *play_map = *out_map;
    *rec_ss = *out_ss;
    *rec_map = *out_map;

    //make LVVEFS Rx/Tx config
    LVVEFS_Rx_Config->BufferConfig[LVVEFS_RX_PORT_IN].SampleRate   = smpl_rate;
    LVVEFS_Rx_Config->BufferConfig[LVVEFS_RX_PORT_IN].Channels     = SINK_CHANNELS;
    LVVEFS_Rx_Config->BufferConfig[LVVEFS_RX_PORT_IN].Format       = 16;

    LVVEFS_Rx_Config->BufferConfig[LVVEFS_RX_PORT_OUT].SampleRate  = smpl_rate;
    LVVEFS_Rx_Config->BufferConfig[LVVEFS_RX_PORT_OUT].Channels    = SINK_CHANNELS;
    LVVEFS_Rx_Config->BufferConfig[LVVEFS_RX_PORT_OUT].Format      = 16;

    LVVEFS_Tx_Config->BufferConfig[LVVEFS_TX_PORT_IN].SampleRate   = smpl_rate;
    LVVEFS_Tx_Config->BufferConfig[LVVEFS_TX_PORT_IN].Channels     = SOURCE_CHANNELS;
    LVVEFS_Tx_Config->BufferConfig[LVVEFS_TX_PORT_IN].Format       = 16;

    LVVEFS_Tx_Config->BufferConfig[LVVEFS_TX_PORT_REF].SampleRate  = smpl_rate;
    LVVEFS_Tx_Config->BufferConfig[LVVEFS_TX_PORT_REF].Channels    = SOURCE_CHANNELS;
    LVVEFS_Tx_Config->BufferConfig[LVVEFS_TX_PORT_REF].Format      = 16;

    LVVEFS_Tx_Config->BufferConfig[LVVEFS_TX_PORT_OUT].SampleRate  = smpl_rate;
    LVVEFS_Tx_Config->BufferConfig[LVVEFS_TX_PORT_OUT].Channels    = SOURCE_CHANNELS;
    LVVEFS_Tx_Config->BufferConfig[LVVEFS_TX_PORT_OUT].Format      = 16;

}


pa_bool_t pa_lvvefs_ec_init(pa_core *c, pa_echo_canceller *ec,
                            pa_sample_spec *rec_ss, pa_channel_map *rec_map,
                            pa_sample_spec *play_ss, pa_channel_map *play_map,
                            pa_sample_spec *out_ss, pa_channel_map *out_map,
                            uint32_t *nframes, const char *args)
{
    pa_modargs *ma;
    int trace_level = 0;
    int bandwidth = 0;
    int smpl_rate = 0;
    LVVEFS_ReturnStatus_en               LVVEFS_Status = LVVEFS_SUCCESS;
    LVVEFS_Rx_Config_st                  LVVEFS_Rx_Config;
    LVVEFS_Tx_Config_st                  LVVEFS_Tx_Config;

    LVVEFS_Rx_Params_st                  LVVEFS_Rx_Params;
    LVVEFS_Tx_Params_st                  LVVEFS_Tx_Params;

    LVM_UINT32                          Device;

    LVVEFS_InstanceParams_st             LVVEFS_InstanceParams = { LVVEFS_CONFIGURATION_PATH };
    LVVEFS_InstanceParams_st             LVVEFS_InstanceParams_Csc = { LVVEFS_CONFIGURATION_CSC_PATH };


    /*
     * get module argument
     */
    if (!(ma = pa_modargs_new(args, valid_modargs))) {
        pa_log_error("Failed to parse submodule arguments.");
        goto fail;
    }

    trace_level = DEFAULT_TRACE_LEVEL;
    if (pa_modargs_get_value_u32(ma, "trace_level", &trace_level) < 0 || trace_level < 0 || trace_level > 10) {
        pa_log_error("Invalid trace_level specification");
        goto fail;
    }

    bandwidth = DEFAULT_BAND_WIDTH; //LVVEFS_VOICEMODE_WIDEBAND by default
    if (pa_modargs_get_value_u32(ma, "wide_band", &bandwidth) < 0 || bandwidth < LVVEFS_VOICEMODE_NARROWBAND || bandwidth > LVVEFS_VOICEMODE_WIDEBAND) {
        pa_log_error("Invalid wide_band value %d", bandwidth);
        goto fail;
    }

    /*
     * determine audio specification
     */
    smpl_rate = bandwidth ? LVM_FS_16000 : LVM_FS_8000;
    pa_lvvefs_ec_fixate_spec(rec_ss, rec_map, play_ss, play_map, out_ss, out_map, &LVVEFS_Rx_Config, &LVVEFS_Tx_Config, smpl_rate);

    /*
     * calculate block size
     */
    *nframes = BUFFER_SIZE[smpl_rate];
    ec->params.priv.lvvefs.samples_to_process = *nframes * SOURCE_CHANNELS;
    ec->params.priv.lvvefs.blocksize = ec->params.priv.lvvefs.samples_to_process * pa_sample_size (out_ss);
    pa_log_debug ("Using samples_to_process %d, blocksize %u, channels %d, rate %d",
        ec->params.priv.lvvefs.samples_to_process, ec->params.priv.lvvefs.blocksize, out_ss->channels, out_ss->rate);

    /*
     * assign debug & trace function.
     */
    LVVEFS_Status = LVVEFS_AssignTrace( (LVVEFS_Trace_t)LVVEFS_trace_func, 1 );
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_AddTraceInterface, status=[%d]", LVVEFS_Status);
        goto fail;
    }

    LVVEFS_Status = LVVEFS_AssignDebug( (LVVEFS_Debug_t)LVVEFS_debug_func );
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_AddDebugInterface, status=[%d]", LVVEFS_Status);
        goto fail;
    }

    /*
     * Init. LVVE engine
     */
    ec->params.priv.lvvefs.LVVEFS_Handle = LVM_NULL;

    // Create
    LVVEFS_Status = LVVEFS_Create(&ec->params.priv.lvvefs.LVVEFS_Handle, &LVVEFS_InstanceParams_Csc);
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        LVVEFS_Status = LVVEFS_Create(&ec->params.priv.lvvefs.LVVEFS_Handle, &LVVEFS_InstanceParams);
        if (LVVEFS_Status != LVVEFS_SUCCESS) {
            pa_log_error("Error while processing returned from LVVEFS_Create, status=[%d]", LVVEFS_Status);
            goto fail;
        }
        else {
            pa_log_debug ("LVVEFS_Create Success");
        }
    }
    // Configure Rx
    LVVEFS_Status = LVVEFS_Rx_Command (ec->params.priv.lvvefs.LVVEFS_Handle, LVVEFS_COMMAND_CONFIGURE, &LVVEFS_Rx_Config );
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_Command, status=[%d]", LVVEFS_Status);
        goto fail;
    }

    // Configure Tx
    LVVEFS_Status = LVVEFS_Tx_Command (ec->params.priv.lvvefs.LVVEFS_Handle, LVVEFS_COMMAND_CONFIGURE, &LVVEFS_Tx_Config );
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_Tx_Command, status=[%d]", LVVEFS_Status);
        goto fail;
    }

    // Set Rx Parameters
    LVVEFS_Rx_Params.Mute = LVM_MODE_OFF;
    LVVEFS_Rx_Params.VoiceMode = bandwidth;

    LVVEFS_Status = LVVEFS_Rx_Command (ec->params.priv.lvvefs.LVVEFS_Handle, LVVEFS_COMMAND_SET_PARAM, &LVVEFS_Rx_Params );
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_Rx_Commandstatus=[%d]", LVVEFS_Status);
        goto fail;
    }

    // Set Tx Parameters
    LVVEFS_Tx_Params.Mute = LVM_MODE_OFF;
    LVVEFS_Tx_Params.VoiceMode = bandwidth;

    LVVEFS_Status = LVVEFS_Tx_Command (ec->params.priv.lvvefs.LVVEFS_Handle, LVVEFS_COMMAND_SET_PARAM, &LVVEFS_Tx_Params );
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_Tx_Command, status=[%d]", LVVEFS_Status);
        goto fail;
    }

    // Set Rx Device
    Device = DEV_ID_RX_PARAM_HANDSET;
    LVVEFS_Status = LVVEFS_Rx_Command (ec->params.priv.lvvefs.LVVEFS_Handle, LVVEFS_COMMAND_SET_DEVICE, &Device );
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_Rx_Command, status=[%d]", LVVEFS_Status);
        goto fail;
    }

    // Set Tx Device
    Device = DEV_ID_TX_PARAM_HANDSET;
    LVVEFS_Status = LVVEFS_Tx_Command (ec->params.priv.lvvefs.LVVEFS_Handle, LVVEFS_COMMAND_SET_DEVICE, &Device );
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_Tx_Command, status=[%d]", LVVEFS_Status);
        goto fail;
    }

    //Alloc Buffer
    ec->params.priv.lvvefs.OutBuffer16_Rx = (LVM_INT16*) calloc(ec->params.priv.lvvefs.samples_to_process, sizeof(LVM_INT16));
    if (!ec->params.priv.lvvefs.OutBuffer16_Rx) {
        pa_log_error ("Error while allocate intermediate buffer, status=[%d]", LVVEFS_Status);
        goto fail;
    }

    if (ma)
        pa_modargs_free(ma);

    return TRUE;

fail:
    if(ec->params.priv.lvvefs.OutBuffer16_Rx != NULL)
        free(ec->params.priv.lvvefs.OutBuffer16_Rx);

    if (ma)
        pa_modargs_free(ma);
    return FALSE;
}

void pa_lvvefs_ec_run(pa_echo_canceller *ec, const uint8_t *rec, const uint8_t *play, uint8_t *out)
{
    LVVEFS_ReturnStatus_en LVVEFS_Status = LVVEFS_SUCCESS;

    LVVEFS_Buffer_st RxInBuffer;
    LVVEFS_Buffer_st RxOutBuffer;

    LVVEFS_Buffer_st RefBuffer;

    LVVEFS_Buffer_st TxInBuffer;
    LVVEFS_Buffer_st TxOutBuffer;

    /* Configuration of the Rx buffers */
    RxInBuffer.Size     = ec->params.priv.lvvefs.samples_to_process;
    RxInBuffer.pBuffer  = (LVM_INT16*)play;
    RxOutBuffer.Size    = ec->params.priv.lvvefs.samples_to_process;
    RxOutBuffer.pBuffer = ec->params.priv.lvvefs.OutBuffer16_Rx;

    /* Process the Rx samples by the LVVEFS */
    LVVEFS_Status = LVVEFS_Rx_Process(ec->params.priv.lvvefs.LVVEFS_Handle, &RxInBuffer, &RxOutBuffer);
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_Rx_Process, status=[%d]", LVVEFS_Status);
    }

    /* Write the samples that will be played on the loudspeaker to the LVVEFS reference port */
    RefBuffer.Size      = ec->params.priv.lvvefs.samples_to_process;
    RefBuffer.pBuffer   = ec->params.priv.lvvefs.OutBuffer16_Rx;

    LVVEFS_Status = LVVEFS_Tx_pushReference( ec->params.priv.lvvefs.LVVEFS_Handle, LVVEFS_TX_PORT_REF, &RefBuffer);
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("LVVEFS_Example_App.c: Error while processing returned from LVVEFS_WriteInPort when writing Ref, status=[%d]", LVVEFS_Status);
    }

    /* Configuration of the Tx buffers */
    TxInBuffer.Size     = ec->params.priv.lvvefs.samples_to_process;;
    TxInBuffer.pBuffer  = (LVM_INT16*)rec;
    TxOutBuffer.Size    = ec->params.priv.lvvefs.samples_to_process;;
    TxOutBuffer.pBuffer = (LVM_INT16*)out;

    /* Process the Tx samples by the LVVEFS */
    LVVEFS_Status = LVVEFS_Tx_Process(ec->params.priv.lvvefs.LVVEFS_Handle, &TxInBuffer, &TxOutBuffer);
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_Tx_Process, status=[%d]", LVVEFS_Status);
    }

}

static void _lvvefs_set_volume (pa_echo_canceller *ec, uint32_t vol_level)
{
    LVVEFS_ReturnStatus_en LVVEFS_Status = LVVEFS_SUCCESS;
    LVVEFS_Status = LVVEFS_Rx_Command(ec->params.priv.lvvefs.LVVEFS_Handle, LVVEFS_COMMAND_SET_VOLUME, (void *)&vol_level);
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while setting volume returned from LVVEFS_Rx_Command, status=[%d]", LVVEFS_Status);
    }
}

void pa_lvvefs_ec_set_volume (pa_echo_canceller *ec, uint32_t value)
{
    int volume_level = 0;
    ec->params.priv.lvvefs.volume = value;

    if(ec->params.priv.lvvefs.device == DEVICE_IN_MIC_OUT_WIRED ||
       ec->params.priv.lvvefs.device == DEVICE_IN_WIRED_OUT_WIRED) {
        volume_level = (value >> 8) & 0x00FF;
    } else {
        volume_level = value & 0x00FF;
    }
    volume_level = volume_level & 0x00FF;
    pa_log_debug("volume level in set volume = %d", volume_level);
    _lvvefs_set_volume(ec, volume_level);
}

void pa_lvvefs_ec_set_device (pa_echo_canceller *ec,uint32_t value)
{
    LVVEFS_ReturnStatus_en LVVEFS_Status = LVVEFS_SUCCESS;
    int volume_level = 0;
    int in_device = 0;
    int out_device = 0;
    ec->params.priv.lvvefs.device = value;

    if(value == DEVICE_IN_MIC_OUT_SPEAKER){
        in_device = DEV_ID_RX_PARAM_SPEAKER;
        out_device = DEV_ID_TX_PARAM_SPEAKER;
        volume_level = ec->params.priv.lvvefs.volume & 0x00FF;
    } else if(value == DEVICE_IN_MIC_OUT_RECEIVER) {
        in_device = DEV_ID_RX_PARAM_HANDSET;
        out_device = DEV_ID_TX_PARAM_HANDSET;
        volume_level = ec->params.priv.lvvefs.volume & 0x00FF;
    } else if(value == DEVICE_IN_MIC_OUT_WIRED) {
        in_device = DEV_ID_RX_PARAM_HEADPHONE;
        out_device = DEV_ID_TX_PARAM_HEADPHONE;
        volume_level = (ec->params.priv.lvvefs.volume >> 8) & 0x00FF;
    } else if(value == DEVICE_IN_WIRED_OUT_WIRED) {
        in_device = DEV_ID_RX_PARAM_HEADSET;
        out_device = DEV_ID_TX_PARAM_HEADSET;
        volume_level = (ec->params.priv.lvvefs.volume >> 8) & 0x00FF;
    } else if(value == DEVICE_IN_BT_SCO_OUT_BT_SCO) {
        in_device = DEV_ID_RX_PARAM_BLUETOOTH;
        out_device = DEV_ID_TX_PARAM_BLUETOOTH;
        volume_level = ec->params.priv.lvvefs.volume & 0x00FF;
    } else {
        pa_log_error("Invalid device");
    }
    //volume_level = volume_level & 0x00FF;
    pa_log_debug("volume level = %d & device = %d in set device function, in=[%d],out=[%d] ",volume_level, value, in_device, out_device);

    LVVEFS_Status = LVVEFS_Rx_Command(ec->params.priv.lvvefs.LVVEFS_Handle, LVVEFS_COMMAND_SET_DEVICE, (void *)&in_device);
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while setting device from LVVEFS_Rx_Command, status=[%d]", LVVEFS_Status);
    }
    LVVEFS_Status = LVVEFS_Tx_Command(ec->params.priv.lvvefs.LVVEFS_Handle, LVVEFS_COMMAND_SET_DEVICE, (void *)&out_device);
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while setting device from LVVEFS_Tx_Command, status=[%d]", LVVEFS_Status);
    }

    _lvvefs_set_volume(ec, volume_level);
}

void pa_lvvefs_ec_done(pa_echo_canceller *ec)
{
    LVVEFS_ReturnStatus_en LVVEFS_Status = LVVEFS_SUCCESS;

    if(ec->params.priv.lvvefs.OutBuffer16_Rx != NULL)
        free(ec->params.priv.lvvefs.OutBuffer16_Rx);

    LVVEFS_Status = LVVEFS_Destroy(ec->params.priv.lvvefs.LVVEFS_Handle);
    ec->params.priv.lvvefs.LVVEFS_Handle = LVM_NULL;
    if (LVVEFS_Status != LVVEFS_SUCCESS) {
        pa_log_error("Error while processing returned from LVVEFS_Destroy, status=[%d]", LVVEFS_Status);
    }

}
