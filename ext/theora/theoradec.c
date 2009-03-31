/* GStreamer
 * Copyright (C) 2004 Benjamin Otte <in7y118@public.uni-hamburg.de>
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
 * SECTION:element-theoradec
 * @see_also: theoraenc, oggdemux
 *
 * This element decodes theora streams into raw video
 * <ulink url="http://www.theora.org/">Theora</ulink> is a royalty-free
 * video codec maintained by the <ulink url="http://www.xiph.org/">Xiph.org
 * Foundation</ulink>, based on the VP3 codec.
 *
 * <refsect2>
 * <title>Example pipeline</title>
 * |[
 * gst-launch -v filesrc location=videotestsrc.ogg ! oggdemux ! theoradec ! xvimagesink
 * ]| This example pipeline will decode an ogg stream and decodes the theora video. Refer to
 * the theoraenc example to create the ogg file.
 * </refsect2>
 *
 * Last reviewed on 2006-03-01 (0.10.4)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "gsttheoradec.h"
#include <gst/tag/tag.h>

#define GST_CAT_DEFAULT theoradec_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

#define THEORA_DEF_CROP         TRUE
enum
{
  ARG_0,
  ARG_CROP
};

static const GstElementDetails theora_dec_details =
GST_ELEMENT_DETAILS ("Theora video decoder",
    "Codec/Decoder/Video",
    "decode raw theora streams to raw YUV video",
    "Benjamin Otte <in7y118@public.uni-hamburg.de>, "
    "Wim Taymans <wim@fluendo.com>");

static GstStaticPadTemplate theora_dec_src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-yuv, "
        "format = (fourcc) I420, "
        "framerate = (fraction) [0/1, MAX], "
        "width = (int) [ 1, MAX ], " "height = (int) [ 1, MAX ]")
    );

static GstStaticPadTemplate theora_dec_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-theora")
    );

GST_BOILERPLATE (GstTheoraDec, gst_theora_dec, GstElement, GST_TYPE_ELEMENT);

static void theora_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void theora_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static gboolean theora_dec_sink_event (GstPad * pad, GstEvent * event);
static gboolean theora_dec_setcaps (GstPad * pad, GstCaps * caps);
static GstFlowReturn theora_dec_chain (GstPad * pad, GstBuffer * buffer);
static GstStateChangeReturn theora_dec_change_state (GstElement * element,
    GstStateChange transition);
static gboolean theora_dec_src_event (GstPad * pad, GstEvent * event);
static gboolean theora_dec_src_query (GstPad * pad, GstQuery * query);
static gboolean theora_dec_src_convert (GstPad * pad,
    GstFormat src_format, gint64 src_value,
    GstFormat * dest_format, gint64 * dest_value);
static gboolean theora_dec_sink_convert (GstPad * pad,
    GstFormat src_format, gint64 src_value,
    GstFormat * dest_format, gint64 * dest_value);
static gboolean theora_dec_sink_query (GstPad * pad, GstQuery * query);

#if 0
static const GstFormat *theora_get_formats (GstPad * pad);
#endif
#if 0
static const GstEventMask *theora_get_event_masks (GstPad * pad);
#endif
static const GstQueryType *theora_get_query_types (GstPad * pad);


static void
gst_theora_dec_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&theora_dec_src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&theora_dec_sink_factory));
  gst_element_class_set_details (element_class, &theora_dec_details);
}

static void
gst_theora_dec_class_init (GstTheoraDecClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = theora_dec_set_property;
  gobject_class->get_property = theora_dec_get_property;

  g_object_class_install_property (gobject_class, ARG_CROP,
      g_param_spec_boolean ("crop", "Crop",
          "Crop the image to the visible region", THEORA_DEF_CROP,
          (GParamFlags) G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state = theora_dec_change_state;

  GST_DEBUG_CATEGORY_INIT (theoradec_debug, "theoradec", 0, "Theora decoder");
}

static void
gst_theora_dec_init (GstTheoraDec * dec, GstTheoraDecClass * g_class)
{
  dec->sinkpad =
      gst_pad_new_from_static_template (&theora_dec_sink_factory, "sink");
  gst_pad_set_query_function (dec->sinkpad, theora_dec_sink_query);
  gst_pad_set_event_function (dec->sinkpad, theora_dec_sink_event);
  gst_pad_set_setcaps_function (dec->sinkpad, theora_dec_setcaps);
  gst_pad_set_chain_function (dec->sinkpad, theora_dec_chain);
  gst_element_add_pad (GST_ELEMENT (dec), dec->sinkpad);

  dec->srcpad =
      gst_pad_new_from_static_template (&theora_dec_src_factory, "src");
  gst_pad_set_event_function (dec->srcpad, theora_dec_src_event);
  gst_pad_set_query_type_function (dec->srcpad, theora_get_query_types);
  gst_pad_set_query_function (dec->srcpad, theora_dec_src_query);
  gst_pad_use_fixed_caps (dec->srcpad);

  gst_element_add_pad (GST_ELEMENT (dec), dec->srcpad);

  dec->crop = THEORA_DEF_CROP;
  dec->gather = NULL;
  dec->decode = NULL;
  dec->queued = NULL;
  dec->pendingevents = NULL;
}

static void
gst_theora_dec_reset (GstTheoraDec * dec)
{
  dec->need_keyframe = TRUE;
  dec->last_timestamp = -1;
  dec->granulepos = -1;
  dec->discont = TRUE;
  dec->frame_nr = -1;
  dec->seqnum = gst_util_seqnum_next ();
  gst_segment_init (&dec->segment, GST_FORMAT_TIME);

  GST_OBJECT_LOCK (dec);
  dec->proportion = 1.0;
  dec->earliest_time = -1;
  GST_OBJECT_UNLOCK (dec);

  g_list_foreach (dec->queued, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (dec->queued);
  dec->queued = NULL;
  g_list_foreach (dec->gather, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (dec->gather);
  dec->gather = NULL;
  g_list_foreach (dec->decode, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (dec->decode);
  dec->decode = NULL;
  g_list_foreach (dec->pendingevents, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (dec->pendingevents);
  dec->pendingevents = NULL;

  if (dec->tags) {
    gst_tag_list_free (dec->tags);
    dec->tags = NULL;
  }
}

static int
_theora_ilog (unsigned int v)
{
  int ret = 0;

  while (v) {
    ret++;
    v >>= 1;
  }
  return (ret);
}

/* Return the frame number (starting from zero) corresponding to this 
 * granulepos */
static gint64
_theora_granule_frame (GstTheoraDec * dec, gint64 granulepos)
{
  guint ilog;
  gint framenum;

  if (granulepos == -1)
    return -1;

  ilog = dec->granule_shift;

  /* granulepos is last ilog bits for counting pframes since last iframe and 
   * bits in front of that for the framenumber of the last iframe. */
  framenum = granulepos >> ilog;
  framenum += granulepos - (framenum << ilog);

  /* This is 0-based for old bitstreams, 1-based for new. Fix up. */
  if (!dec->is_old_bitstream)
    framenum -= 1;

  GST_DEBUG_OBJECT (dec, "framecount=%d, ilog=%u", framenum, ilog);

  return framenum;
}

/* Return the frame start time corresponding to this granulepos */
static GstClockTime
_theora_granule_start_time (GstTheoraDec * dec, gint64 granulepos)
{
  gint64 framecount;

  /* invalid granule results in invalid time */
  if (granulepos == -1)
    return -1;

  /* get framecount */
  framecount = _theora_granule_frame (dec, granulepos);

  return gst_util_uint64_scale_int (framecount * GST_SECOND,
      dec->info.fps_denominator, dec->info.fps_numerator);
}

static gint64
_inc_granulepos (GstTheoraDec * dec, gint64 granulepos)
{
  gint framecount;

  if (granulepos == -1)
    return -1;

  framecount = _theora_granule_frame (dec, granulepos);

  return (framecount + 1 +
      (dec->is_old_bitstream ? 0 : 1)) << dec->granule_shift;
}

#if 0
static const GstFormat *
theora_get_formats (GstPad * pad)
{
  static GstFormat src_formats[] = {
    GST_FORMAT_DEFAULT,         /* frames in this case */
    GST_FORMAT_TIME,
    GST_FORMAT_BYTES,
    0
  };
  static GstFormat sink_formats[] = {
    GST_FORMAT_DEFAULT,
    GST_FORMAT_TIME,
    0
  };

  return (GST_PAD_IS_SRC (pad) ? src_formats : sink_formats);
}
#endif

#if 0
static const GstEventMask *
theora_get_event_masks (GstPad * pad)
{
  static const GstEventMask theora_src_event_masks[] = {
    {GST_EVENT_SEEK, GST_SEEK_METHOD_SET | GST_SEEK_FLAG_FLUSH},
    {0,}
  };

  return theora_src_event_masks;
}
#endif

static const GstQueryType *
theora_get_query_types (GstPad * pad)
{
  static const GstQueryType theora_src_query_types[] = {
    GST_QUERY_POSITION,
    GST_QUERY_DURATION,
    GST_QUERY_CONVERT,
    0
  };

  return theora_src_query_types;
}


static gboolean
theora_dec_src_convert (GstPad * pad,
    GstFormat src_format, gint64 src_value,
    GstFormat * dest_format, gint64 * dest_value)
{
  gboolean res = TRUE;
  GstTheoraDec *dec;
  guint64 scale = 1;

  if (src_format == *dest_format) {
    *dest_value = src_value;
    return TRUE;
  }

  dec = GST_THEORA_DEC (gst_pad_get_parent (pad));

  /* we need the info part before we can done something */
  if (!dec->have_header)
    goto no_header;

  switch (src_format) {
    case GST_FORMAT_BYTES:
      switch (*dest_format) {
        case GST_FORMAT_DEFAULT:
          *dest_value = gst_util_uint64_scale_int (src_value, 2,
              dec->info.height * dec->info.width * 3);
          break;
        case GST_FORMAT_TIME:
          /* seems like a rather silly conversion, implement me if you like */
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_TIME:
      switch (*dest_format) {
        case GST_FORMAT_BYTES:
          scale = 3 * (dec->info.width * dec->info.height) / 2;
        case GST_FORMAT_DEFAULT:
          *dest_value = scale * gst_util_uint64_scale (src_value,
              dec->info.fps_numerator, dec->info.fps_denominator * GST_SECOND);
          break;
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_DEFAULT:
      switch (*dest_format) {
        case GST_FORMAT_TIME:
          *dest_value = gst_util_uint64_scale (src_value,
              GST_SECOND * dec->info.fps_denominator, dec->info.fps_numerator);
          break;
        case GST_FORMAT_BYTES:
          *dest_value = gst_util_uint64_scale_int (src_value,
              3 * dec->info.width * dec->info.height, 2);
          break;
        default:
          res = FALSE;
      }
      break;
    default:
      res = FALSE;
  }
done:
  gst_object_unref (dec);
  return res;

  /* ERRORS */
no_header:
  {
    GST_DEBUG_OBJECT (dec, "no header yet, cannot convert");
    res = FALSE;
    goto done;
  }
}

static gboolean
theora_dec_sink_convert (GstPad * pad,
    GstFormat src_format, gint64 src_value,
    GstFormat * dest_format, gint64 * dest_value)
{
  gboolean res = TRUE;
  GstTheoraDec *dec;

  if (src_format == *dest_format) {
    *dest_value = src_value;
    return TRUE;
  }

  dec = GST_THEORA_DEC (gst_pad_get_parent (pad));

  /* we need the info part before we can done something */
  if (!dec->have_header)
    goto no_header;

  switch (src_format) {
    case GST_FORMAT_DEFAULT:
      switch (*dest_format) {
        case GST_FORMAT_TIME:
          *dest_value = _theora_granule_start_time (dec, src_value);
          break;
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_TIME:
      switch (*dest_format) {
        case GST_FORMAT_DEFAULT:
        {
          guint rest;

          /* framecount */
          *dest_value = gst_util_uint64_scale (src_value,
              dec->info.fps_numerator, GST_SECOND * dec->info.fps_denominator);

          /* funny way of calculating granulepos in theora */
          rest = *dest_value / dec->info.keyframe_frequency_force;
          *dest_value -= rest;
          *dest_value <<= dec->granule_shift;
          *dest_value += rest;
          break;
        }
        default:
          res = FALSE;
          break;
      }
      break;
    default:
      res = FALSE;
  }
done:
  gst_object_unref (dec);
  return res;

  /* ERRORS */
no_header:
  {
    GST_DEBUG_OBJECT (dec, "no header yet, cannot convert");
    res = FALSE;
    goto done;
  }
}

static gboolean
theora_dec_src_query (GstPad * pad, GstQuery * query)
{
  GstTheoraDec *dec;

  gboolean res = FALSE;

  dec = GST_THEORA_DEC (gst_pad_get_parent (pad));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:
    {
      gint64 granulepos, value;
      GstFormat my_format, format;
      gint64 time;

      /* we can convert a granule position to everything */
      granulepos = dec->granulepos;

      GST_LOG_OBJECT (dec,
          "query %p: we have current granule: %lld", query, granulepos);

      /* parse format */
      gst_query_parse_position (query, &format, NULL);

      /* and convert to the final format in two steps with time as the 
       * intermediate step */
      my_format = GST_FORMAT_TIME;
      if (!(res =
              theora_dec_sink_convert (dec->sinkpad, GST_FORMAT_DEFAULT,
                  granulepos, &my_format, &time)))
        goto error;

      time = gst_segment_to_stream_time (&dec->segment, GST_FORMAT_TIME, time);

      GST_LOG_OBJECT (dec,
          "query %p: our time: %" GST_TIME_FORMAT, query, GST_TIME_ARGS (time));

      if (!(res =
              theora_dec_src_convert (pad, my_format, time, &format, &value)))
        goto error;

      gst_query_set_position (query, format, value);

      GST_LOG_OBJECT (dec,
          "query %p: we return %lld (format %u)", query, value, format);

      break;
    }
    case GST_QUERY_DURATION:
    {
      GstPad *peer;

      if (!(peer = gst_pad_get_peer (dec->sinkpad)))
        goto error;

      /* forward to peer for total */
      res = gst_pad_query (peer, query);
      gst_object_unref (peer);
      if (!res)
        goto error;

      break;
    }
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (!(res =
              theora_dec_src_convert (pad, src_fmt, src_val, &dest_fmt,
                  &dest_val)))
        goto error;

      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      break;
    }
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }
done:
  gst_object_unref (dec);

  return res;

  /* ERRORS */
error:
  {
    GST_DEBUG_OBJECT (dec, "query failed");
    goto done;
  }
}

static gboolean
theora_dec_sink_query (GstPad * pad, GstQuery * query)
{
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (!(res =
              theora_dec_sink_convert (pad, src_fmt, src_val, &dest_fmt,
                  &dest_val)))
        goto error;

      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      break;
    }
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }

error:
  return res;
}

static gboolean
theora_dec_src_event (GstPad * pad, GstEvent * event)
{
  gboolean res = TRUE;
  GstTheoraDec *dec;

  dec = GST_THEORA_DEC (gst_pad_get_parent (pad));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
    {
      GstFormat format, tformat;
      gdouble rate;
      GstEvent *real_seek;
      GstSeekFlags flags;
      GstSeekType cur_type, stop_type;
      gint64 cur, stop;
      gint64 tcur, tstop;
      guint32 seqnum;

      gst_event_parse_seek (event, &rate, &format, &flags, &cur_type, &cur,
          &stop_type, &stop);
      seqnum = gst_event_get_seqnum (event);
      gst_event_unref (event);

      /* we have to ask our peer to seek to time here as we know
       * nothing about how to generate a granulepos from the src
       * formats or anything.
       * 
       * First bring the requested format to time 
       */
      tformat = GST_FORMAT_TIME;
      if (!(res = theora_dec_src_convert (pad, format, cur, &tformat, &tcur)))
        goto convert_error;
      if (!(res = theora_dec_src_convert (pad, format, stop, &tformat, &tstop)))
        goto convert_error;

      /* then seek with time on the peer */
      real_seek = gst_event_new_seek (rate, GST_FORMAT_TIME,
          flags, cur_type, tcur, stop_type, tstop);
      gst_event_set_seqnum (real_seek, seqnum);

      res = gst_pad_push_event (dec->sinkpad, real_seek);
      break;
    }
    case GST_EVENT_QOS:
    {
      gdouble proportion;
      GstClockTimeDiff diff;
      GstClockTime timestamp;

      gst_event_parse_qos (event, &proportion, &diff, &timestamp);

      /* we cannot randomly skip frame decoding since we don't have
       * B frames. we can however use the timestamp and diff to not
       * push late frames. This would at least save us the time to
       * crop/memcpy the data. */
      GST_OBJECT_LOCK (dec);
      dec->proportion = proportion;
      dec->earliest_time = timestamp + diff;
      GST_OBJECT_UNLOCK (dec);

      GST_DEBUG_OBJECT (dec, "got QoS %" GST_TIME_FORMAT ", %" G_GINT64_FORMAT,
          GST_TIME_ARGS (timestamp), diff);

      res = gst_pad_push_event (dec->sinkpad, event);
      break;
    }
    default:
      res = gst_pad_push_event (dec->sinkpad, event);
      break;
  }
done:
  gst_object_unref (dec);

  return res;

  /* ERRORS */
convert_error:
  {
    GST_DEBUG_OBJECT (dec, "could not convert format");
    goto done;
  }
}

static gboolean
theora_dec_sink_event (GstPad * pad, GstEvent * event)
{
  gboolean ret = FALSE;
  GstTheoraDec *dec;

  dec = GST_THEORA_DEC (gst_pad_get_parent (pad));

  GST_LOG_OBJECT (dec, "handling event");
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_START:
      ret = gst_pad_push_event (dec->srcpad, event);
      break;
    case GST_EVENT_FLUSH_STOP:
      gst_theora_dec_reset (dec);
      ret = gst_pad_push_event (dec->srcpad, event);
      break;
    case GST_EVENT_EOS:
      ret = gst_pad_push_event (dec->srcpad, event);
      break;
    case GST_EVENT_NEWSEGMENT:
    {
      gboolean update;
      GstFormat format;
      gdouble rate, arate;
      gint64 start, stop, time;

      gst_event_parse_new_segment_full (event, &update, &rate, &arate, &format,
          &start, &stop, &time);

      /* we need TIME format */
      if (format != GST_FORMAT_TIME)
        goto newseg_wrong_format;

      /* now configure the values */
      gst_segment_set_newsegment_full (&dec->segment, update,
          rate, arate, format, start, stop, time);
      dec->seqnum = gst_event_get_seqnum (event);

      /* We don't forward this unless/until the decoder is initialised */
      if (dec->have_header) {
        ret = gst_pad_push_event (dec->srcpad, event);
      } else {
        dec->pendingevents = g_list_append (dec->pendingevents, event);
        ret = TRUE;
      }
      break;
    }
    default:
      ret = gst_pad_push_event (dec->srcpad, event);
      break;
  }
done:
  gst_object_unref (dec);

  return ret;

  /* ERRORS */
newseg_wrong_format:
  {
    GST_DEBUG_OBJECT (dec, "received non TIME newsegment");
    gst_event_unref (event);
    goto done;
  }
}

static gboolean
theora_dec_setcaps (GstPad * pad, GstCaps * caps)
{
  GstTheoraDec *dec;
  GstStructure *s;

  dec = GST_THEORA_DEC (gst_pad_get_parent (pad));

  s = gst_caps_get_structure (caps, 0);

  /* parse the par, this overrides the encoded par */
  dec->have_par = gst_structure_get_fraction (s, "pixel-aspect-ratio",
      &dec->par_num, &dec->par_den);

  gst_object_unref (dec);

  return TRUE;
}

static GstFlowReturn
theora_handle_comment_packet (GstTheoraDec * dec, ogg_packet * packet)
{
  gchar *encoder = NULL;
  GstBuffer *buf;
  GstTagList *list;

  GST_DEBUG_OBJECT (dec, "parsing comment packet");

  buf = gst_buffer_new_and_alloc (packet->bytes);
  memcpy (GST_BUFFER_DATA (buf), packet->packet, packet->bytes);

  list =
      gst_tag_list_from_vorbiscomment_buffer (buf, (guint8 *) "\201theora", 7,
      &encoder);

  gst_buffer_unref (buf);

  if (!list) {
    GST_ERROR_OBJECT (dec, "couldn't decode comments");
    list = gst_tag_list_new ();
  }
  if (encoder) {
    gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
        GST_TAG_ENCODER, encoder, NULL);
    g_free (encoder);
  }
  gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
      GST_TAG_ENCODER_VERSION, dec->info.version_major,
      GST_TAG_VIDEO_CODEC, "Theora", NULL);

  if (dec->info.target_bitrate > 0) {
    gst_tag_list_add (list, GST_TAG_MERGE_REPLACE,
        GST_TAG_BITRATE, dec->info.target_bitrate,
        GST_TAG_NOMINAL_BITRATE, dec->info.target_bitrate, NULL);
  }

  dec->tags = list;

  return GST_FLOW_OK;
}

static GstFlowReturn
theora_handle_type_packet (GstTheoraDec * dec, ogg_packet * packet)
{
  GstCaps *caps;
  gint par_num, par_den;
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 bitstream_version;
  GList *walk;

  GST_DEBUG_OBJECT (dec, "fps %d/%d, PAR %d/%d",
      dec->info.fps_numerator, dec->info.fps_denominator,
      dec->info.aspect_numerator, dec->info.aspect_denominator);

  /* calculate par
   * the info.aspect_* values reflect PAR;
   * 0:0 is allowed and can be interpreted as 1:1, so correct for it.
   * x:0 for other x isn't technically allowed, but it's seen in the wild and
   * is reasonable to treat the same. 
   */
  if (dec->have_par) {
    /* we had a par on the sink caps, override the encoded par */
    GST_DEBUG_OBJECT (dec, "overriding with input PAR");
    par_num = dec->par_num;
    par_den = dec->par_den;
  } else {
    /* take encoded par */
    par_num = dec->info.aspect_numerator;
    par_den = dec->info.aspect_denominator;
  }
  if (par_den == 0) {
    par_num = par_den = 1;
  }
  /* theora has:
   *
   *  width/height : dimension of the encoded frame 
   *  frame_width/frame_height : dimension of the visible part
   *  offset_x/offset_y : offset in encoded frame where visible part starts
   */
  GST_DEBUG_OBJECT (dec, "dimension %dx%d, PAR %d/%d", dec->info.width,
      dec->info.height, par_num, par_den);
  GST_DEBUG_OBJECT (dec, "frame dimension %dx%d, offset %d:%d",
      dec->info.frame_width, dec->info.frame_height,
      dec->info.offset_x, dec->info.offset_y);
  if (dec->info.pixelformat != OC_PF_420) {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, DECODE,
        (NULL), ("pixel formats other than 4:2:0 not yet supported"));

    return GST_FLOW_ERROR;
  }

  if (dec->crop) {
    /* add black borders to make width/height/offsets even. we need this because
     * we cannot express an offset to the peer plugin. */
    dec->width =
        GST_ROUND_UP_2 (dec->info.frame_width + (dec->info.offset_x & 1));
    dec->height =
        GST_ROUND_UP_2 (dec->info.frame_height + (dec->info.offset_y & 1));
    dec->offset_x = dec->info.offset_x & ~1;
    dec->offset_y = dec->info.offset_y & ~1;
  } else {
    /* no cropping, use the encoded dimensions */
    dec->width = dec->info.width;
    dec->height = dec->info.height;
    dec->offset_x = 0;
    dec->offset_y = 0;
  }

  dec->granule_shift = _theora_ilog (dec->info.keyframe_frequency_force - 1);

  /* With libtheora-1.0beta1 the granulepos scheme was changed:
   * where earlier the granulepos refered to the index/beginning
   * of a frame, it now refers to the end, which matches the use
   * in vorbis/speex. We check the bitstream version from the header so
   * we know which way to interpret the incoming granuepos
   */
  bitstream_version = (dec->info.version_major << 16) |
      (dec->info.version_minor << 8) | dec->info.version_subminor;
  dec->is_old_bitstream = (bitstream_version <= 0x00030200);

  GST_DEBUG_OBJECT (dec, "after fixup frame dimension %dx%d, offset %d:%d",
      dec->width, dec->height, dec->offset_x, dec->offset_y);

  /* done */
  theora_decode_init (&dec->state, &dec->info);

  caps = gst_caps_new_simple ("video/x-raw-yuv",
      "format", GST_TYPE_FOURCC, GST_MAKE_FOURCC ('I', '4', '2', '0'),
      "framerate", GST_TYPE_FRACTION,
      dec->info.fps_numerator, dec->info.fps_denominator,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, par_num, par_den,
      "width", G_TYPE_INT, dec->width, "height", G_TYPE_INT, dec->height, NULL);
  gst_pad_set_caps (dec->srcpad, caps);
  gst_caps_unref (caps);

  dec->have_header = TRUE;

  if (dec->pendingevents) {
    for (walk = dec->pendingevents; walk; walk = g_list_next (walk))
      gst_pad_push_event (dec->srcpad, GST_EVENT_CAST (walk->data));
    g_list_free (dec->pendingevents);
    dec->pendingevents = NULL;
  }

  if (dec->tags) {
    gst_element_found_tags_for_pad (GST_ELEMENT_CAST (dec), dec->srcpad,
        dec->tags);
    dec->tags = NULL;
  }

  return ret;
}

static GstFlowReturn
theora_handle_header_packet (GstTheoraDec * dec, ogg_packet * packet)
{
  GstFlowReturn res;

  GST_DEBUG_OBJECT (dec, "parsing header packet");

  if (theora_decode_header (&dec->info, &dec->comment, packet))
    goto header_read_error;

  switch (packet->packet[0]) {
    case 0x81:
      res = theora_handle_comment_packet (dec, packet);
      break;
    case 0x82:
      res = theora_handle_type_packet (dec, packet);
      break;
    default:
      /* ignore */
      g_warning ("unknown theora header packet found");
    case 0x80:
      /* nothing special, this is the identification header */
      res = GST_FLOW_OK;
      break;
  }
  return res;

  /* ERRORS */
header_read_error:
  {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, DECODE,
        (NULL), ("couldn't read header packet"));
    return GST_FLOW_ERROR;
  }
}

/* returns TRUE if buffer is within segment, else FALSE.
 * if Buffer is on segment border, it's timestamp and duration will be clipped */
static gboolean
clip_buffer (GstTheoraDec * dec, GstBuffer * buf)
{
  gboolean res = TRUE;
  GstClockTime in_ts, in_dur, stop;
  gint64 cstart, cstop;

  in_ts = GST_BUFFER_TIMESTAMP (buf);
  in_dur = GST_BUFFER_DURATION (buf);

  GST_LOG_OBJECT (dec,
      "timestamp:%" GST_TIME_FORMAT " , duration:%" GST_TIME_FORMAT,
      GST_TIME_ARGS (in_ts), GST_TIME_ARGS (in_dur));

  /* can't clip without TIME segment */
  if (dec->segment.format != GST_FORMAT_TIME)
    goto beach;

  /* we need a start time */
  if (!GST_CLOCK_TIME_IS_VALID (in_ts))
    goto beach;

  /* generate valid stop, if duration unknown, we have unknown stop */
  stop =
      GST_CLOCK_TIME_IS_VALID (in_dur) ? (in_ts + in_dur) : GST_CLOCK_TIME_NONE;

  /* now clip */
  if (!(res = gst_segment_clip (&dec->segment, GST_FORMAT_TIME,
              in_ts, stop, &cstart, &cstop)))
    goto beach;

  /* update timestamp and possibly duration if the clipped stop time is
   * valid */
  GST_BUFFER_TIMESTAMP (buf) = cstart;
  if (GST_CLOCK_TIME_IS_VALID (cstop))
    GST_BUFFER_DURATION (buf) = cstop - cstart;

beach:
  GST_LOG_OBJECT (dec, "%sdropping", (res ? "not " : ""));
  return res;
}

/* FIXME, this needs to be moved to the demuxer */
static GstFlowReturn
theora_dec_push_forward (GstTheoraDec * dec, GstBuffer * buf)
{
  GstFlowReturn result = GST_FLOW_OK;
  GstClockTime outtime = GST_BUFFER_TIMESTAMP (buf);

  if (outtime == GST_CLOCK_TIME_NONE) {
    dec->queued = g_list_append (dec->queued, buf);
    GST_DEBUG_OBJECT (dec, "queued buffer");
  } else {
    if (dec->queued) {
      gint64 size;
      GList *walk;

      GST_DEBUG_OBJECT (dec, "first buffer with time %" GST_TIME_FORMAT,
          GST_TIME_ARGS (outtime));

      size = g_list_length (dec->queued);
      for (walk = dec->queued; walk; walk = g_list_next (walk)) {
        GstBuffer *buffer = GST_BUFFER (walk->data);
        GstClockTime time;

        time = outtime - gst_util_uint64_scale_int (size * GST_SECOND,
            dec->info.fps_denominator, dec->info.fps_numerator);

        GST_DEBUG_OBJECT (dec, "patch buffer %lld %lld", size, time);
        GST_BUFFER_TIMESTAMP (buffer) = time;
        /* Next timestamp - this one is duration */
        GST_BUFFER_DURATION (buffer) =
            (outtime - gst_util_uint64_scale_int ((size - 1) * GST_SECOND,
                dec->info.fps_denominator, dec->info.fps_numerator)) - time;

        if (dec->discont) {
          GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DISCONT);
          dec->discont = FALSE;
        }
        /* ignore the result.. */
        if (clip_buffer (dec, buffer))
          gst_pad_push (dec->srcpad, buffer);
        else
          gst_buffer_unref (buffer);
        size--;
      }
      g_list_free (dec->queued);
      dec->queued = NULL;
    }
    if (dec->discont) {
      GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
      dec->discont = FALSE;
    }
    if (clip_buffer (dec, buf))
      result = gst_pad_push (dec->srcpad, buf);
    else
      gst_buffer_unref (buf);
  }
  return result;
}

static GstFlowReturn
theora_dec_push_reverse (GstTheoraDec * dec, GstBuffer * buf)
{
  GstFlowReturn result = GST_FLOW_OK;

  dec->queued = g_list_prepend (dec->queued, buf);

  return result;
}

static GstFlowReturn
theora_handle_data_packet (GstTheoraDec * dec, ogg_packet * packet,
    GstClockTime outtime)
{
  /* normal data packet */
  yuv_buffer yuv;
  GstBuffer *out;
  guint i;
  gboolean keyframe;
  gint out_size;
  gint stride_y, stride_uv;
  gint width, height;
  gint cwidth, cheight;
  GstFlowReturn result;

  if (G_UNLIKELY (!dec->have_header))
    goto not_initialized;

  /* the second most significant bit of the first data byte is cleared 
   * for keyframes. We can only check it if it's not a zero-length packet. */
  keyframe = packet->bytes && ((packet->packet[0] & 0x40) == 0);
  if (G_UNLIKELY (keyframe)) {
    GST_DEBUG_OBJECT (dec, "we have a keyframe");
    dec->need_keyframe = FALSE;
  } else if (G_UNLIKELY (dec->need_keyframe)) {
    goto dropping;
  }

  GST_DEBUG_OBJECT (dec, "parsing data packet");

  /* this does the decoding */
  if (G_UNLIKELY (theora_decode_packetin (&dec->state, packet)))
    goto decode_error;

  if (outtime != -1) {
    gboolean need_skip;
    GstClockTime qostime;

    /* qos needs to be done on running time */
    qostime = gst_segment_to_running_time (&dec->segment, GST_FORMAT_TIME,
        outtime);

    GST_OBJECT_LOCK (dec);
    /* check for QoS, don't perform the last steps of getting and
     * pushing the buffers that are known to be late. */
    /* FIXME, we can also entirely skip decoding if the next valid buffer is 
     * known to be after a keyframe (using the granule_shift) */
    need_skip = dec->earliest_time != -1 && qostime <= dec->earliest_time;
    GST_OBJECT_UNLOCK (dec);

    if (need_skip)
      goto dropping_qos;
  }

  /* this does postprocessing and set up the decoded frame
   * pointers in our yuv variable */
  if (G_UNLIKELY (theora_decode_YUVout (&dec->state, &yuv) < 0))
    goto no_yuv;

  if (G_UNLIKELY ((yuv.y_width != dec->info.width)
          || (yuv.y_height != dec->info.height)))
    goto wrong_dimensions;

  width = dec->width;
  height = dec->height;
  cwidth = width / 2;
  cheight = height / 2;

  /* should get the stride from the caps, for now we round up to the nearest
   * multiple of 4 because some element needs it. chroma needs special 
   * treatment, see videotestsrc. */
  stride_y = GST_ROUND_UP_4 (width);
  stride_uv = GST_ROUND_UP_8 (width) / 2;

  out_size = stride_y * GST_ROUND_UP_2 (height) + stride_uv * GST_ROUND_UP_2 (height);

  /* now copy over the area contained in offset_x,offset_y,
   * frame_width, frame_height */
  result =
      gst_pad_alloc_buffer_and_set_caps (dec->srcpad, GST_BUFFER_OFFSET_NONE,
      out_size, GST_PAD_CAPS (dec->srcpad), &out);
  if (G_UNLIKELY (result != GST_FLOW_OK))
    goto no_buffer;

  /* copy the visible region to the destination. This is actually pretty
   * complicated and gstreamer doesn't support all the needed caps to do this
   * correctly. For example, when we have an odd offset, we should only combine
   * 1 row/column of luma samples with one chroma sample in colorspace conversion. 
   * We compensate for this by adding a black border around the image when the
   * offset or size is odd (see above).
   */
  {
    guchar *dest_y, *src_y;
    guchar *dest_u, *src_u;
    guchar *dest_v, *src_v;
    gint offset;

    dest_y = GST_BUFFER_DATA (out);
    dest_u = dest_y + stride_y * GST_ROUND_UP_2 (height);
    dest_v = dest_u + stride_uv * GST_ROUND_UP_2 (height) / 2;

    GST_LOG_OBJECT (dec, "plane 0, offset 0");
    GST_LOG_OBJECT (dec, "plane 1, offset %d", dest_u - dest_y);
    GST_LOG_OBJECT (dec, "plane 2, offset %d", dest_v - dest_y);

    src_y = yuv.y + dec->offset_x + dec->offset_y * yuv.y_stride;

    for (i = 0; i < height; i++) {
      memcpy (dest_y, src_y, width);

      dest_y += stride_y;
      src_y += yuv.y_stride;
    }

    offset = dec->offset_x / 2 + dec->offset_y / 2 * yuv.uv_stride;

    src_u = yuv.u + offset;
    src_v = yuv.v + offset;

    for (i = 0; i < cheight; i++) {
      memcpy (dest_u, src_u, cwidth);
      memcpy (dest_v, src_v, cwidth);

      dest_u += stride_uv;
      src_u += yuv.uv_stride;
      dest_v += stride_uv;
      src_v += yuv.uv_stride;
    }
  }

  GST_BUFFER_OFFSET (out) = dec->frame_nr;
  if (dec->frame_nr != -1)
    dec->frame_nr++;
  GST_BUFFER_OFFSET_END (out) = dec->frame_nr;
  if (dec->granulepos != -1) {
    gint64 cf = _theora_granule_frame (dec, dec->granulepos) + 1;

    GST_BUFFER_DURATION (out) = gst_util_uint64_scale_int (cf * GST_SECOND,
        dec->info.fps_denominator, dec->info.fps_numerator) - outtime;
  } else {
    GST_BUFFER_DURATION (out) =
        gst_util_uint64_scale_int (GST_SECOND, dec->info.fps_denominator,
        dec->info.fps_numerator);
  }
  GST_BUFFER_TIMESTAMP (out) = outtime;

  if (dec->segment.rate >= 0.0)
    result = theora_dec_push_forward (dec, out);
  else
    result = theora_dec_push_reverse (dec, out);

  return result;

  /* ERRORS */
not_initialized:
  {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, DECODE,
        (NULL), ("no header sent yet"));
    return GST_FLOW_ERROR;
  }
dropping:
  {
    GST_WARNING_OBJECT (dec, "dropping frame because we need a keyframe");
    dec->discont = TRUE;
    return GST_FLOW_OK;
  }
dropping_qos:
  {
    if (dec->frame_nr != -1)
      dec->frame_nr++;
    dec->discont = TRUE;
    GST_WARNING_OBJECT (dec, "dropping frame because of QoS");
    return GST_FLOW_OK;
  }
decode_error:
  {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, DECODE,
        (NULL), ("theora decoder did not decode data packet"));
    return GST_FLOW_ERROR;
  }
no_yuv:
  {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, DECODE,
        (NULL), ("couldn't read out YUV image"));
    return GST_FLOW_ERROR;
  }
wrong_dimensions:
  {
    GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, FORMAT,
        (NULL), ("dimensions of image do not match header"));
    return GST_FLOW_ERROR;
  }
no_buffer:
  {
    GST_DEBUG_OBJECT (dec, "could not get buffer, reason: %s",
        gst_flow_get_name (result));
    return result;
  }
}

static GstFlowReturn
theora_dec_decode_buffer (GstTheoraDec * dec, GstBuffer * buf)
{
  ogg_packet packet;
  GstFlowReturn result = GST_FLOW_OK;

  /* make ogg_packet out of the buffer */
  packet.packet = GST_BUFFER_DATA (buf);
  packet.bytes = GST_BUFFER_SIZE (buf);
  packet.granulepos = GST_BUFFER_OFFSET_END (buf);
  packet.packetno = 0;          /* we don't really care */
  packet.b_o_s = dec->have_header ? 0 : 1;
  /* EOS does not matter for the decoder */
  packet.e_o_s = 0;

  GST_LOG_OBJECT (dec, "decode buffer of size %ld", packet.bytes);

  if (dec->have_header) {
    if (packet.granulepos != -1) {
      dec->granulepos = packet.granulepos;
      dec->last_timestamp = _theora_granule_start_time (dec, packet.granulepos);
    } else if (dec->last_timestamp != -1) {
      dec->last_timestamp = _theora_granule_start_time (dec, dec->granulepos);
    }
    if (dec->last_timestamp == -1 && GST_BUFFER_TIMESTAMP_IS_VALID (buf))
      dec->last_timestamp = GST_BUFFER_TIMESTAMP (buf);
  } else {
    dec->last_timestamp = -1;
  }

  GST_DEBUG_OBJECT (dec, "header=%02x packetno=%lld, granule pos=%"
      G_GINT64_FORMAT ", outtime=%" GST_TIME_FORMAT,
      packet.bytes ? packet.packet[0] : -1, packet.packetno, packet.granulepos,
      GST_TIME_ARGS (dec->last_timestamp));

  /* switch depending on packet type. A zero byte packet is always a data
   * packet; we don't dereference it in that case. */
  if (packet.bytes && packet.packet[0] & 0x80) {
    if (dec->have_header) {
      GST_WARNING_OBJECT (GST_OBJECT (dec), "Ignoring header");
      goto done;
    }
    result = theora_handle_header_packet (dec, &packet);
  } else {
    result = theora_handle_data_packet (dec, &packet, dec->last_timestamp);
  }

done:
  /* interpolate granule pos */
  dec->granulepos = _inc_granulepos (dec, dec->granulepos);

  return result;
}

/* For reverse playback we use a technique that can be used for
 * any keyframe based video codec.
 *
 * Input:
 *  Buffer decoding order:  7  8  9  4  5  6  1  2  3  EOS
 *  Keyframe flag:                      K        K
 *  Discont flag:           D        D        D
 *
 * - Each Discont marks a discont in the decoding order.
 * - The keyframes mark where we can start decoding.
 *
 * First we prepend incomming buffers to the gather queue, whenever we receive
 * a discont, we flush out the gather queue.
 *
 * The above data will be accumulated in the gather queue like this:
 *
 *   gather queue:  9  8  7
 *                        D
 *
 * Whe buffer 4 is received (with a DISCONT), we flush the gather queue like
 * this:
 *
 *   while (gather)
 *     take head of queue and prepend to decode queue.
 *     if we copied a keyframe, decode the decode queue.
 *
 * After we flushed the gather queue, we add 4 to the (now empty) gather queue.
 * We get the following situation:
 *
 *  gather queue:    4
 *  decode queue:    7  8  9
 *
 * After we received 5 (Keyframe) and 6:
 *
 *  gather queue:    6  5  4
 *  decode queue:    7  8  9
 *
 * When we receive 1 (DISCONT) which triggers a flush of the gather queue:
 *
 *   Copy head of the gather queue (6) to decode queue:
 *
 *    gather queue:    5  4
 *    decode queue:    6  7  8  9
 *
 *   Copy head of the gather queue (5) to decode queue. This is a keyframe so we
 *   can start decoding.
 *
 *    gather queue:    4
 *    decode queue:    5  6  7  8  9
 *
 *   Decode frames in decode queue, store raw decoded data in output queue, we
 *   can take the head of the decode queue and prepend the decoded result in the
 *   output queue:
 *
 *    gather queue:    4
 *    decode queue:    
 *    output queue:    9  8  7  6  5
 *
 *   Now output all the frames in the output queue, picking a frame from the
 *   head of the queue.
 *
 *   Copy head of the gather queue (4) to decode queue, we flushed the gather
 *   queue and can now store input buffer in the gather queue:
 *
 *    gather queue:    1
 *    decode queue:    4
 *
 *  When we receive EOS, the queue looks like:
 *
 *    gather queue:    3  2  1
 *    decode queue:    4
 *
 *  Fill decode queue, first keyframe we copy is 2:
 *
 *    gather queue:    1
 *    decode queue:    2  3  4
 *
 *  Decoded output:
 *
 *    gather queue:    1
 *    decode queue:    
 *    output queue:    4  3  2
 *
 *  Leftover buffer 1 cannot be decoded and must be discarded.
 */
static GstFlowReturn
theora_dec_flush_decode (GstTheoraDec * dec)
{
  GstFlowReturn res = GST_FLOW_OK;

  while (dec->decode) {
    GstBuffer *buf = GST_BUFFER_CAST (dec->decode->data);

    GST_DEBUG_OBJECT (dec, "decoding buffer %p, ts %" GST_TIME_FORMAT,
        buf, GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)));

    /* decode buffer, prepend to output queue */
    res = theora_dec_decode_buffer (dec, buf);

    /* don't need it anymore now */
    gst_buffer_unref (buf);

    dec->decode = g_list_delete_link (dec->decode, dec->decode);
  }
  while (dec->queued) {
    GstBuffer *buf = GST_BUFFER_CAST (dec->queued->data);

    /* iterate ouput queue an push downstream */
    res = gst_pad_push (dec->srcpad, buf);

    dec->queued = g_list_delete_link (dec->queued, dec->queued);
  }

  return res;
}

static GstFlowReturn
theora_dec_chain_reverse (GstTheoraDec * dec, gboolean discont, GstBuffer * buf)
{
  GstFlowReturn res = GST_FLOW_OK;

  /* if we have a discont, move buffers to the decode list */
  if (G_UNLIKELY (discont)) {
    GST_DEBUG_OBJECT (dec, "received discont,gathering buffers");
    while (dec->gather) {
      GstBuffer *gbuf;
      guint8 *data;

      gbuf = GST_BUFFER_CAST (dec->gather->data);
      /* remove from the gather list */
      dec->gather = g_list_delete_link (dec->gather, dec->gather);
      /* copy to decode queue */
      dec->decode = g_list_prepend (dec->decode, gbuf);

      /* if we copied a keyframe, flush and decode the decode queue */
      data = GST_BUFFER_DATA (gbuf);
      if ((data[0] & 0x40) == 0) {
        GST_DEBUG_OBJECT (dec, "copied keyframe");
        res = theora_dec_flush_decode (dec);
      }
    }
  }

  /* add buffer to gather queue */
  GST_DEBUG_OBJECT (dec, "gathering buffer %p, size %u", buf,
      GST_BUFFER_SIZE (buf));
  dec->gather = g_list_prepend (dec->gather, buf);

  return res;
}

static GstFlowReturn
theora_dec_chain_forward (GstTheoraDec * dec, gboolean discont,
    GstBuffer * buffer)
{
  GstFlowReturn result;

  result = theora_dec_decode_buffer (dec, buffer);

  gst_buffer_unref (buffer);

  return result;
}

static GstFlowReturn
theora_dec_chain (GstPad * pad, GstBuffer * buf)
{
  GstTheoraDec *dec;
  GstFlowReturn res;
  gboolean discont;

  dec = GST_THEORA_DEC (gst_pad_get_parent (pad));

  /* peel of DISCONT flag */
  discont = GST_BUFFER_IS_DISCONT (buf);

  /* resync on DISCONT */
  if (G_UNLIKELY (discont)) {
    GST_DEBUG_OBJECT (dec, "received DISCONT buffer");
    dec->need_keyframe = TRUE;
    dec->last_timestamp = -1;
    dec->granulepos = -1;
    dec->discont = TRUE;
  }

  if (dec->segment.rate > 0.0)
    res = theora_dec_chain_forward (dec, discont, buf);
  else
    res = theora_dec_chain_reverse (dec, discont, buf);

  gst_object_unref (dec);

  return res;
}

static GstStateChangeReturn
theora_dec_change_state (GstElement * element, GstStateChange transition)
{
  GstTheoraDec *dec = GST_THEORA_DEC (element);
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      theora_info_init (&dec->info);
      theora_comment_init (&dec->comment);
      GST_DEBUG_OBJECT (dec, "Setting have_header to FALSE in READY->PAUSED");
      dec->have_header = FALSE;
      dec->have_par = FALSE;
      gst_theora_dec_reset (dec);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = parent_class->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      theora_clear (&dec->state);
      theora_comment_clear (&dec->comment);
      theora_info_clear (&dec->info);
      gst_theora_dec_reset (dec);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}

static void
theora_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTheoraDec *dec = GST_THEORA_DEC (object);

  switch (prop_id) {
    case ARG_CROP:
      dec->crop = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
theora_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstTheoraDec *dec = GST_THEORA_DEC (object);

  switch (prop_id) {
    case ARG_CROP:
      g_value_set_boolean (value, dec->crop);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
