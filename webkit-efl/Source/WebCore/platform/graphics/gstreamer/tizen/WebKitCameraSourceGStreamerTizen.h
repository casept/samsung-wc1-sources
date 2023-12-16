/*
    Copyright (C) 2012 Samsung Electronics.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef WebKitCameraSourceGStreamer_h
#define WebKitCameraSourceGStreamer_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "Frame.h"
#include <gst/gst.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_CAMERA_SRC            (webkit_camera_src_get_type ())
#define WEBKIT_CAMERA_SRC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBKIT_TYPE_CAMERA_SRC, WebKitCameraSrc))
#define WEBKIT_CAMERA_SRC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WEBKIT_TYPE_CAMERA_SRC, WebKitCameraSrcClass))
#define WEBKIT_IS_CAMERA_SRC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBKIT_TYPE_CAMERA_SRC))
#define WEBKIT_IS_CAMERA_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBKIT_TYPE_CAMERA_SRC))

typedef struct _WebKitCameraSrc        WebKitCameraSrc;
typedef struct _WebKitCameraSrcClass   WebKitCameraSrcClass;
typedef struct _WebKitCameraSrcPrivate WebKitCameraSrcPrivate;

struct _WebKitCameraSrc {
    GstBin parent;

    WebKitCameraSrcPrivate *priv;
};

struct _WebKitCameraSrcClass {
    GstBinClass parentClass;
};

GType webkit_camera_src_get_type(void);

G_END_DECLS

#endif // ENABLE(TIZEN_MEDIA_STREAM)
#endif // WebKitCameraSourceGStreamer_h
