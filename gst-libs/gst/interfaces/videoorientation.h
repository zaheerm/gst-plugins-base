/* GStreamer
 * Copyright (C) 2006 Nokia <stefan.kost@nokia.com
 *
 * videoorientation.h: video flipping and centering interface
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

#ifndef __GST_VIDEO_ORIENTATION_H__
#define __GST_VIDEO_ORIENTATION_H__

#include <gst/gst.h>
#include <gst/interfaces/interfaces-enumtypes.h>

G_BEGIN_DECLS

#define GST_TYPE_VIDEO_ORIENTATION \
  (gst_video_orientation_get_type ())
#define GST_VIDEO_ORIENTATION(obj) \
  (GST_IMPLEMENTS_INTERFACE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_VIDEO_ORIENTATION, GstVideoOrientation))
#define GST_IS_VIDEO_ORIENTATION(obj) \
  (GST_IMPLEMENTS_INTERFACE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_VIDEO_ORIENTATION))
#define GST_VIDEO_ORIENTATION_GET_IFACE(inst) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GST_TYPE_VIDEO_ORIENTATION, GstVideoOrientationInterface))

typedef struct _GstVideoOrientation GstVideoOrientation;

typedef struct _GstVideoOrientationInterface {
  GTypeInterface parent;

  /* virtual functions */
  gboolean (* get_hflip)   (GstVideoOrientation *video_orientation, gboolean *flip);
  gboolean (* get_vflip)   (GstVideoOrientation *video_orientation, gboolean *flip);
  gboolean (* get_hcenter) (GstVideoOrientation *video_orientation, gint *center);
  gboolean (* get_vcenter) (GstVideoOrientation *video_orientation, gint *center);

  gboolean (* set_hflip)   (GstVideoOrientation *video_orientation, gboolean flip);
  gboolean (* set_vflip)   (GstVideoOrientation *video_orientation, gboolean flip);
  gboolean (* set_hcenter) (GstVideoOrientation *video_orientation, gint center);
  gboolean (* set_vcenter) (GstVideoOrientation *video_orientation, gint center);

  gpointer _gst_reserved[GST_PADDING];
} GstVideoOrientationInterface;

GType           gst_video_orientation_get_type              (void);

/* virtual class function wrappers */
gboolean gst_video_orientation_get_hflip (GstVideoOrientation *video_orientation, gboolean *flip);
gboolean gst_video_orientation_get_vflip (GstVideoOrientation *video_orientation, gboolean *flip);
gboolean gst_video_orientation_get_hcenter (GstVideoOrientation *video_orientation, gint *center);
gboolean gst_video_orientation_get_vcenter (GstVideoOrientation *video_orientation, gint *center);

gboolean gst_video_orientation_set_hflip (GstVideoOrientation *video_orientation, gboolean flip);
gboolean gst_video_orientation_set_vflip (GstVideoOrientation *video_orientation, gboolean flip);
gboolean gst_video_orientation_set_hcenter (GstVideoOrientation *video_orientation, gint center);
gboolean gst_video_orientation_set_vcenter (GstVideoOrientation *video_orientation, gint center);

G_END_DECLS

#endif /* __GST_VIDEO_ORIENTATION_H__ */