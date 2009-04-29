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


#ifndef __GST_VORBIS_ENC_H__
#define __GST_VORBIS_ENC_H__


#include <gst/gst.h>

#include <vorbis/codec.h>

G_BEGIN_DECLS

#define GST_TYPE_VORBISENC \
  (gst_vorbis_enc_get_type())
#define GST_VORBISENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VORBISENC,GstVorbisEnc))
#define GST_VORBISENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VORBISENC,GstVorbisEncClass))
#define GST_IS_VORBISENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VORBISENC))
#define GST_IS_VORBISENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VORBISENC))

typedef struct _GstVorbisEnc GstVorbisEnc;
typedef struct _GstVorbisEncClass GstVorbisEncClass;

/**
 * GstVorbisEnc:
 *
 * Opaque data structure.
 */
struct _GstVorbisEnc {
  GstElement       element;

  GstPad          *sinkpad;
  GstPad          *srcpad;

  GstCaps         *srccaps;
  GstCaps         *sinkcaps;

  vorbis_info      vi; /* struct that stores all the static vorbis bitstream
                                                            settings */
  vorbis_comment   vc; /* struct that stores all the user comments */

  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */

  gboolean         managed;
  gint             bitrate;
  gint             min_bitrate;
  gint             max_bitrate;
  gfloat           quality;
  gboolean         quality_set;

  gint             channels;
  gint             frequency;

  guint64          samples_in;
  guint64          bytes_out;
  GstClockTime     next_ts;
  GstClockTime     expected_ts;
  gboolean         next_discont;
  guint64          granulepos_offset;
  gint64           subgranule_offset;
  GstSegment       segment;
  GstClockTime     initial_ts;

  GstTagList *     tags;

  gboolean         setup;
  gboolean         header_sent;
  gchar           *last_message;
};

struct _GstVorbisEncClass {
  GstElementClass parent_class;
};

GType gst_vorbis_enc_get_type(void);

G_END_DECLS

#endif /* __GST_VORBIS_ENC_H__ */
