/* GStreamer AAC parser plugin
 * Copyright (C) 2008 Nokia Corporation. All rights reserved.
 *
 * Contact: Stefan Kost <stefan.kost@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-aacparse
 * @short_description: AAC parser
 * @see_also: #GstAmrParse
 *
 * This is an AAC parser which handles both ADIF and ADTS stream formats.
 *
 * As ADIF format is not framed, it is not seekable and stream duration cannot
 * be determined either. However, ADTS format AAC clips can be seeked, and parser
 * can also estimate playback position and clip duration.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch filesrc location=abc.aac ! aacparse ! faad ! audioresample ! audioconvert ! alsasink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "gstaacparse.h"


static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/mpeg, "
        "framed = (boolean) true, " "mpegversion = (int) { 2, 4 }, "
        "stream-format = (string) { raw, adts, adif };"));

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/mpeg, mpegversion = (int) { 2, 4 };"));

GST_DEBUG_CATEGORY_STATIC (aacparse_debug);
#define GST_CAT_DEFAULT aacparse_debug


#define ADIF_MAX_SIZE 40        /* Should be enough */
#define ADTS_MAX_SIZE 10        /* Should be enough */

#ifdef GST_EXT_AACPARSER_MODIFICATION /* to get more accurate duration */
#define AAC_MAX_ESTIMATE_DURATION_BUF (1024 * 1024) /* use first 1 Mbyte */
#define AAC_SAMPLE_PER_FRAME 1024
#endif

#define AAC_FRAME_DURATION(parse) (GST_SECOND/parse->frames_per_sec)

static gboolean gst_aac_parse_start (GstBaseParse * parse);
static gboolean gst_aac_parse_stop (GstBaseParse * parse);

static gboolean gst_aac_parse_sink_setcaps (GstBaseParse * parse,
    GstCaps * caps);
static GstCaps *gst_aac_parse_sink_getcaps (GstBaseParse * parse);

static gboolean gst_aac_parse_check_valid_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, guint * size, gint * skipsize);

static GstFlowReturn gst_aac_parse_parse_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame);

#ifdef GST_EXT_AACPARSER_MODIFICATION /* make full aac(adts) index table when seek */
  static guint gst_aac_parse_adts_get_fast_frame_len (const guint8 * data);
  static gboolean gst_aac_parse_src_eventfunc(GstBaseParse * parse, GstEvent * event); // evil
  static gboolean gst_aac_parse_adts_src_eventfunc (GstBaseParse * parse, GstEvent * event);
  static gboolean gst_aac_parse_adif_src_eventfunc (GstBaseParse * parse, GstEvent * event); // evil 
  #define AAC_MAX_PULL_RANGE_BUF	( 5 * 1024 * 1024)   /* 5 MByte */
#endif

#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (aacparse_debug, "aacparse", 0, \
    "AAC audio stream parser");

GST_BOILERPLATE_FULL (GstAacParse, gst_aac_parse, GstBaseParse,
    GST_TYPE_BASE_PARSE, _do_init);

static inline gint
gst_aac_parse_get_sample_rate_from_index (guint sr_idx)
{
  static const guint aac_sample_rates[] = { 96000, 88200, 64000, 48000, 44100,
    32000, 24000, 22050, 16000, 12000, 11025, 8000
  };

  if (sr_idx < G_N_ELEMENTS (aac_sample_rates))
    return aac_sample_rates[sr_idx];
  GST_WARNING ("Invalid sample rate index %u", sr_idx);
  return 0;
}

/**
 * gst_aac_parse_base_init:
 * @klass: #GstElementClass.
 *
 */
static void
gst_aac_parse_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_template);
  gst_element_class_add_static_pad_template (element_class, &src_template);

  gst_element_class_set_details_simple (element_class,
      "AAC audio stream parser", "Codec/Parser/Audio",
      "Advanced Audio Coding parser", "Stefan Kost <stefan.kost@nokia.com>");
}


/**
 * gst_aac_parse_class_init:
 * @klass: #GstAacParseClass.
 *
 */
static void
gst_aac_parse_class_init (GstAacParseClass * klass)
{
  GstBaseParseClass *parse_class = GST_BASE_PARSE_CLASS (klass);

  parse_class->start = GST_DEBUG_FUNCPTR (gst_aac_parse_start);
  parse_class->stop = GST_DEBUG_FUNCPTR (gst_aac_parse_stop);
  parse_class->set_sink_caps = GST_DEBUG_FUNCPTR (gst_aac_parse_sink_setcaps);
  parse_class->get_sink_caps = GST_DEBUG_FUNCPTR (gst_aac_parse_sink_getcaps);
  parse_class->parse_frame = GST_DEBUG_FUNCPTR (gst_aac_parse_parse_frame);
  parse_class->check_valid_frame =
      GST_DEBUG_FUNCPTR (gst_aac_parse_check_valid_frame);

#ifdef GST_EXT_AACPARSER_MODIFICATION /* make full aac(adts) index table when seek */
  parse_class->src_event = GST_DEBUG_FUNCPTR (gst_aac_parse_src_eventfunc);
#endif
}


/**
 * gst_aac_parse_init:
 * @aacparse: #GstAacParse.
 * @klass: #GstAacParseClass.
 *
 */
static void
gst_aac_parse_init (GstAacParse * aacparse, GstAacParseClass * klass)
{
  GST_DEBUG ("initialized");
#ifdef GST_EXT_AACPARSER_MODIFICATION /* to get more correct duration */
  aacparse->first_frame = TRUE;
#endif
}


/**
 * gst_aac_parse_set_src_caps:
 * @aacparse: #GstAacParse.
 * @sink_caps: (proposed) caps of sink pad
 *
 * Set source pad caps according to current knowledge about the
 * audio stream.
 *
 * Returns: TRUE if caps were successfully set.
 */
static gboolean
gst_aac_parse_set_src_caps (GstAacParse * aacparse, GstCaps * sink_caps)
{
  GstStructure *s;
  GstCaps *src_caps = NULL;
  gboolean res = FALSE;
  const gchar *stream_format;

  GST_DEBUG_OBJECT (aacparse, "sink caps: %" GST_PTR_FORMAT, sink_caps);
  if (sink_caps)
    src_caps = gst_caps_copy (sink_caps);
  else
    src_caps = gst_caps_new_simple ("audio/mpeg", NULL);

  gst_caps_set_simple (src_caps, "framed", G_TYPE_BOOLEAN, TRUE,
      "mpegversion", G_TYPE_INT, aacparse->mpegversion, NULL);

  switch (aacparse->header_type) {
    case DSPAAC_HEADER_NONE:
      stream_format = "raw";
      break;
    case DSPAAC_HEADER_ADTS:
      stream_format = "adts";
      break;
    case DSPAAC_HEADER_ADIF:
      stream_format = "adif";
      break;
    default:
      stream_format = NULL;
  }

  s = gst_caps_get_structure (src_caps, 0);
  if (aacparse->sample_rate > 0)
    gst_structure_set (s, "rate", G_TYPE_INT, aacparse->sample_rate, NULL);
  if (aacparse->channels > 0)
    gst_structure_set (s, "channels", G_TYPE_INT, aacparse->channels, NULL);
  if (stream_format)
    gst_structure_set (s, "stream-format", G_TYPE_STRING, stream_format, NULL);

  GST_DEBUG_OBJECT (aacparse, "setting src caps: %" GST_PTR_FORMAT, src_caps);

  res = gst_pad_set_caps (GST_BASE_PARSE (aacparse)->srcpad, src_caps);
  gst_caps_unref (src_caps);
  return res;
}


/**
 * gst_aac_parse_sink_setcaps:
 * @sinkpad: GstPad
 * @caps: GstCaps
 *
 * Implementation of "set_sink_caps" vmethod in #GstBaseParse class.
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_aac_parse_sink_setcaps (GstBaseParse * parse, GstCaps * caps)
{
  GstAacParse *aacparse;
  GstStructure *structure;
  gchar *caps_str;
  const GValue *value;

  aacparse = GST_AAC_PARSE (parse);
  structure = gst_caps_get_structure (caps, 0);
  caps_str = gst_caps_to_string (caps);

  GST_DEBUG_OBJECT (aacparse, "setcaps: %s", caps_str);
  g_free (caps_str);

  /* This is needed at least in case of RTP
   * Parses the codec_data information to get ObjectType,
   * number of channels and samplerate */
  value = gst_structure_get_value (structure, "codec_data");
  if (value) {
    GstBuffer *buf = gst_value_get_buffer (value);

    if (buf) {
      const guint8 *buffer = GST_BUFFER_DATA (buf);
      guint sr_idx;

      sr_idx = ((buffer[0] & 0x07) << 1) | ((buffer[1] & 0x80) >> 7);
      aacparse->object_type = (buffer[0] & 0xf8) >> 3;
      aacparse->sample_rate = gst_aac_parse_get_sample_rate_from_index (sr_idx);
      aacparse->channels = (buffer[1] & 0x78) >> 3;
      aacparse->header_type = DSPAAC_HEADER_NONE;
      aacparse->mpegversion = 4;
      aacparse->frame_samples = (buffer[1] & 4) ? 960 : 1024;

      GST_DEBUG ("codec_data: object_type=%d, sample_rate=%d, channels=%d, "
          "samples=%d", aacparse->object_type, aacparse->sample_rate,
          aacparse->channels, aacparse->frame_samples);

      /* arrange for metadata and get out of the way */
      gst_aac_parse_set_src_caps (aacparse, caps);
      gst_base_parse_set_passthrough (parse, TRUE);
    } else
      return FALSE;

    /* caps info overrides */
    gst_structure_get_int (structure, "rate", &aacparse->sample_rate);
    gst_structure_get_int (structure, "channels", &aacparse->channels);
  } else {
    gst_base_parse_set_passthrough (parse, FALSE);
  }

  return TRUE;
}


/**
 * gst_aac_parse_adts_get_frame_len:
 * @data: block of data containing an ADTS header.
 *
 * This function calculates ADTS frame length from the given header.
 *
 * Returns: size of the ADTS frame.
 */
static inline guint
gst_aac_parse_adts_get_frame_len (const guint8 * data)
{
  return ((data[3] & 0x03) << 11) | (data[4] << 3) | ((data[5] & 0xe0) >> 5);
}


/**
 * gst_aac_parse_check_adts_frame:
 * @aacparse: #GstAacParse.
 * @data: Data to be checked.
 * @avail: Amount of data passed.
 * @framesize: If valid ADTS frame was found, this will be set to tell the
 *             found frame size in bytes.
 * @needed_data: If frame was not found, this may be set to tell how much
 *               more data is needed in the next round to detect the frame
 *               reliably. This may happen when a frame header candidate
 *               is found but it cannot be guaranteed to be the header without
 *               peeking the following data.
 *
 * Check if the given data contains contains ADTS frame. The algorithm
 * will examine ADTS frame header and calculate the frame size. Also, another
 * consecutive ADTS frame header need to be present after the found frame.
 * Otherwise the data is not considered as a valid ADTS frame. However, this
 * "extra check" is omitted when EOS has been received. In this case it is
 * enough when data[0] contains a valid ADTS header.
 *
 * This function may set the #needed_data to indicate that a possible frame
 * candidate has been found, but more data (#needed_data bytes) is needed to
 * be absolutely sure. When this situation occurs, FALSE will be returned.
 *
 * When a valid frame is detected, this function will use
 * gst_base_parse_set_min_frame_size() function from #GstBaseParse class
 * to set the needed bytes for next frame.This way next data chunk is already
 * of correct size.
 *
 * Returns: TRUE if the given data contains a valid ADTS header.
 */
static gboolean
gst_aac_parse_check_adts_frame (GstAacParse * aacparse,
    const guint8 * data, const guint avail, gboolean drain,
    guint * framesize, guint * needed_data)
{
  if (G_UNLIKELY (avail < 2))
    return FALSE;

  if ((data[0] == 0xff) && ((data[1] & 0xf6) == 0xf0)) {
    *framesize = gst_aac_parse_adts_get_frame_len (data);

    /* In EOS mode this is enough. No need to examine the data further.
       We also relax the check when we have sync, on the assumption that
       if we're not looking at random data, we have a much higher chance
       to get the correct sync, and this avoids losing two frames when
       a single bit corruption happens. */
    if (drain || !GST_BASE_PARSE_LOST_SYNC (aacparse)) {
      return TRUE;
    }

    if (*framesize + ADTS_MAX_SIZE > avail) {
      /* We have found a possible frame header candidate, but can't be
         sure since we don't have enough data to check the next frame */
      GST_DEBUG ("NEED MORE DATA: we need %d, available %d",
          *framesize + ADTS_MAX_SIZE, avail);
      *needed_data = *framesize + ADTS_MAX_SIZE;
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
          *framesize + ADTS_MAX_SIZE);
      return FALSE;
    }

    if ((data[*framesize] == 0xff) && ((data[*framesize + 1] & 0xf6) == 0xf0)) {
      guint nextlen = gst_aac_parse_adts_get_frame_len (data + (*framesize));

      aacparse->frame_byte = ((*framesize) + nextlen) / 2;
      GST_LOG ("ADTS frame found, len: %d bytes", *framesize);
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
          nextlen + ADTS_MAX_SIZE);
      return TRUE;
    }
  }
  return FALSE;
}

/* caller ensure sufficient data */
static inline void
gst_aac_parse_parse_adts_header (GstAacParse * aacparse, const guint8 * data,
    gint * rate, gint * channels, gint * object, gint * version)
{

  if (rate) {
    gint sr_idx = (data[2] & 0x3c) >> 2;

    *rate = gst_aac_parse_get_sample_rate_from_index (sr_idx);
  }
  if (channels)
    *channels = ((data[2] & 0x01) << 2) | ((data[3] & 0xc0) >> 6);

  if (version)
    *version = (data[1] & 0x08) ? 2 : 4;
  if (object)
    *object = (data[2] & 0xc0) >> 6;
}
static inline guint aac_get_bits(guint8* data, guint* bit_pos, guint num)
{
    guint ret_val;
    guint byte_pos = (*bit_pos) >> 3;
    guint cur_bit = (*bit_pos) & 0x7;
    guint bit_buffer = ((guint)(data[byte_pos]) << 8) | data[byte_pos + 1];
    guint r_shift_val = 16 + cur_bit;
    guint l_shift_val = 32 - num;

    (*bit_pos) = (*bit_pos) + num;
    ret_val = ((bit_buffer << r_shift_val) >> l_shift_val);

    GST_INFO("curr byte %i, bit %i :", byte_pos, cur_bit);
    GST_INFO("getbits num %i, ret %i\n", num, ret_val);
    return ret_val;
}

static inline void aac_skip_bits(guint* bit_pos, guint num)
{
    guint byte_pos = (*bit_pos) >> 3;
    guint cur_bit = (*bit_pos) & 0x7;

    GST_INFO("curr byte %i, bit %i :", byte_pos, cur_bit);
    GST_INFO("skipbits num %i\n", num);
    (*bit_pos) = (*bit_pos) + num;
}

static inline void aac_byte_alignment(guint* bit_pos)
{
    guint byte_pos = (*bit_pos) >> 3;
    guint cur_bit = (*bit_pos) & 0x7;

    GST_INFO("curr byte %i, bit %i :", byte_pos, cur_bit);
    GST_INFO("byte_alignment\n");
    (*bit_pos) = (*bit_pos) + 7;
    (*bit_pos) = (*bit_pos) & 0xFFffFFf8;
}

static guint skip_aac_read_adif_header(guint8* data)
{
    guint val, num_pce;
    guint bit_pos = 0;
    guint read_bytes;

    /* adif_id; 32 bslbf */
    aac_skip_bits(&bit_pos, 32);

    /* copyright_id_present; 1 bslbf */
    val = aac_get_bits(data, &bit_pos, 1);
    if(val)
    {
        /* copyright_id; 72 bslbf */
        aac_skip_bits(&bit_pos, 72);
    }

    /* original_copy; 1 bslbf */
    aac_skip_bits(&bit_pos, 1);

    /* home; 1 bslbf */
    aac_skip_bits(&bit_pos, 1);

    /* bitstream_type; 1 bslbf */
    val = aac_get_bits(data, &bit_pos, 1);

    /* bitrate; 23 uimsbf */
    aac_get_bits(data, &bit_pos, 23);

    /* num_program_config_elements; 4 bslbf */
    num_pce = aac_get_bits(data, &bit_pos, 4) + 1;

    if(val == 0)
    {
        /* adif_buffer_fullness; 20 uimsbf */
        aac_skip_bits(&bit_pos, 20);
    }

    for(; num_pce; num_pce--)
    {
        gint sr_index;
        guint num_fce, num_sce, num_bce, num_lce, num_ade, num_vce, i;

        /* element_instance_tag; 4 uimsbf */
        aac_skip_bits(&bit_pos, 4);

        /* profile; 2 uimsbf */
        aac_skip_bits(&bit_pos, 2);

        /* sampling_frequency_index; 4 uimsbf */
        sr_index = aac_get_bits(data, &bit_pos, 4);

        /* num_front_channel_elements; 4 uimsbf */
        num_fce = aac_get_bits(data, &bit_pos, 4);

        /* num_side_channel_elements; 4 uimsbf */
        num_sce = aac_get_bits(data, &bit_pos, 4);

        /* num_back_channel_elements; 4 uimsbf */
        num_bce = aac_get_bits(data, &bit_pos, 4);

        /* num_lfe_channel_elements; 2 uimsbf */
        num_lce = aac_get_bits(data, &bit_pos, 2);

        /* num_assoc_data_elements; 3 uimsbf */
        num_ade = aac_get_bits(data, &bit_pos, 3);

        /* num_valid_cc_elements; 4 uimsbf */
        num_vce = aac_get_bits(data, &bit_pos, 4);

        /* mono_mixdown_present; 1 uimsbf */
        val = aac_get_bits(data, &bit_pos, 1);
        if(val == 1)
        {
            /* mono_mixdown_element_number; 4 uimsbf */
            aac_skip_bits(&bit_pos, 4);
        }

        /* stereo_mixdown_present; 1 uimsbf */
        val = aac_get_bits(data, &bit_pos, 1);
        if(val == 1)
        {
            /* stereo_mixdown_element_number; 4 uimsbf */
            aac_skip_bits(&bit_pos, 4);
        }

        /* matrix_mixdown_idx_present; 1 uimsbf */
        val = aac_get_bits(data, &bit_pos, 1);
        if(val == 1)
        {
            /* matrix_mixdown_idx ; 2 uimsbf */
            aac_skip_bits(&bit_pos, 2);

            /* pseudo_surround_enable; 1 uimsbf */
            aac_skip_bits(&bit_pos, 1);
        }

        for(i = 0; i < num_fce; i++)
        {
            /* front_element_is_cpe[i]; 1 bslbf */
            aac_skip_bits(&bit_pos, 1);

            /* front_element_tag_select[i]; 4 uimsbf */
            aac_skip_bits(&bit_pos, 4);
        }

        for(i = 0; i < num_sce; i++)
        {
            /* side_element_is_cpe[i]; 1 bslbf */
            aac_skip_bits(&bit_pos, 1);

            /* side_element_tag_select[i]; 4 uimsbf */
            aac_skip_bits(&bit_pos, 4);
        }

        for(i = 0; i < num_bce; i++)
        {
            /* back_element_is_cpe[i]; 1 bslbf */
            aac_skip_bits(&bit_pos, 1);

            /* back_element_tag_select[i]; 4 uimsbf */
            aac_skip_bits(&bit_pos, 4);
        }

        for(i = 0; i < num_lce; i++)
        {
            /* lfe_element_tag_select[i]; 4 uimsbf */
            aac_skip_bits(&bit_pos, 4);
        }

        for(i = 0; i < num_ade; i++)
        {
            /* assoc_data_element_tag_select[i]; 4 uimsbf */
            aac_skip_bits(&bit_pos, 4);
        }

        for(i = 0; i < num_vce; i++)
        {
            /* cc_element_is_ind_sw[i]; 1 uimsbf */
            aac_skip_bits(&bit_pos, 1);

            /* valid_cc_element_tag_select[i]; 4 uimsbf */
            aac_skip_bits(&bit_pos, 4);
        }

        aac_byte_alignment(&bit_pos);

        /* comment_field_bytes; 8 uimsbf */
        val = aac_get_bits(data, &bit_pos, 8);
        for(i = 0; i < val; i++)
        {
            /* comment_field_data[i]; 8 uimsbf */
            aac_skip_bits(&bit_pos, 8);
        }
    }

    /* aac_byte_alignment(&bit_pos); */
    read_bytes = bit_pos >> 3;

    GST_DEBUG("bytes_read %i", read_bytes);
    return read_bytes;
}
/**
 * gst_aac_parse_detect_stream:
 * @aacparse: #GstAacParse.
 * @data: A block of data that needs to be examined for stream characteristics.
 * @avail: Size of the given datablock.
 * @framesize: If valid stream was found, this will be set to tell the
 *             first frame size in bytes.
 * @skipsize: If valid stream was found, this will be set to tell the first
 *            audio frame position within the given data.
 *
 * Examines the given piece of data and try to detect the format of it. It
 * checks for "ADIF" header (in the beginning of the clip) and ADTS frame
 * header. If the stream is detected, TRUE will be returned and #framesize
 * is set to indicate the found frame size. Additionally, #skipsize might
 * be set to indicate the number of bytes that need to be skipped, a.k.a. the
 * position of the frame inside given data chunk.
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_aac_parse_detect_stream (GstAacParse * aacparse,
    const guint8 * data, const guint avail, gboolean drain,
    guint * framesize, gint * skipsize)
{
  gboolean found = FALSE;
  guint need_data = 0;
  guint i = 0;

  GST_DEBUG_OBJECT (aacparse, "Parsing header data");

  /* FIXME: No need to check for ADIF if we are not in the beginning of the
     stream */

  /* Can we even parse the header? */
  if (avail < ADTS_MAX_SIZE)
    return FALSE;

  for (i = 0; i < avail - 4; i++) {
    if (((data[i] == 0xff) && ((data[i + 1] & 0xf6) == 0xf0)) ||
        strncmp ((char *) data + i, "ADIF", 4) == 0) {
      found = TRUE;

      if (i) {
        /* Trick: tell the parent class that we didn't find the frame yet,
           but make it skip 'i' amount of bytes. Next time we arrive
           here we have full frame in the beginning of the data. */
        *skipsize = i;
        return FALSE;
      }
      break;
    }
  }
  if (!found) {
    if (i)
      *skipsize = i;
    return FALSE;
  }

  if (gst_aac_parse_check_adts_frame (aacparse, data, avail, drain,
          framesize, &need_data)) {
    gint rate, channels;

    GST_INFO ("ADTS ID: %d, framesize: %d", (data[1] & 0x08) >> 3, *framesize);

    aacparse->header_type = DSPAAC_HEADER_ADTS;
    gst_aac_parse_parse_adts_header (aacparse, data, &rate, &channels,
        &aacparse->object_type, &aacparse->mpegversion);

    gst_base_parse_set_frame_rate (GST_BASE_PARSE (aacparse), rate,
        aacparse->frame_samples, 2, 2);

    GST_DEBUG ("ADTS: samplerate %d, channels %d, objtype %d, version %d",
        rate, channels, aacparse->object_type, aacparse->mpegversion);

    gst_base_parse_set_syncable (GST_BASE_PARSE (aacparse), TRUE);

    return TRUE;
  } else if (need_data) {
    /* This tells the parent class not to skip any data */
    *skipsize = 0;
    return FALSE;
  }

  if (avail < ADIF_MAX_SIZE)
    return FALSE;

  if (memcmp (data + i, "ADIF", 4) == 0) {
    const guint8 *adif, *tmp_data;
    int skip_size = 0;
    int bitstream_type;
    int sr_idx;
   GstBuffer* buffer;

    aacparse->header_type = DSPAAC_HEADER_ADIF;
    aacparse->mpegversion = 4;

    /* Skip the "ADIF" bytes */
    adif = data + i + 4;

    /* copyright string */
    if (adif[0] & 0x80)
      skip_size += 9;           /* skip 9 bytes */

    aacparse->bitstream_type =  bitstream_type = adif[0 + skip_size] & 0x10; // Added bitstream_type to the struct.
    aacparse->bitrate =
        ((unsigned int) (adif[0 + skip_size] & 0x0f) << 19) |
        ((unsigned int) adif[1 + skip_size] << 11) |
        ((unsigned int) adif[2 + skip_size] << 3) |
        ((unsigned int) adif[3 + skip_size] & 0xe0);

    /* CBR */
    if (bitstream_type == 0) {
#if 0
      /* Buffer fullness parsing. Currently not needed... */
      guint num_elems = 0;
      guint fullness = 0;

      num_elems = (adif[3 + skip_size] & 0x1e);
      GST_INFO ("ADIF num_config_elems: %d", num_elems);

      fullness = ((unsigned int) (adif[3 + skip_size] & 0x01) << 19) |
          ((unsigned int) adif[4 + skip_size] << 11) |
          ((unsigned int) adif[5 + skip_size] << 3) |
          ((unsigned int) (adif[6 + skip_size] & 0xe0) >> 5);

      GST_INFO ("ADIF buffer fullness: %d", fullness);
#endif
      aacparse->object_type = ((adif[6 + skip_size] & 0x01) << 1) |
          ((adif[7 + skip_size] & 0x80) >> 7);
      sr_idx = (adif[7 + skip_size] & 0x78) >> 3;
    }
    /* VBR */
    else {
      aacparse->object_type = (adif[4 + skip_size] & 0x18) >> 3;
      sr_idx = ((adif[4 + skip_size] & 0x07) << 1) |
          ((adif[5 + skip_size] & 0x80) >> 7);
    }

    /* FIXME: This gives totally wrong results. Duration calculation cannot
       be based on this */
    aacparse->sample_rate = gst_aac_parse_get_sample_rate_from_index (sr_idx);

    /* baseparse is not given any fps,
     * so it will give up on timestamps, seeking, etc */

    /* FIXME: Can we assume this? */
    aacparse->channels = 2;

    GST_INFO ("ADIF: br=%d, samplerate=%d, objtype=%d",
        aacparse->bitrate, aacparse->sample_rate, aacparse->object_type);

    gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse), 512);

    /* arrange for metadata and get out of the way */
    gst_aac_parse_set_src_caps (aacparse,
        GST_PAD_CAPS (GST_BASE_PARSE_SINK_PAD (aacparse)));

    gst_base_parse_set_syncable (GST_BASE_PARSE_CAST (aacparse), TRUE);
    gst_base_parse_set_passthrough (GST_BASE_PARSE_CAST (aacparse), FALSE);
    gst_base_parse_set_average_bitrate (GST_BASE_PARSE_CAST (aacparse), aacparse->bitrate);

    aacparse->read_bytes = skip_aac_read_adif_header(data);

    tmp_data = data + aacparse->read_bytes;

    *framesize = ((int)tmp_data [2] << 8) + tmp_data [3] + 4;
    return TRUE;
  }

  /* This should never happen */
  return FALSE;
}


/**
 * gst_aac_parse_check_valid_frame:
 * @parse: #GstBaseParse.
 * @buffer: #GstBuffer.
 * @framesize: If the buffer contains a valid frame, its size will be put here
 * @skipsize: How much data parent class should skip in order to find the
 *            frame header.
 *
 * Implementation of "check_valid_frame" vmethod in #GstBaseParse class.
 *
 * Returns: TRUE if buffer contains a valid frame.
 */
static gboolean
gst_aac_parse_check_valid_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, guint * framesize, gint * skipsize)
{
  const guint8 *data;
  GstAacParse *aacparse;
  gboolean ret = FALSE;
  gboolean lost_sync;
  GstBuffer *buffer;

  aacparse = GST_AAC_PARSE (parse);
  buffer = frame->buffer;
  data = GST_BUFFER_DATA (buffer);

  lost_sync = GST_BASE_PARSE_LOST_SYNC (parse);

  if (aacparse->header_type == DSPAAC_HEADER_ADIF ||
      aacparse->header_type == DSPAAC_HEADER_NONE) {
    *framesize = ((int)data [2] << 8) + data [3] + 4;
    ret = TRUE;

  } else if (aacparse->header_type == DSPAAC_HEADER_NOT_PARSED || lost_sync) {

    ret = gst_aac_parse_detect_stream (aacparse, data, GST_BUFFER_SIZE (buffer),
        GST_BASE_PARSE_DRAINING (parse), framesize, skipsize);

  } else if (aacparse->header_type == DSPAAC_HEADER_ADTS) {
    guint needed_data = 1024;

    ret = gst_aac_parse_check_adts_frame (aacparse, data,
        GST_BUFFER_SIZE (buffer), GST_BASE_PARSE_DRAINING (parse),
        framesize, &needed_data);

    if (!ret) {
      GST_DEBUG ("buffer didn't contain valid frame");
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
          needed_data);
    }

  } else {
    GST_DEBUG ("buffer didn't contain valid frame");
    gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
        ADTS_MAX_SIZE);
  }

  return ret;
}


#ifdef GST_EXT_AACPARSER_MODIFICATION /* to get more correct duration */
/**
 * get_aac_parse_get_adts_framelength:
 * @data: #GstBufferData.
 * @offset: #GstBufferData offset
 *
 * Implementation to get adts framelength by using first some frame.
 *
 * Returns: frame size
 */
int get_aac_parse_get_adts_frame_length (const unsigned char* data, gint64 offset)
{
  const gint adts_header_length_no_crc = 7;
  const gint adts_header_length_with_crc = 9;
  gint frame_size = 0;
  gint protection_absent;
  gint head_size;

  /* check of syncword */
  if ((data[offset+0] != 0xff) || ((data[offset+1] & 0xf6) != 0xf0)) {
    GST_ERROR("check sync word is fail\n");
    return  -1;
  }

  /* check of protection absent */
  protection_absent = (data[offset+1] & 0x01);

  /*check of frame length */
  frame_size = (data[offset+3] & 0x3) << 11 | data[offset+4] << 3 | data[offset+5] >> 5;

  /* check of header size */
  /* protectionAbsent is 0 if there is CRC */
  head_size = protection_absent ? adts_header_length_no_crc : adts_header_length_with_crc;
  if (head_size > frame_size) {
    GST_ERROR("return frame length as 0 (frameSize %u < headSize %u)", frame_size, head_size);
    return 0;
  }

  return frame_size;
}

/**
 * gst_aac_parse_estimate_duration:
 * @parse: #GstBaseParse.
 *
 * Implementation to get estimated total duration by using first some frame.
 *
 * Returns: TRUE if we can get estimated total duraion
 */
static gboolean
gst_aac_parse_estimate_duration (GstBaseParse * parse)
{
  GstFlowReturn res = GST_FLOW_OK;
  gint64 pull_size = 0, file_size = 0, offset = 0, num_frames=0, duration=0;
  guint profile = 0, sample_rate_index = 0, sample_rate = 0, channel = 0;
  guint frame_size = 0, frame_duration_us = 0, estimated_bitrate = 0;
  guint  lost_sync_count=0;
  GstClockTime estimated_duration = GST_CLOCK_TIME_NONE;
  GstBuffer *buffer = NULL;
  guint8 *buf = NULL;
  gint i = 0;
  GstActivateMode pad_mode = GST_ACTIVATE_NONE;
  GstAacParse *aacparse;
  gint64 buffer_size = 0;

  aacparse = GST_AAC_PARSE (parse);
  GST_LOG_OBJECT (aacparse, "gst_aac_parse_estimate_duration enter");

#ifdef GST_EXT_BASEPARSER_MODIFICATION /* check baseparse define these fuction */
  gst_base_parse_get_pad_mode(parse, &pad_mode);
  if (pad_mode != GST_ACTIVATE_PULL) {
    GST_INFO_OBJECT (aacparse, "aac parser is not pull mode. can not estimate duration");
    return FALSE;
  }

  gst_base_parse_get_upstream_size (parse, &file_size);
#else
  GST_WARNING_OBJECT (aacparse, "baseparser does not define get private param functions");
  return FALSE;
#endif

  if (file_size < ADIF_MAX_SIZE) {
    GST_ERROR_OBJECT (aacparse, "file size is too short");
    return FALSE;
  }

  pull_size = MIN(file_size, AAC_MAX_ESTIMATE_DURATION_BUF);

  res = gst_pad_pull_range (parse->sinkpad, 0, pull_size, &buffer);
  if (res != GST_FLOW_OK) {
    GST_ERROR_OBJECT (aacparse, "gst_pad_pull_range failed!");
    return FALSE;
  }

  buf = GST_BUFFER_DATA(buffer);
  buffer_size = GST_BUFFER_SIZE(buffer);
  if(buffer_size != pull_size)
  {
    GST_ERROR_OBJECT(aacparse, "We got different buffer_size(%d) with pull_size(%d).", buffer_size, pull_size);
  }
  /*MODIFICATION : add defence codes for real buffer_size is different with pull_size*/
  //for (i = 0; i < pull_size; i ++) {
  for (i = 0; i < buffer_size; i ++) {
    if ((buf[i] == 0xff) && ((buf[i+1] & 0xf6) == 0xf0)) { /* aac sync word */
      profile = (buf[i+2] >> 6) & 0x3;
      sample_rate_index = (buf[i+2] >> 2) & 0xf;
      sample_rate = gst_aac_parse_get_sample_rate_from_index(sample_rate_index);
      if (sample_rate == 0) {
          GST_WARNING_OBJECT (aacparse, "Invalid sample rate index (0)");
          return FALSE;
      }
      channel = (buf[i+2] & 0x1) << 2 | (buf[i+3] >> 6);

      GST_INFO_OBJECT (aacparse, "found sync. aac sample_rate=%d, channel=%d", sample_rate, channel);

      /* count number of frames */
      /*MODIFICATION : add defence codes for real buffer_size is different with pull_size*/
      //while (offset < pull_size) {
      while (offset < buffer_size) {
        frame_size = get_aac_parse_get_adts_frame_length(buf, i + offset);
        if (frame_size  == 0) {
          GST_ERROR_OBJECT (aacparse, "framesize error at offset %"G_GINT64_FORMAT, offset);
          break;
        } else if (frame_size == -1) {
          offset++;
          lost_sync_count++;    //  lost sync count limmitation 2K Bytes
          if (lost_sync_count > (1024*2))
            return FALSE;
        } else {
          offset += frame_size;
          num_frames++;
          lost_sync_count=0;
        }
      } /* while */

      /* if we can got full file, we can calculate the accurate duration */
      /*MODIFICATION : add defence codes for real buffer_size is different with pull_size*/
      //if (pull_size == file_size) {
      if (buffer_size == file_size) {
        gfloat duration_for_one_frame = 0;
        GstClockTime calculated_duration = GST_CLOCK_TIME_NONE;

        GST_INFO_OBJECT (aacparse, "we got total file (%d bytes). do not estimate but make Accurate total duration.", pull_size);

        duration_for_one_frame = (gfloat)AAC_SAMPLE_PER_FRAME / (gfloat)sample_rate;
        calculated_duration = num_frames * duration_for_one_frame * 1000 * 1000 * 1000;

        GST_INFO_OBJECT (aacparse, "duration_for_one_frame %f ms", duration_for_one_frame);
        GST_INFO_OBJECT (aacparse, "calculated duration = %"GST_TIME_FORMAT, GST_TIME_ARGS(calculated_duration));
        gst_base_parse_set_duration (parse, GST_FORMAT_TIME, calculated_duration, 0); /* 0 means disable estimate */

      } else {
        GST_INFO_OBJECT (aacparse, "we got %d bytes in total file (%"G_GINT64_FORMAT
            "). can not make accurate duration but Estimate.", pull_size, file_size);
        frame_duration_us = (1024 * 1000000ll + (sample_rate - 1)) / sample_rate;
        duration = num_frames * frame_duration_us;

        estimated_bitrate = (gint)((gfloat)(offset * 8) / (gfloat)(duration / 1000));
        estimated_duration = (GstClockTime)((file_size * 8) / (estimated_bitrate * 1000)) * GST_SECOND;

        GST_INFO_OBJECT (aacparse, "number of frame = %"G_GINT64_FORMAT, num_frames);
        GST_INFO_OBJECT (aacparse, "duration = %"G_GINT64_FORMAT, duration / 1000000);
        GST_INFO_OBJECT (aacparse, "byte = %"G_GINT64_FORMAT, offset);
        GST_INFO_OBJECT (aacparse, "estimated bitrate = %d bps", estimated_bitrate);
        GST_INFO_OBJECT (aacparse, "estimated duration = %"GST_TIME_FORMAT, GST_TIME_ARGS(estimated_duration));

        gst_base_parse_set_average_bitrate (parse, estimated_bitrate * 1000);
        /* set update_interval as duration(sec)/2 */
        gst_base_parse_set_duration (parse, GST_FORMAT_TIME, estimated_duration, (gint)(duration/2));
      }

      break;
    }
  }

  gst_buffer_unref (buffer);
  return TRUE;
}
#endif


/**
 * gst_aac_parse_parse_frame:
 * @parse: #GstBaseParse.
 * @buffer: #GstBuffer.
 *
 * Implementation of "parse_frame" vmethod in #GstBaseParse class.
 *
 * Also determines frame overhead.
 * ADTS streams have a 7 byte header in each frame. MP4 and ADIF streams don't have
 * a per-frame header.
 *
 * We're making a couple of simplifying assumptions:
 *
 * 1. We count Program Configuration Elements rather than searching for them
 *    in the streams to discount them - the overhead is negligible.
 *
 * 2. We ignore CRC. This has a worst-case impact of (num_raw_blocks + 1)*16
 *    bits, which should still not be significant enough to warrant the
 *    additional parsing through the headers
 *
 * Returns: GST_FLOW_OK if frame was successfully parsed and can be pushed
 *          forward. Otherwise appropriate error is returned.
 */
static GstFlowReturn
gst_aac_parse_parse_frame (GstBaseParse * parse, GstBaseParseFrame * frame)
{
  GstAacParse *aacparse;
  GstBuffer *buffer;
  GstFlowReturn ret = GST_FLOW_OK;
  gint rate, channels;

  aacparse = GST_AAC_PARSE (parse);
  buffer = frame->buffer;

  if(aacparse->header_type == DSPAAC_HEADER_ADTS)
  {
	  /* see above */
	  frame->overhead = 7;

  gst_aac_parse_parse_adts_header (aacparse, GST_BUFFER_DATA (buffer),
      &rate, &channels, NULL, NULL);
  GST_LOG_OBJECT (aacparse, "rate: %d, chans: %d", rate, channels);

  if (G_UNLIKELY (rate != aacparse->sample_rate
          || channels != aacparse->channels)) {
    aacparse->sample_rate = rate;
    aacparse->channels = channels;

    if (!gst_aac_parse_set_src_caps (aacparse,
            GST_PAD_CAPS (GST_BASE_PARSE (aacparse)->sinkpad))) {
      /* If linking fails, we need to return appropriate error */
      ret = GST_FLOW_NOT_LINKED;
    }

    gst_base_parse_set_frame_rate (GST_BASE_PARSE (aacparse),
        aacparse->sample_rate, aacparse->frame_samples, 2, 2);
  }

#ifdef GST_EXT_AACPARSER_MODIFICATION /* to get more correct duration */
  if (aacparse->first_frame == TRUE) {
    gboolean ret = FALSE;
    aacparse->first_frame = FALSE;

    ret = gst_aac_parse_estimate_duration(parse);
    if (!ret) {
      GST_WARNING_OBJECT (aacparse, "can not estimate total duration");
      ret = GST_FLOW_NOT_SUPPORTED;
    }
  }
#endif
  }
  else if(aacparse->header_type == DSPAAC_HEADER_ADIF /* DSPAAC_HEADER_NOT_PARSED */)
  {
    int file_size = 0;
    float estimated_duration = 0;
    GstBuffer *buffer = NULL;
    gint64 total_file_size;
    const guint8 *data;
    data = GST_BUFFER_DATA (buffer);
	  if (G_UNLIKELY (rate != aacparse->sample_rate || channels != aacparse->channels)) {
      GST_DEBUG("ADIF: Sampling Rate = %d", aacparse->sample_rate);
      if (!gst_aac_parse_set_src_caps (aacparse, GST_PAD_CAPS (GST_BASE_PARSE (aacparse)->sinkpad))) {
        /* If linking fails, we need to return appropriate error */
        ret = GST_FLOW_NOT_LINKED;
      }
    }
    gst_base_parse_get_upstream_size(parse, &total_file_size);

    estimated_duration = ((total_file_size * 8) / (float)(aacparse->bitrate * 1000))* GST_SECOND;
    aacparse->file_size = total_file_size;

    gst_base_parse_set_average_bitrate (parse, aacparse->bitrate);
    gst_base_parse_set_duration (parse, GST_FORMAT_TIME, estimated_duration * 1000, 0);

    return GST_FLOW_OK;
  }
  return ret;
}


/**
 * gst_aac_parse_start:
 * @parse: #GstBaseParse.
 *
 * Implementation of "start" vmethod in #GstBaseParse class.
 *
 * Returns: TRUE if startup succeeded.
 */
static gboolean
gst_aac_parse_start (GstBaseParse * parse)
{
  GstAacParse *aacparse;

  aacparse = GST_AAC_PARSE (parse);
  GST_DEBUG ("start");
  aacparse->frame_samples = 1024;
  gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse), ADTS_MAX_SIZE);
  return TRUE;
}


/**
 * gst_aac_parse_stop:
 * @parse: #GstBaseParse.
 *
 * Implementation of "stop" vmethod in #GstBaseParse class.
 *
 * Returns: TRUE is stopping succeeded.
 */
static gboolean
gst_aac_parse_stop (GstBaseParse * parse)
{
  GST_DEBUG ("stop");
  return TRUE;
}

static GstCaps *
gst_aac_parse_sink_getcaps (GstBaseParse * parse)
{
  GstCaps *peercaps;
  GstCaps *res;

  peercaps = gst_pad_get_allowed_caps (GST_BASE_PARSE_SRC_PAD (parse));
  if (peercaps) {
    guint i, n;

    /* Remove the framed field */
    peercaps = gst_caps_make_writable (peercaps);
    n = gst_caps_get_size (peercaps);
    for (i = 0; i < n; i++) {
      GstStructure *s = gst_caps_get_structure (peercaps, i);

      gst_structure_remove_field (s, "framed");
    }

    res =
        gst_caps_intersect_full (peercaps,
        gst_pad_get_pad_template_caps (GST_BASE_PARSE_SRC_PAD (parse)),
        GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (peercaps);
  } else {
    res =
        gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_PARSE_SINK_PAD
            (parse)));
  }

  return res;
}


#ifdef GST_EXT_AACPARSER_MODIFICATION
/* perform seek in push based mode:
   find BYTE position to move to based on time and delegate to upstream
*/
static gboolean
gst_aac_audio_parse_do_push_seek (GstBaseParse * parse, GstPad * pad, GstEvent * event)
{
  GstAacParse *aacparse;
  aacparse = GST_AAC_PARSE(parse);

  gdouble rate;
  GstFormat format;
  GstSeekFlags flags;
  GstSeekType cur_type, stop_type;
  gint64 cur, stop;
  gboolean res;
  gint64 byte_cur;
  gint64 esimate_byte;
  gint32 frame_dur;
  gint64 upstream_total_bytes = 0;
  GstFormat fmt = GST_FORMAT_BYTES;
  
  GST_INFO_OBJECT (parse, "doing aac push-based seek");

  gst_event_parse_seek (event, &rate, &format, &flags, &cur_type, &cur, &stop_type, &stop);

  GST_INFO_OBJECT(parse, " rate =%.2f,  flags = %d , cur_type =%d, cur = %lld, stop_type=%d, stop=%lld", rate, (int)flags, (int)cur_type, cur , (int)stop_type , stop);

  /* FIXME, always play to the end */
  stop = -1;

  /* only forward streaming and seeking is possible */
	if (rate <= 0)
	{
		GST_WARNING_OBJECT(parse, " failed to seek, rate <= 0 !!!");
		goto unsupported_seek;
	}

  GST_INFO_OBJECT(parse, "aacparse->sample_rate = %d, aacparse->frame_byte=%u, aacparse->spf=%u, aacparse->sample_rate=%d",
	aacparse->sample_rate, aacparse->frame_byte , aacparse->spf,aacparse->sample_rate );

  if ( cur == 0 ) {
    /* handle rewind only */
    cur_type = GST_SEEK_TYPE_SET;
    byte_cur = 0;
    stop_type = GST_SEEK_TYPE_NONE;
    stop = -1;
    flags |= GST_SEEK_FLAG_FLUSH;
  } else {
    /* handle normal seek */
    cur_type = GST_SEEK_TYPE_SET;
    stop_type = GST_SEEK_TYPE_NONE;
    stop = -1;
    flags |= GST_SEEK_FLAG_FLUSH;

    // aacparse->spf is not getting initialized , not updated anywhere in case of push seek case - yoserb.yi
	aacparse->spf = aacparse->frame_samples;

	esimate_byte = (cur / (1000 * 1000)) * aacparse->frame_byte;
    if (aacparse->sample_rate> 0)
      frame_dur = (aacparse->spf * 1000) / aacparse->sample_rate;
    else
      {
		GST_WARNING_OBJECT(parse," failed to seek, aacparse->sample_rate <0 !!!!!");
		goto unsupported_seek;
	}
    if (frame_dur > 0)
      byte_cur =  esimate_byte / (frame_dur);
    else
	{
		GST_WARNING_OBJECT(parse," failed to seek, frame_dur <0 !!!!!");
		goto unsupported_seek;
	}

    GST_INFO_OBJECT(parse, "frame_byte(%d) spf(%d)  rate (%d) ", aacparse->frame_byte, aacparse->spf, aacparse->sample_rate);
    GST_INFO_OBJECT(parse, "seek cur (%"G_GINT64_FORMAT") = (%"GST_TIME_FORMAT") ", cur, GST_TIME_ARGS (cur));
    GST_INFO_OBJECT(parse, "esimate_byte(%"G_GINT64_FORMAT")  esimate_byte (%d)", esimate_byte, frame_dur );
  }

  /* obtain real upstream total bytes */
  if (!gst_pad_query_peer_duration (GST_BASE_PARSE_SINK_PAD (GST_BASE_PARSE
              (aacparse)), &fmt, &upstream_total_bytes))
    upstream_total_bytes = 0;
    GST_INFO_OBJECT (aacparse, "gst_pad_query_peer_duration -upstream_total_bytes (%"G_GUINT64_FORMAT")", upstream_total_bytes);
    aacparse->file_size = upstream_total_bytes;

  if ( (byte_cur == -1) || (byte_cur > aacparse->file_size))
  {
    GST_INFO_OBJECT(parse, "[WEB-ERROR] seek cur (%"G_GINT64_FORMAT") > file_size (%"G_GINT64_FORMAT") ", cur, aacparse->file_size );
    goto abort_seek;
  }

  GST_INFO_OBJECT (parse, "Pushing BYTE seek rate %g, "  "start %" G_GINT64_FORMAT ", stop %" G_GINT64_FORMAT, rate, byte_cur,  stop);

  if (!(flags & GST_SEEK_FLAG_KEY_UNIT)) {
    GST_INFO_OBJECT (parse, "Requested seek time: %" GST_TIME_FORMAT ", calculated seek offset: %" G_GUINT64_FORMAT, GST_TIME_ARGS (cur), byte_cur);
  }

  /* BYTE seek event */
  event = gst_event_new_seek (rate, GST_FORMAT_BYTES, flags, cur_type, byte_cur, stop_type, stop);
  res = gst_pad_push_event (parse->sinkpad, event);

  return res;

  /* ERRORS */

abort_seek:
  {
    GST_DEBUG_OBJECT (parse, "could not determine byte position to seek to, " "seek aborted.");
    return FALSE;
  }

unsupported_seek:
  {
    GST_DEBUG_OBJECT (parse, "unsupported seek, seek aborted.");
    return FALSE;
  }
}


static guint
gst_aac_parse_adts_get_fast_frame_len (const guint8 * data)
{
  int length;
  if ((data[0] == 0xff) && ((data[1] & 0xf6) == 0xf0)) {
    length = ((data[3] & 0x03) << 11) | (data[4] << 3) | ((data[5] & 0xe0) >> 5);
  } else {
    length = 0;
  }
  return length;
}

// evil
static gboolean
gst_aac_parse_src_eventfunc(GstBaseParse * parse, GstEvent * event)
{
  gboolean  handled = FALSE;
  GstAacParse *aacparse;
  aacparse = GST_AAC_PARSE(parse);

  GST_DEBUG("Entering gst_aac_parse_src_eventfunc header type = %d", aacparse->header_type);
  if(aacparse->header_type == DSPAAC_HEADER_ADIF)
    return gst_aac_parse_adif_src_eventfunc(parse, event);
  else if(aacparse->header_type == DSPAAC_HEADER_ADTS)
    return gst_aac_parse_adts_src_eventfunc (parse, event);
  else
    goto aac_seek_null_exit;
aac_seek_null_exit:

  /* call baseparse src_event function to handle event */
  handled = GST_BASE_PARSE_CLASS (parent_class)->src_event (parse, event);
  return handled;
}

/**
 * gst_aac_parse_adts_src_eventfunc:
 * @parse: #GstBaseParse. #event
 *
 * before baseparse handles seek event, make full amr index table.
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_aac_parse_adts_src_eventfunc (GstBaseParse * parse, GstEvent * event)
{
  gboolean handled = FALSE;
  GstAacParse *aacparse;
  aacparse = GST_AAC_PARSE (parse);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      {
        GstFlowReturn res = GST_FLOW_OK;
        gint64 base_offset = 0, sync_offset = 0, cur = 0;
        gint32 frame_count = 1;    /* do not add first frame because it is already in index table */
        gint64 second_count = 0;   /* initial 1 second */
        gint64 total_file_size = 0, start_offset = 0;
        GstClockTime current_ts = GST_CLOCK_TIME_NONE;
        GstActivateMode pad_mode = GST_ACTIVATE_NONE;

#ifdef GST_EXT_BASEPARSER_MODIFICATION /* check baseparse define these fuction */
        gst_base_parse_get_pad_mode(parse, &pad_mode);
        if (pad_mode != GST_ACTIVATE_PULL) {
          gboolean ret = FALSE;
          GST_INFO_OBJECT (aacparse, "aac parser is PUSH MODE.");
          GstPad* srcpad = gst_element_get_pad(parse, "src");
          /* check NULL */
          ret = gst_aac_audio_parse_do_push_seek(parse, srcpad, event);
          gst_object_unref(srcpad);
          return ret;
        }
        gst_base_parse_get_upstream_size(parse, &total_file_size);
        gst_base_parse_get_index_last_offset(parse, &start_offset);
        gst_base_parse_get_index_last_ts(parse, &current_ts);
#else
        GST_ERROR("baseparser does not define get private param functions. can not make index table here.");
        break;
#endif

        GST_DEBUG("gst_aac_parse_adts_src_eventfunc GST_EVENT_SEEK enter");

        if (total_file_size == 0 || start_offset >= total_file_size) {
          GST_ERROR("last index offset %d is larger than file size %d", start_offset, total_file_size);
          break;
        }

        gst_event_parse_seek (event, NULL, NULL, NULL, NULL, &cur, NULL, NULL);
        if (cur <= current_ts) {
          GST_INFO("seek to %"GST_TIME_FORMAT" within index table %"GST_TIME_FORMAT". do not make index table",
              GST_TIME_ARGS(cur), GST_TIME_ARGS(current_ts));
          break;
        } else {
          GST_INFO("seek to %"GST_TIME_FORMAT" without index table %"GST_TIME_FORMAT". make index table",
              GST_TIME_ARGS(cur), GST_TIME_ARGS(current_ts));
        }

        GST_INFO("make AAC(ADTS) Index Table. file_size  = %"G_GINT64_FORMAT" last idx offset=%"G_GINT64_FORMAT
            ", last idx ts=%"GST_TIME_FORMAT, total_file_size, start_offset, GST_TIME_ARGS(current_ts));

        base_offset = start_offset; /* set base by start offset */
        second_count = current_ts + GST_SECOND; /* 1sec */

        /************************************/
        /* STEP 0: Setting parse information */
        /************************************/
          aacparse->spf = aacparse->frame_samples;
          aacparse->frame_duration = (aacparse->spf * 1000 * 100) / aacparse->sample_rate;  /* duration per frame (msec) */
          aacparse->frame_per_sec = (aacparse->sample_rate) / aacparse->spf;          /* frames per second (ea) */

        /************************************/
        /* STEP 1: MAX_PULL_RANGE_BUF cycle */
        /************************************/
        while (total_file_size - base_offset >= AAC_MAX_PULL_RANGE_BUF) {
          gint64 offset = 0;
          GstBuffer *buffer = NULL;
          guint8 *buf = NULL;

         GST_INFO("gst_pad_pull_range %d bytes (from %"G_GINT64_FORMAT") use max size", AAC_MAX_PULL_RANGE_BUF, base_offset);
          res = gst_pad_pull_range (parse->sinkpad, base_offset, base_offset + AAC_MAX_PULL_RANGE_BUF, &buffer);
          if (res != GST_FLOW_OK) {
            GST_ERROR ("gst_pad_pull_range failed!");
            break;
          }

          buf = GST_BUFFER_DATA(buffer);
          if (buf == NULL) {
            GST_WARNING("buffer is NULL in make aac seek table's STEP1");
            gst_buffer_unref (buffer);
            goto aac_seek_null_exit;
          }

          while (offset <= AAC_MAX_PULL_RANGE_BUF) {
            gint frame_size = 0;
            guint32 header;

            /* make sure the values in the frame header look sane */
            frame_size = gst_aac_parse_adts_get_fast_frame_len (buf);

            if ((frame_size > 0) && (frame_size < (AAC_MAX_PULL_RANGE_BUF - offset))) {
              if (current_ts > second_count)  { /* 1 sec == xx frames. we make idx per sec */
                gst_base_parse_add_index_entry (parse, base_offset +offset, current_ts, TRUE, TRUE); /* force */
                GST_DEBUG("Adding  index ts=%"GST_TIME_FORMAT" offset %"G_GINT64_FORMAT,
                    GST_TIME_ARGS(current_ts), base_offset + offset);
                second_count += GST_SECOND;    /* 1sec */
              }

              current_ts += (aacparse->frame_duration * GST_MSECOND) / 100; /* each frame is (frame_duration) ms */
              offset += frame_size;
              buf += frame_size;
              frame_count++;
            } else if (frame_size >= (AAC_MAX_PULL_RANGE_BUF - offset)) {
              GST_DEBUG("we need refill buffer");
              break;
            } else {
              GST_WARNING("we lost sync");
              buf++;
              offset++;
            }
          } /* while */
          base_offset = base_offset + offset;
          gst_buffer_unref (buffer);
        } /* end MAX buffer cycle */

        /*******************************/
        /* STEP 2: Remain Buffer cycle */
        /*******************************/
        if (total_file_size - base_offset > 0) {
          gint64 offset = 0;
          GstBuffer *buffer = NULL;
          guint8 *buf = NULL;

          GST_INFO("gst_pad_pull_range %"G_GINT64_FORMAT" bytes (from %"G_GINT64_FORMAT") use remain_buf size",
              total_file_size - base_offset, base_offset);
          res = gst_pad_pull_range (parse->sinkpad, base_offset, total_file_size, &buffer);
          if (res != GST_FLOW_OK) {
            GST_ERROR ("gst_pad_pull_range failed!");
            break;
          }

          buf = GST_BUFFER_DATA(buffer);
          if (buf == NULL) {
            GST_WARNING("buffer is NULL in make aac seek table's STEP2");
            gst_buffer_unref (buffer);
            goto aac_seek_null_exit;
          }

          while (base_offset + offset < total_file_size) {
            gint frame_size = 0;
            guint32 header;

            /* make sure the values in the frame header look sane */
            frame_size = gst_aac_parse_adts_get_fast_frame_len (buf);

            if ((frame_size > 0) && (frame_size <= (total_file_size - (base_offset + offset)))) {
              if (current_ts > second_count) {  /* 1 sec == xx frames. we make idx per sec */
                gst_base_parse_add_index_entry (parse, base_offset +offset, current_ts, TRUE, TRUE); /* force */
                GST_DEBUG("Adding  index ts=%"GST_TIME_FORMAT" offset %"G_GINT64_FORMAT,
                    GST_TIME_ARGS(current_ts), base_offset + offset);
                second_count += GST_SECOND;    /* 1sec */
              }

              current_ts += (aacparse->frame_duration * GST_MSECOND) / 100; /* each frame is (frame_duration) ms */
              offset += frame_size;
              buf += frame_size;
              frame_count++;
            } else if (frame_size == 0) {
              GST_DEBUG("Frame size is 0 so, Decoding end..");
              break;
            } else {
              GST_WARNING("we lost sync");
              buf++;
              offset++;
            }
          } /* while */

        gst_buffer_unref (buffer);
        } /* end remain_buf buffer cycle */

        GST_DEBUG("gst_aac_parse_adts_src_eventfunc GST_EVENT_SEEK leave");
      }
     break;

    default:
      break;
  }

aac_seek_null_exit:

  /* call baseparse src_event function to handle event */
  handled = GST_BASE_PARSE_CLASS (parent_class)->src_event (parse, event);

  return handled;
}

// evil (ADIF SEEK)
// Enter this function if the detected format is adif.
static gboolean
gst_aac_parse_adif_src_eventfunc (GstBaseParse * parse, GstEvent * event)
{
	// Case 1: Constant bit rate => Find the byte boundary using bit rate. Add these values as indices using gst_base_parse_add_index_entry.
	// Case 2: Variable bit rate => Need to parse the whole file here itself to find frame boundaries.
	//gboolean handled = FALSE;
	gint64 cur = 0;
	int offset = 0;
	gboolean handled = FALSE;
	GstBuffer* buffer;
	const guint8 * data;

	GstAacParse *aacparse;
	aacparse = GST_AAC_PARSE(parse);

	GST_DEBUG("gst_aac_parse_adif_src_eventfunc enter");

	aacparse->frame_duration = (aacparse->frame_samples* 1000 * 100) / aacparse->sample_rate;  /* duration per frame (msec) */

	switch (GST_EVENT_TYPE (event)) {
		case GST_EVENT_SEEK:
		{
			int base_offset = aacparse->read_bytes;
			int framesize = 0, frame_duration_us = 0, duration = 0, estimated_bitrate, estimated_duration;
			GstFlowReturn res = GST_FLOW_OK;
			static int framecount = 0;
		       GstClockTime current_ts = GST_CLOCK_TIME_NONE;

			GST_LOG_OBJECT(aacparse, "ADIF: Let us Handle seek event");

			// Parse the event and get the timestamp where to seek.
			//gst_event_parse_seek (event, NULL, NULL, NULL, NULL, &cur, NULL, NULL);

			res = gst_pad_pull_range (parse->sinkpad, base_offset, aacparse->file_size, &buffer);

			if (res != GST_FLOW_OK) {
				GST_ERROR ("gst_pad_pull_range failed!");
				break;
			}	

			data = GST_BUFFER_DATA(buffer);
		       if (data == NULL) {
		         GST_WARNING("buffer is NULL in make aac seek table's STEP2");
		         gst_buffer_unref (buffer);
		         goto aac_seek_null_exit;
		       }
			offset = base_offset;
			while(offset <= aacparse->file_size) {
				gst_base_parse_add_index_entry (parse, offset, current_ts, TRUE, TRUE); /* force */
				framesize = ((int)data[2] << 8) + data[3] + 4;
				offset += framesize;
				data += framesize;
				framecount++;
				current_ts += (aacparse->frame_duration * GST_MSECOND) / 100; /* each frame is (frame_duration) ms */
			}
		  gst_base_parse_set_frame_rate (GST_BASE_PARSE (aacparse), aacparse->sample_rate, 1024, 2, 2);
			gst_buffer_unref (buffer);
			break;
		}
		default:
			break;
	}
aac_seek_null_exit:

  /* call baseparse src_event function to handle event */
  handled = GST_BASE_PARSE_CLASS (parent_class)->src_event (parse, event);

  return handled;
}

#endif //end of #ifdef GST_EXT_AACPARSER_MODIFICATION
