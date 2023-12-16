/*
    Copyright (C) 2013 Samsung Electronics.

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

#ifndef CameraImageCaptureImpl_h
#define CameraImageCaptureImpl_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "CameraControl.h"
#include "CameraError.h"

#include <gst/gst.h>
#include <gst/interfaces/cameracontrol.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/CString.h>

namespace WebCore {

class CameraErrorCallback;
class CameraImageCaptureImpl;
class CameraSuccessCallback;
class LocalMediaServer;

class CameraImageCaptureImpl : public RefCounted<CameraImageCaptureImpl> {
public:
    enum CameraPictureFormat {
        PictureFormatJPEG,
        PictureFormatPNG
    };
    static PassRefPtr<CameraImageCaptureImpl> create(CameraControl* cameraControl) { return adoptRef(new CameraImageCaptureImpl(cameraControl)); }
    CameraImageCaptureImpl(CameraControl*);
    virtual ~CameraImageCaptureImpl();

    void takePicture(PassRefPtr<CameraSuccessCallback>, PassRefPtr<CameraErrorCallback>);
    void setCustomFileName(const String& fileName) { m_customFileName = fileName; }
    void setPictureSize(const IntSize& size) { m_pictureSize = size; }
    bool setPictureFormat(const String& pictureFormat);
    bool initialize();
    void release();
    void writeGstBufferToFile(GstBuffer* mainImage, const String& customFileName, const IntSize& size, CameraImageCaptureImpl::CameraPictureFormat pictureFormat, bool isRawDataYUV420);
    void callErrorCallback(CameraError::Code);
    void callSuccessCallback();

private:
    CameraControl* m_cameraControl;
    RefPtr<CameraSuccessCallback> m_successCb;
    RefPtr<CameraErrorCallback> m_errorCb;
    GstElement* cameraSource() const;
    static void takePictureCallbackForV4l2Src(GstPad*, GstBuffer* buffer, gpointer userData);
    static void takePictureCallbackForCameraSrc(GstElement* object, GstBuffer* mainImage, GstBuffer* thubnailImage, GstBuffer* screennailImage, gpointer userData);
    String m_customFileName;
    IntSize m_pictureSize;
    IntSize m_previewSize;
    bool m_initialized;
    CameraPictureFormat m_pictureFormat;
    unsigned int m_probeId;
    LocalMediaServer& m_localMediaServer;
    bool m_isNV21Format;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // CameraImageCaptureImpl_h
