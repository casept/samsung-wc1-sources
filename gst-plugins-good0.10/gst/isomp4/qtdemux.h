/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_QTDEMUX_H__
#define __GST_QTDEMUX_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>

#define QTDEMUX_MODIFICATION
#define MULTI_AUDIO 1

#ifdef QTDEMUX_MODIFICATION
#include <stdio.h>
#include <gst/base/gstbitreader.h>
#endif

G_BEGIN_DECLS

GST_DEBUG_CATEGORY_EXTERN (qtdemux_debug);
#define GST_CAT_DEFAULT qtdemux_debug

#define GST_TYPE_QTDEMUX \
  (gst_qtdemux_get_type())
#define GST_QTDEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_QTDEMUX,GstQTDemux))
#define GST_QTDEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_QTDEMUX,GstQTDemuxClass))
#define GST_IS_QTDEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_QTDEMUX))
#define GST_IS_QTDEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_QTDEMUX))

#define GST_QTDEMUX_CAST(obj) ((GstQTDemux *)(obj))

/* qtdemux produces these for atoms it cannot parse */
#define GST_QT_DEMUX_PRIVATE_TAG "private-qt-tag"
#define GST_QT_DEMUX_CLASSIFICATION_TAG "classification"

#define GST_QTDEMUX_MAX_STREAMS         8

typedef struct _GstQTDemux GstQTDemux;
typedef struct _GstQTDemuxClass GstQTDemuxClass;
typedef struct _QtDemuxStream QtDemuxStream;
#ifdef QTDEMUX_MODIFICATION
typedef struct _GstMpeg4VideoObjectLayer        GstMpeg4VideoObjectLayer;
typedef struct _GstMpeg4VisualObject            GstMpeg4VisualObject;
typedef enum {
  GST_MPEG4_SQUARE        = 0x01,
  GST_MPEG4_625_TYPE_4_3  = 0x02,
  GST_MPEG4_525_TYPE_4_3  = 0x03,
  GST_MPEG4_625_TYPE_16_9 = 0x04,
  GST_MPEG4_525_TYPE_16_9 = 0x05,
  GST_MPEG4_EXTENDED_PAR  = 0x0f,
} GstMpeg4AspectRatioInfo;
typedef enum {
  GST_MPEG4_VIDEO_ID         = 0x01,
  GST_MPEG4_STILL_TEXTURE_ID = 0x02,
  GST_MPEG4_STILL_MESH_ID    = 0x03,
  GST_MPEG4_STILL_FBA_ID     = 0x04,
  GST_MPEG4_STILL_3D_MESH_ID = 0x05,
  /*... reserved */

} GstMpeg4VisualObjectType;
typedef enum {
  /* Other value are reserved */
  GST_MPEG4_CHROMA_4_2_0 = 0x01
} GstMpeg4ChromaFormat;
typedef enum {
  GST_MPEG4_RECTANGULAR,
  GST_MPEG4_BINARY,
  GST_MPEG4_BINARY_ONLY,
  GST_MPEG4_GRAYSCALE
} GstMpeg4VideoObjectLayerShape;
typedef enum {
  GST_MPEG4_SPRITE_UNUSED,
  GST_MPEG4_SPRITE_STATIC,
  GST_MPEG4_SPRITE_GMG
} GstMpeg4SpriteEnable;
typedef enum {
  GST_MPEG4_PARSER_OK,
  GST_MPEG4_PARSER_BROKEN_DATA,
  GST_MPEG4_PARSER_NO_PACKET,
  GST_MPEG4_PARSER_NO_PACKET_END,
  GST_MPEG4_PARSER_ERROR,
} GstMpeg4ParseResult;
struct _GstMpeg4VideoObjectLayer {
  guint8 random_accessible_vol;
  guint8 video_object_type_indication;

  guint8 is_object_layer_identifier;
  /* if is_object_layer_identifier */
  guint8 verid;
  guint8 priority;

  GstMpeg4AspectRatioInfo aspect_ratio_info;
  guint8 par_width;
  guint8 par_height;

  guint8 control_parameters;
  /* if control_parameters */
  GstMpeg4ChromaFormat chroma_format;
  guint8 low_delay;
  guint8 vbv_parameters;
  /* if vbv_parameters */
  guint16 first_half_bitrate;
  guint16 latter_half_bitrate;
  guint16 first_half_vbv_buffer_size;
  guint16 latter_half_vbv_buffer_size;
  guint16 first_half_vbv_occupancy;
  guint16 latter_half_vbv_occupancy;

  /* Computed values */
  guint32 bit_rate;
  guint32 vbv_buffer_size;

  GstMpeg4VideoObjectLayerShape shape;
  /* if shape == GST_MPEG4_GRAYSCALE && verid =! 1 */
  guint8 shape_extension;

  guint16 vop_time_increment_resolution;
  guint8 vop_time_increment_bits;
  guint8 fixed_vop_rate;
  /* if fixed_vop_rate */
  guint16 fixed_vop_time_increment;

  guint16 width;
  guint16 height;
  guint8 interlaced;
  guint8 obmc_disable;

  GstMpeg4SpriteEnable sprite_enable;
  /* if vol->sprite_enable == SPRITE_GMG or SPRITE_STATIC*/
  /* if vol->sprite_enable != GST_MPEG4_SPRITE_GMG */
  guint16 sprite_width;
  guint16 sprite_height;
  guint16 sprite_left_coordinate;
  guint16 sprite_top_coordinate;

  guint8 no_of_sprite_warping_points;
  guint8 sprite_warping_accuracy;
  guint8 sprite_brightness_change;
  /* if vol->sprite_enable != GST_MPEG4_SPRITE_GMG */
  guint8 low_latency_sprite_enable;

  /* if shape != GST_MPEG4_RECTANGULAR */
  guint8 sadct_disable;

  guint8 not_8_bit;

  /* if no_8_bit */
  guint8 quant_precision;
  guint8 bits_per_pixel;

  /* if shape == GRAYSCALE */
  guint8 no_gray_quant_update;
  guint8 composition_method;
  guint8 linear_composition;

  guint8 quant_type;
  /* if quant_type */
  guint8 load_intra_quant_mat;
  guint8 intra_quant_mat[64];
  guint8 load_non_intra_quant_mat;
  guint8 non_intra_quant_mat[64];

  guint8 quarter_sample;
  guint8 complexity_estimation_disable;
  guint8 resync_marker_disable;
  guint8 data_partitioned;
  guint8 reversible_vlc;
  guint8 newpred_enable;
  guint8 reduced_resolution_vop_enable;
  guint8 scalability;
  guint8 enhancement_type;

  //GstMpeg4VideoPlaneShortHdr short_hdr;
};
struct _GstMpeg4VisualObject {
  guint8 is_identifier;
  /* If is_identifier */
  guint8 verid;
  guint8 priority;
 GstMpeg4VisualObjectType type;
};
#endif
struct _GstQTDemux {
  GstElement element;

  /* pads */
  GstPad *sinkpad;

  QtDemuxStream *streams[GST_QTDEMUX_MAX_STREAMS];
  gint     n_streams;
  gint     n_video_streams;
  gint     n_audio_streams;
  gint     n_sub_streams;

  guint  major_brand;
  GstBuffer *comp_brands;
  GNode *moov_node;
  GNode *moov_node_compressed;

  guint32 timescale;
  guint64 duration;

  gboolean fragmented;
  /* offset of the mfra atom */
  guint64 mfra_offset;
  guint64 moof_offset;

  gint state;

  gboolean pullbased;
  gboolean posted_redirect;

  /* push based variables */
  guint neededbytes;
  guint todrop;
  GstAdapter *adapter;
  GstBuffer *mdatbuffer;
  guint64 mdatleft;

  guint64 offset;
  /* offset of the mdat atom */
  guint64 mdatoffset;
  guint64 first_mdat;
  gboolean got_moov;
  guint header_size;

  GstTagList *tag_list;

  /* configured playback region */
  GstSegment segment;
  gboolean segment_running;
  GstEvent *pending_newsegment;

  /* gst index support */
  GstIndex *element_index;
  gint index_id;

  gint64 requested_seek_time;
  guint64 seek_offset;

  gboolean upstream_seekable;
  gboolean upstream_size;
#ifdef MULTI_AUDIO
  GList *Language_list;
#endif
#ifdef QTDEMUX_MODIFICATION
  FILE* file;
  FILE* ofile;
  gchar* filename;
  guint filesize;
  guint maxbuffersize;
  gboolean fwdtrick_mode;
  gboolean piff_fragmented;
  GstClockTime max_pop_ts;
  gboolean need_parsing_moov;
#endif
};

struct _GstQTDemuxClass {
  GstElementClass parent_class;
};

GType gst_qtdemux_get_type (void);

G_END_DECLS

#endif /* __GST_QTDEMUX_H__ */
