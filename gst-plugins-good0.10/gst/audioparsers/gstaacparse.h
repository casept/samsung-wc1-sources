/* GStreamer AAC parser
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

#ifndef __GST_AAC_PARSE_H__
#define __GST_AAC_PARSE_H__

#include <gst/gst.h>
#include <gst/base/gstbaseparse.h>

G_BEGIN_DECLS

#define GST_TYPE_AAC_PARSE \
  (gst_aac_parse_get_type())
#define GST_AAC_PARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_AAC_PARSE, GstAacParse))
#define GST_AAC_PARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_AAC_PARSE, GstAacParseClass))
#define GST_IS_AAC_PARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_AAC_PARSE))
#define GST_IS_AAC_PARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_AAC_PARSE))

typedef enum
{
    DECAAC_RET_SUCCESS           = 0,
    DECAAC_RET_ERR_NOT_SUFF_MEM  = -2,
    DECAAC_RET_ERR_NOT_SUPPORT   = -3,
    DECAAC_RET_ERR_INVALID_ARG   = -4,
    DECAAC_RET_ERR_INVALID_BS    = -5,
    DECAAC_RET_ERR_NOT_EXPECTED  = -8,
    DECAAC_RET_ERR_NOT_SUFF_BS   = -9,
    DECAAC_RET_ERR_BAD_CRC       = -10,
    DECAAC_RET_ERR_INVALID_HCB   = -11,
    DECAAC_RET_ERR_UNKNOWN       = -0xFF,
} eDECAACRET;

/**
 * GstAacHeaderType:
 * @DSPAAC_HEADER_NOT_PARSED: Header not parsed yet.
 * @DSPAAC_HEADER_UNKNOWN: Unknown (not recognized) header.
 * @DSPAAC_HEADER_ADIF: ADIF header found.
 * @DSPAAC_HEADER_ADTS: ADTS header found.
 * @DSPAAC_HEADER_NONE: Raw stream, no header.
 *
 * Type header enumeration set in #header_type.
 */
typedef enum {
  DSPAAC_HEADER_NOT_PARSED,
  DSPAAC_HEADER_UNKNOWN,
  DSPAAC_HEADER_ADIF,
  DSPAAC_HEADER_ADTS,
  DSPAAC_HEADER_NONE
} GstAacHeaderType;


typedef struct _GstAacParse GstAacParse;
typedef struct _GstAacParseClass GstAacParseClass;

/**
 * GstAacParse:
 *
 * The opaque GstAacParse data structure.
 */
struct _GstAacParse {
  GstBaseParse element;

  /* Stream type -related info */
  gint           object_type;
  gint           bitrate;
  gint           sample_rate;
  gint           channels;
  gint           mpegversion;
  gint           frame_samples;

#ifdef GST_EXT_AACPARSER_MODIFICATION
  gboolean   first_frame; /* estimate duration once at the first time */

  guint           hdr_bitrate;    /* added - estimated bitrate (bps) */
  guint           spf;            /* added - samples per frame = frame_samples */
  guint           frame_duration; /* added - duration per frame (msec) */
  guint           frame_per_sec; /* added - frames per second (ea) */
  guint           bitstream_type; /* added- bitstream type - constant or variable */ //evil
  guint           adif_header_length;
  guint           num_program_config_elements;
  guint           read_bytes;
  gint64         file_size;
  guint           frame_byte;
#endif
  GstAacHeaderType header_type;
};

/**
 * GstAacParseClass:
 * @parent_class: Element parent class.
 *
 * The opaque GstAacParseClass data structure.
 */
struct _GstAacParseClass {
  GstBaseParseClass parent_class;
};

GType gst_aac_parse_get_type (void);

G_END_DECLS

#endif /* __GST_AAC_PARSE_H__ */
