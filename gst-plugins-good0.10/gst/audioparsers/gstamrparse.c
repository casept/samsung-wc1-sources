/* GStreamer Adaptive Multi-Rate parser plugin
 * Copyright (C) 2006 Edgard Lima <edgard.lima@indt.org.br>
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
 * SECTION:element-amrparse
 * @short_description: AMR parser
 * @see_also: #GstAmrnbDec, #GstAmrnbEnc
 *
 * This is an AMR parser capable of handling both narrow-band and wideband
 * formats.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch filesrc location=abc.amr ! amrparse ! amrdec ! audioresample ! audioconvert ! alsasink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "gstamrparse.h"


static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/AMR, " "rate = (int) 8000, " "channels = (int) 1;"
        "audio/AMR-WB, " "rate = (int) 16000, " "channels = (int) 1;")
    );

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-amr-nb-sh; audio/x-amr-wb-sh"));

GST_DEBUG_CATEGORY_STATIC (amrparse_debug);
#define GST_CAT_DEFAULT amrparse_debug

static const gint block_size_nb[16] =
    { 12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0 };

static const gint block_size_wb[16] =
    { 17, 23, 32, 36, 40, 46, 50, 58, 60, 5, -1, -1, -1, -1, 0, 0 };

/* AMR has a "hardcoded" framerate of 50fps */
#define AMR_FRAMES_PER_SECOND 50
#define AMR_FRAME_DURATION (GST_SECOND/AMR_FRAMES_PER_SECOND)
#define AMR_MIME_HEADER_SIZE 9

static gboolean gst_amr_parse_start (GstBaseParse * parse);
static gboolean gst_amr_parse_stop (GstBaseParse * parse);

static gboolean gst_amr_parse_sink_setcaps (GstBaseParse * parse,
    GstCaps * caps);
static GstCaps *gst_amr_parse_sink_getcaps (GstBaseParse * parse);

static gboolean gst_amr_parse_check_valid_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, guint * framesize, gint * skipsize);

static GstFlowReturn gst_amr_parse_parse_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame);

#ifdef GST_EXT_AMRPARSER_MODIFICATION /* make full amr index table when seek */
  #define AMR_MAX_PULL_RANGE_BUF (5 * 1024 * 1024) /* 5 mbyte */
  static gboolean gst_amr_parse_src_eventfunc (GstBaseParse * parse, GstEvent * event);
#endif

#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (amrparse_debug, "amrparse", 0, \
                             "AMR-NB audio stream parser");

GST_BOILERPLATE_FULL (GstAmrParse, gst_amr_parse, GstBaseParse,
    GST_TYPE_BASE_PARSE, _do_init);

/**
 * gst_amr_parse_base_init:
 * @klass: #GstElementClass.
 *
 */
static void
gst_amr_parse_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_template);
  gst_element_class_add_static_pad_template (element_class, &src_template);

  gst_element_class_set_details_simple (element_class,
      "AMR audio stream parser", "Codec/Parser/Audio",
      "Adaptive Multi-Rate audio parser",
      "Ronald Bultje <rbultje@ronald.bitfreak.net>");
}


/**
 * gst_amr_parse_class_init:
 * @klass: GstAmrParseClass.
 *
 */
static void
gst_amr_parse_class_init (GstAmrParseClass * klass)
{
  GstBaseParseClass *parse_class = GST_BASE_PARSE_CLASS (klass);

  parse_class->start = GST_DEBUG_FUNCPTR (gst_amr_parse_start);
  parse_class->stop = GST_DEBUG_FUNCPTR (gst_amr_parse_stop);
  parse_class->set_sink_caps = GST_DEBUG_FUNCPTR (gst_amr_parse_sink_setcaps);
  parse_class->get_sink_caps = GST_DEBUG_FUNCPTR (gst_amr_parse_sink_getcaps);
  parse_class->parse_frame = GST_DEBUG_FUNCPTR (gst_amr_parse_parse_frame);
  parse_class->check_valid_frame =
      GST_DEBUG_FUNCPTR (gst_amr_parse_check_valid_frame);

#ifdef GST_EXT_AMRPARSER_MODIFICATION /* make full amr index table when seek */
  parse_class->src_event = gst_amr_parse_src_eventfunc;
#endif
}


/**
 * gst_amr_parse_init:
 * @amrparse: #GstAmrParse
 * @klass: #GstAmrParseClass.
 *
 */
static void
gst_amr_parse_init (GstAmrParse * amrparse, GstAmrParseClass * klass)
{
  /* init rest */
  gst_base_parse_set_min_frame_size (GST_BASE_PARSE (amrparse), 62);
  GST_DEBUG ("initialized");

}


/**
 * gst_amr_parse_set_src_caps:
 * @amrparse: #GstAmrParse.
 *
 * Set source pad caps according to current knowledge about the
 * audio stream.
 *
 * Returns: TRUE if caps were successfully set.
 */
static gboolean
gst_amr_parse_set_src_caps (GstAmrParse * amrparse)
{
  GstCaps *src_caps = NULL;
  gboolean res = FALSE;

  if (amrparse->wide) {
    GST_DEBUG_OBJECT (amrparse, "setting srcpad caps to AMR-WB");
    src_caps = gst_caps_new_simple ("audio/AMR-WB",
        "channels", G_TYPE_INT, 1, "rate", G_TYPE_INT, 16000, NULL);
  } else {
    GST_DEBUG_OBJECT (amrparse, "setting srcpad caps to AMR-NB");
    /* Max. size of NB frame is 31 bytes, so we can set the min. frame
       size to 32 (+1 for next frame header) */
    gst_base_parse_set_min_frame_size (GST_BASE_PARSE (amrparse), 32);
    src_caps = gst_caps_new_simple ("audio/AMR",
        "channels", G_TYPE_INT, 1, "rate", G_TYPE_INT, 8000, NULL);
  }
  gst_pad_use_fixed_caps (GST_BASE_PARSE (amrparse)->srcpad);
  res = gst_pad_set_caps (GST_BASE_PARSE (amrparse)->srcpad, src_caps);
  gst_caps_unref (src_caps);
  return res;
}


/**
 * gst_amr_parse_sink_setcaps:
 * @sinkpad: GstPad
 * @caps: GstCaps
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_amr_parse_sink_setcaps (GstBaseParse * parse, GstCaps * caps)
{
  GstAmrParse *amrparse;
  GstStructure *structure;
  const gchar *name;

  amrparse = GST_AMR_PARSE (parse);
  structure = gst_caps_get_structure (caps, 0);
  name = gst_structure_get_name (structure);

  GST_DEBUG_OBJECT (amrparse, "setcaps: %s", name);

  if (!strncmp (name, "audio/x-amr-wb-sh", 17)) {
    amrparse->block_size = block_size_wb;
    amrparse->wide = 1;
  } else if (!strncmp (name, "audio/x-amr-nb-sh", 17)) {
    amrparse->block_size = block_size_nb;
    amrparse->wide = 0;
  } else {
    GST_WARNING ("Unknown caps");
    return FALSE;
  }

  amrparse->need_header = FALSE;
  gst_base_parse_set_frame_rate (GST_BASE_PARSE (amrparse), 50, 1, 2, 2);
  gst_amr_parse_set_src_caps (amrparse);
  return TRUE;
}

/**
 * gst_amr_parse_parse_header:
 * @amrparse: #GstAmrParse
 * @data: Header data to be parsed.
 * @skipsize: Output argument where the frame size will be stored.
 *
 * Check if the given data contains an AMR mime header.
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_amr_parse_parse_header (GstAmrParse * amrparse,
    const guint8 * data, gint * skipsize)
{
  GST_DEBUG_OBJECT (amrparse, "Parsing header data");

  if (!memcmp (data, "#!AMR-WB\n", 9)) {
    GST_DEBUG_OBJECT (amrparse, "AMR-WB detected");
    amrparse->block_size = block_size_wb;
    amrparse->wide = TRUE;
    *skipsize = amrparse->header = 9;
  } else if (!memcmp (data, "#!AMR\n", 6)) {
    GST_DEBUG_OBJECT (amrparse, "AMR-NB detected");
    amrparse->block_size = block_size_nb;
    amrparse->wide = FALSE;
    *skipsize = amrparse->header = 6;
  } else
    return FALSE;

  gst_amr_parse_set_src_caps (amrparse);
  return TRUE;
}


/**
 * gst_amr_parse_check_valid_frame:
 * @parse: #GstBaseParse.
 * @buffer: #GstBuffer.
 * @framesize: Output variable where the found frame size is put.
 * @skipsize: Output variable which tells how much data needs to be skipped
 *            until a frame header is found.
 *
 * Implementation of "check_valid_frame" vmethod in #GstBaseParse class.
 *
 * Returns: TRUE if the given data contains valid frame.
 */
static gboolean
gst_amr_parse_check_valid_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, guint * framesize, gint * skipsize)
{
  GstBuffer *buffer;
  const guint8 *data;
  gint fsize, mode, dsize;
  GstAmrParse *amrparse;

  amrparse = GST_AMR_PARSE (parse);
  buffer = frame->buffer;
  data = GST_BUFFER_DATA (buffer);
  dsize = GST_BUFFER_SIZE (buffer);

  GST_LOG ("buffer: %d bytes", dsize);

  if (amrparse->need_header) {
    if (dsize >= AMR_MIME_HEADER_SIZE &&
        gst_amr_parse_parse_header (amrparse, data, skipsize)) {
      amrparse->need_header = FALSE;
      gst_base_parse_set_frame_rate (GST_BASE_PARSE (amrparse), 50, 1, 2, 2);
    } else {
      GST_WARNING ("media doesn't look like a AMR format");
    }
    /* We return FALSE, so this frame won't get pushed forward. Instead,
       the "skip" value is set, so next time we will receive a valid frame. */
    return FALSE;
  }

  /* Does this look like a possible frame header candidate? */
  if ((data[0] & 0x83) == 0) {
    /* Yep. Retrieve the frame size */
    mode = (data[0] >> 3) & 0x0F;
    fsize = amrparse->block_size[mode] + 1;     /* +1 for the header byte */

    /* We recognize this data as a valid frame when:
     *     - We are in sync. There is no need for extra checks then
     *     - We are in EOS. There might not be enough data to check next frame
     *     - Sync is lost, but the following data after this frame seem
     *       to contain a valid header as well (and there is enough data to
     *       perform this check)
     */
    if (fsize) {
      gboolean found = FALSE;

      /* in sync, no further check */
      if (!GST_BASE_PARSE_LOST_SYNC (parse)) {
        found = TRUE;
      } else if (dsize > fsize) {
        /* enough data, check for next sync */
        if ((data[fsize] & 0x83) == 0)
          found = TRUE;
      } else if (GST_BASE_PARSE_DRAINING (parse)) {
        /* not enough, but draining, so ok */
        found = TRUE;
      } else {
        /* indicate we need not skip, but need more data */
        *skipsize = 0;
        *framesize = fsize + 1;
      }
      if (found) {
        *framesize = fsize;
        return TRUE;
      }
    }
  }

  GST_LOG ("sync lost");
  return FALSE;
}


/**
 * gst_amr_parse_parse_frame:
 * @parse: #GstBaseParse.
 * @buffer: #GstBuffer.
 *
 * Implementation of "parse" vmethod in #GstBaseParse class.
 *
 * Returns: #GstFlowReturn defining the parsing status.
 */
static GstFlowReturn
gst_amr_parse_parse_frame (GstBaseParse * parse, GstBaseParseFrame * frame)
{
  return GST_FLOW_OK;
}


/**
 * gst_amr_parse_start:
 * @parse: #GstBaseParse.
 *
 * Implementation of "start" vmethod in #GstBaseParse class.
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_amr_parse_start (GstBaseParse * parse)
{
  GstAmrParse *amrparse;

  amrparse = GST_AMR_PARSE (parse);
  GST_DEBUG ("start");
  amrparse->need_header = TRUE;
  amrparse->header = 0;
  return TRUE;
}


/**
 * gst_amr_parse_stop:
 * @parse: #GstBaseParse.
 *
 * Implementation of "stop" vmethod in #GstBaseParse class.
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_amr_parse_stop (GstBaseParse * parse)
{
  GstAmrParse *amrparse;

  amrparse = GST_AMR_PARSE (parse);
  GST_DEBUG ("stop");
  amrparse->need_header = TRUE;
  amrparse->header = 0;
  return TRUE;
}

static GstCaps *
gst_amr_parse_sink_getcaps (GstBaseParse * parse)
{
  GstCaps *peercaps;
  GstCaps *res;

  peercaps = gst_pad_get_allowed_caps (GST_BASE_PARSE_SRC_PAD (parse));
  if (peercaps) {
    guint i, n;

    /* Rename structure names */
    peercaps = gst_caps_make_writable (peercaps);
    n = gst_caps_get_size (peercaps);
    for (i = 0; i < n; i++) {
      GstStructure *s = gst_caps_get_structure (peercaps, i);

      if (gst_structure_has_name (s, "audio/AMR"))
        gst_structure_set_name (s, "audio/x-amr-nb-sh");
      else
        gst_structure_set_name (s, "audio/x-amr-wb-sh");
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


#ifdef GST_EXT_AMRPARSER_MODIFICATION /* make full amr index table when seek */
/**
 * gst_amr_parse_src_eventfunc:
 * @parse: #GstBaseParse. #event
 *
 * before baseparse handles seek event, make full amr index table.
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_amr_parse_src_eventfunc (GstBaseParse * parse, GstEvent * event)
{
  gboolean handled = FALSE;
  GstAmrParse *amrparse;
  amrparse = GST_AMR_PARSE (parse);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      {
        GstFlowReturn res = GST_FLOW_OK;
        gint64 base_offset = 0, sync_offset = 0, cur = 0;
        gint32 frame_count = 1; /* do not add first frame because it is already in index table */
        gint64 total_file_size = 0, start_offset = 0;
        GstClockTime current_ts = GST_CLOCK_TIME_NONE;
        GstActivateMode pad_mode = GST_ACTIVATE_NONE;

#ifdef GST_EXT_BASEPARSER_MODIFICATION /* check baseparse define these fuction */
        gst_base_parse_get_pad_mode(parse, &pad_mode);
        if (pad_mode != GST_ACTIVATE_PULL) {
          GST_INFO_OBJECT (amrparse, "arm parser is not pull mode. amr parser can not make index table.");
          return FALSE;
        }
        gst_base_parse_get_upstream_size(parse, &total_file_size);
        gst_base_parse_get_index_last_offset(parse, &start_offset);
        gst_base_parse_get_index_last_ts(parse, &current_ts);
#else
        GST_WARNING_OBJECT (amrparse, "baseparser does not define get private param functions. can not make index table here.");
        break;
#endif

        GST_LOG_OBJECT (amrparse, "gst_amr_parse_src_eventfunc GST_EVENT_SEEK enter");

        if (total_file_size == 0 || start_offset >= total_file_size) {
          GST_ERROR("last index offset %d is larger than file size %d", start_offset, total_file_size);
          break;
        }

        gst_event_parse_seek (event, NULL, NULL, NULL, NULL, &cur, NULL, NULL);
        if (cur <= current_ts) {
          GST_INFO_OBJECT (amrparse, "seek to %"GST_TIME_FORMAT" within index table %"GST_TIME_FORMAT". do not make index table",
              GST_TIME_ARGS(cur), GST_TIME_ARGS(current_ts));
          break;
        } else {
          GST_INFO_OBJECT (amrparse, "seek to %"GST_TIME_FORMAT" without index table %"GST_TIME_FORMAT". make index table",
              GST_TIME_ARGS(cur), GST_TIME_ARGS(current_ts));
        }

        GST_INFO_OBJECT (amrparse, "make AMR Index Table. file_size  = %"G_GINT64_FORMAT" last idx offset=%"G_GINT64_FORMAT
            ", last idx ts=%"GST_TIME_FORMAT, total_file_size, start_offset, GST_TIME_ARGS(current_ts));

        base_offset = start_offset; /* set base by start offset */


        /************************************/
        /* STEP 1: MAX_PULL_RANGE_BUF cycle */
        /************************************/
        while (total_file_size - base_offset >= AMR_MAX_PULL_RANGE_BUF) {
          gint64 offset = 0;
          GstBuffer *buffer = NULL;
          guint8 *buf = NULL;

         GST_INFO_OBJECT (amrparse, "gst_pad_pull_range %d bytes (from %"G_GINT64_FORMAT") use max size", AMR_MAX_PULL_RANGE_BUF, base_offset);
          res = gst_pad_pull_range (parse->sinkpad, base_offset,
              base_offset + AMR_MAX_PULL_RANGE_BUF, &buffer);
          if (res != GST_FLOW_OK) {
            GST_ERROR_OBJECT (amrparse, "gst_pad_pull_range failed!");
            break;
          }

          buf = GST_BUFFER_DATA(buffer);
          if (buf == NULL) {
            GST_WARNING("buffer is NULL in make amr seek table's STEP1");
            gst_buffer_unref (buffer);
            goto amr_seek_null_exit;
          }

          while (offset <= AMR_MAX_PULL_RANGE_BUF) {
            gint mode = 0,  frame_size = 0;

            if ((buf[offset] & 0x83) == 0) {
              mode = (buf[offset] >> 3) & 0x0F;
              frame_size = amrparse->block_size[mode] + 1; /* +1 for the header byte */

              if (frame_count % 50 == 0) { /* 1 sec == 50 frames. we make idx per sec */
                gst_base_parse_add_index_entry (parse, base_offset +offset, current_ts, TRUE, TRUE); /* force */
                GST_DEBUG_OBJECT (amrparse, "Adding  index ts=%"GST_TIME_FORMAT" offset %"G_GINT64_FORMAT,
                    GST_TIME_ARGS(current_ts), base_offset + offset);
              }

              current_ts += 20 * 1000 * 1000; /* each frame is 20ms */
              offset += frame_size;
              frame_count++;
            } else {
              GST_WARNING_OBJECT (amrparse, "we lost sync");
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

          GST_INFO_OBJECT (amrparse, "gst_pad_pull_range %"G_GINT64_FORMAT" bytes (from %"G_GINT64_FORMAT") use remain_buf size",
              total_file_size - base_offset, base_offset);
          res = gst_pad_pull_range (parse->sinkpad, base_offset,
              total_file_size, &buffer);
          if (res != GST_FLOW_OK) {
            GST_ERROR ("gst_pad_pull_range failed!");
            break;
          }

          buf = GST_BUFFER_DATA(buffer);
          if (buf == NULL) {
             GST_WARNING("buffer is NULL in make amr seek table's STEP2");
             gst_buffer_unref (buffer);
             goto amr_seek_null_exit;
           }

          while (base_offset + offset < total_file_size) {
            gint mode = 0, frame_size = 0;

            if ((buf[offset] & 0x83) == 0) {
              mode = (buf[offset] >> 3) & 0x0F;
              frame_size = amrparse->block_size[mode] + 1; /* +1 for the header byte */

              if (frame_count % 50 == 0) { /* 1 sec == 50 frames. we make idx per sec */
                gst_base_parse_add_index_entry (parse, base_offset +offset, current_ts, TRUE, TRUE); /* force */
                GST_DEBUG_OBJECT (amrparse, "Adding  index ts=%"GST_TIME_FORMAT" offset %"G_GINT64_FORMAT,
                    GST_TIME_ARGS(current_ts), base_offset + offset);
              }

              current_ts += 20 * 1000 * 1000; /* each frame is 20ms */
              offset += frame_size;
              frame_count++;
            } else {
              GST_WARNING_OBJECT (amrparse, "we lost sync");
              offset++;
            }
          } /* while */

        gst_buffer_unref (buffer);
        } /* end remain_buf buffer cycle */

        GST_LOG_OBJECT (amrparse, "gst_amr_parse_src_eventfunc GST_EVENT_SEEK leave");
      }
     break;

    default:
      break;
  }

amr_seek_null_exit:

  /* call baseparse src_event function to handle event */
  handled = GST_BASE_PARSE_CLASS (parent_class)->src_event (parse, event);

  return handled;
}
#endif
