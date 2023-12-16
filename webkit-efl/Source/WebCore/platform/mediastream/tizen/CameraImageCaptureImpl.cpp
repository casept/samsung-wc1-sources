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

#include "config.h"
#include "CameraImageCaptureImpl.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "CameraErrorCallback.h"
#include "CameraImageCapture.h"
#include "CameraSuccessCallback.h"
#include "GStreamerUtilities.h"
#include "GStreamerVersioning.h"
#include "JPEGImageEncoder.h"
#include "LocalMediaServer.h"
#include "PNGImageEncoder.h"
#include <wtf/Functional.h>
#include <wtf/MainThread.h>

#define IMAGE_FILE_PATH "/opt/usr/media/Images/"

#define IMAGE_CAPTURE_COUNT 1

namespace WebCore {

CameraImageCaptureImpl::CameraImageCaptureImpl(CameraControl* cameraControl)
    : m_cameraControl(cameraControl)
    , m_pictureSize(IntSize(cameraWidth, cameraHeight))
    , m_previewSize(IntSize(0, 0))
    , m_initialized(false)
    , m_pictureFormat(PictureFormatJPEG)
    , m_probeId(0)
    , m_localMediaServer(LocalMediaServer::instance())
    , m_isNV21Format(false)
{
}

CameraImageCaptureImpl::~CameraImageCaptureImpl()
{
}

static unsigned char convertIntToUnsignedChar(int value)
{
    if (value > 255)
        return 255;
    if (value < 0)
        return 0;
    return static_cast<unsigned char>(value);
}

static void convertYUV420_NV21toRGBA(unsigned char* data, unsigned char* dst, int width, int height) {
    int size = width * height;
    int offset = size;
    int u, v;
    int yIndex[4];

    for(int i = 0, k = 0; i < size; i += 2, k += 2) {
        yIndex[0] = i;
        yIndex[1] = i + 1;
        yIndex[2] = width + i;
        yIndex[3] = width + i + 1;

        v = data[offset + k  ] - 128;
        u = data[offset + k + 1] - 128;

        int tmpR = 1.772f * v;
        int tmpG = 0.344f * v + 0.714f * u;
        int tmpB = 1.402f * u;

        for (int j = 0; j < 4; j++) {
            dst[yIndex[j] * 4] = convertIntToUnsignedChar(data[yIndex[j]] + tmpR);
            dst[yIndex[j] * 4 + 1] = convertIntToUnsignedChar(data[yIndex[j]] - tmpG);
            dst[yIndex[j] * 4 + 2] = convertIntToUnsignedChar(data[yIndex[j]] + tmpB);
            dst[yIndex[j] * 4 + 3] = 255;
        }

        if (i != 0 && (i + 2) % width == 0)
            i += width;
    }
}

static void convertYUV420ToRGBA(unsigned char* src, unsigned char* dst, int width, int height)
{
    const int dstPixelSize = 4;
    int imageSize = width * height;
    int stride = width * dstPixelSize;
    int vIndex = imageSize;
    int uIndex = imageSize + static_cast<int>(imageSize / 4.0);

    int yVal[4], uVal, vVal;
    unsigned char* dstPtr[4];
    unsigned char* srcPtr = src;
    int x, y, i, dstBaseIndex;
    double tmpY, tmpU, tmpV;

    for (y = 0; y <= height - 2; y += 2) {
        for (x = 0; x <= width - 2; x += 2) {
            dstBaseIndex = stride * y + x * dstPixelSize;
            uVal = src[uIndex];
            vVal = src[vIndex];

            yVal[0] = *srcPtr;
            yVal[1] = *(srcPtr + 1);
            yVal[2] = *(srcPtr + width);
            yVal[3] = *(srcPtr + width + 1);

            dstPtr[0] = dst + dstBaseIndex;
            dstPtr[1] = dstPtr[0] + dstPixelSize;
            dstPtr[2] = dstPtr[0] + stride;
            dstPtr[3] = dstPtr[2] + dstPixelSize;

            for (i = 0; i < 4; i++) {
                tmpY = 1.164 * (yVal[i] - 16);
                tmpU = uVal - 128;
                tmpV = vVal - 128;
                dstPtr[i][0] = convertIntToUnsignedChar(tmpY + 2.018 * tmpU);
                dstPtr[i][1] = convertIntToUnsignedChar(tmpY - 0.813 * tmpV - 0.391 * tmpU);
                dstPtr[i][2] = convertIntToUnsignedChar(tmpY + 1.596 * tmpV);
                dstPtr[i][3] = 255;
            }

            uIndex++;
            vIndex++;
            srcPtr += 2;
        }
        srcPtr += width;
    }
}

static void convertYUY2ToRGBA(unsigned char* src, unsigned char* dst, int width, int height)
{
    const int cy = 76284;
    const int cu1 = 104595;
    const int cu2 = -53280;
    const int cv1 = -25624;
    const int cv2 = 132252;

    int col, row;
    unsigned char* yuv;
    unsigned char* rgb;
    long y1, y2, v1, u1;

    for (row = 0; row < height; ++row) {
        yuv = src + width * 2 * row;
        rgb = dst + width * 4 * row;
        for (col = 0; col < width; col += 2) {
            y1 = yuv[0];
            y2 = yuv[2];
            v1 = yuv[1];
            u1 = yuv[3];

            rgb[0] = convertIntToUnsignedChar((cy * (y1 - 16) >> 16) + (cu1 * (u1 - 128) >> 16));
            rgb[1] = convertIntToUnsignedChar((cy * (y1 - 16) >> 16) + (cu2 * (u1 - 128) >> 16) + (cv1 * (v1 - 128) >> 16));
            rgb[2] = convertIntToUnsignedChar((cy * (y1 - 16) >> 16) + (cv2 * (v1 - 128) >> 16));
            rgb[3] = 255;

            rgb[4] = convertIntToUnsignedChar((cy * (y2 - 16) >> 16) + (cu1 * (u1 - 128) >> 16));
            rgb[5] = convertIntToUnsignedChar((cy * (y2 - 16) >> 16) + (cu2 * (u1 - 128) >> 16) + (cv1 * (v1 - 128) >> 16));
            rgb[6] = convertIntToUnsignedChar((cy * (y2 - 16) >> 16) + (cv2 * (v1 - 128) >> 16));
            rgb[7] = 255;

            yuv += 4;
            rgb += 8;
        }
    }
}

void CameraImageCaptureImpl::writeGstBufferToFile(GstBuffer* mainImage, const String& customFileName, const IntSize& size, CameraImageCaptureImpl::CameraPictureFormat pictureFormat, bool isRawDataYUV420)
{
    String fileName;
    if (customFileName.isEmpty()) {
        GTimeZone* tz = g_time_zone_new_local();
        GDateTime* dt = g_date_time_new_now(tz);

        gchar* tstr = 0;
        if (pictureFormat == CameraImageCaptureImpl::PictureFormatJPEG)
            tstr = g_date_time_format(dt, IMAGE_FILE_PATH"%s.jpg");
        else if (pictureFormat == CameraImageCaptureImpl::PictureFormatPNG)
            tstr = g_date_time_format(dt, IMAGE_FILE_PATH"%s.png");
        fileName = String(tstr);
        g_free(tstr);
    } else
        fileName = IMAGE_FILE_PATH + customFileName;

    TIZEN_LOGI("[Media] fileName : %s, width : %d, height : %d. buffersize: %d", fileName.utf8().data(), size.width(), size.height(), GST_BUFFER_SIZE(mainImage));

    const int rgbaPixelSize = 4;
    Vector<unsigned char> pixels(size.area() * rgbaPixelSize);

    if (isRawDataYUV420)
        convertYUV420ToRGBA(GST_BUFFER_DATA(mainImage), pixels.data(), size.width(), size.height());
    else if (m_isNV21Format)
        convertYUV420_NV21toRGBA(GST_BUFFER_DATA(mainImage), pixels.data(), size.width(), size.height());
    else
        convertYUY2ToRGBA(GST_BUFFER_DATA(mainImage), pixels.data(), size.width(), size.height());

    gst_buffer_unref(mainImage);

    Vector<char> imageData;
    if (pictureFormat == CameraImageCaptureImpl::PictureFormatJPEG)
        compressRGBABigEndianToJPEG(pixels.data(), size, imageData, 0);
    else if (pictureFormat == CameraImageCaptureImpl::PictureFormatPNG)
        compressRGBABigEndianToPNG(pixels.data(), size, imageData);

    FILE* fstream = fopen(fileName.utf8().data(), "w");
    if (!fstream) {
        callErrorCallback(CameraError::FILE_WRITE_ERR);
        return;
    }

    fwrite(imageData.data(), imageData.size(), 1, fstream);
    fclose(fstream);

    callSuccessCallback();
}

static bool isValidGstBuffer(GstBuffer* buffer)
{
    const GstCaps* caps = gst_buffer_get_caps(buffer);
    if (!caps)
        return false;

    const GstStructure* structure = gst_caps_get_structure(caps, 0);
    if (!structure)
        return false;

    return true;
}

void CameraImageCaptureImpl::takePictureCallbackForV4l2Src(GstPad* pad, GstBuffer* mainImage, gpointer userData)
{
    CameraImageCaptureImpl* cameraImageCapture = static_cast<CameraImageCaptureImpl*>(userData);

    if (!isValidGstBuffer(mainImage)) {
        callOnMainThread(bind(&CameraImageCaptureImpl::callErrorCallback, cameraImageCapture, CameraError::PIPELINE_ERR));
        return;
    }

    GstElement* cameraSource = cameraImageCapture->cameraSource();
    if (!cameraSource) {
        callOnMainThread(bind(&CameraImageCaptureImpl::callErrorCallback, cameraImageCapture, CameraError::PIPELINE_ERR));
        return;
    }

    if (cameraImageCapture->m_probeId) {
        gst_pad_remove_buffer_probe(pad, cameraImageCapture->m_probeId);
        cameraImageCapture->m_probeId = 0;
    }

    GstBuffer* copiedBuffer = gst_buffer_copy(mainImage);
    callOnMainThread(bind(&CameraImageCaptureImpl::writeGstBufferToFile, cameraImageCapture, copiedBuffer, cameraImageCapture->m_customFileName, cameraImageCapture->m_pictureSize, cameraImageCapture->m_pictureFormat, true));
}

void CameraImageCaptureImpl::takePictureCallbackForCameraSrc(GstElement*, GstBuffer* mainImage, GstBuffer* thumbnailImage, GstBuffer* screennailImage, gpointer userData)
{
    UNUSED_PARAM(thumbnailImage);
    UNUSED_PARAM(screennailImage);

    CameraImageCaptureImpl* cameraImageCapture = static_cast<CameraImageCaptureImpl*>(userData);

    if (!isValidGstBuffer(mainImage)) {
        callOnMainThread(bind(&CameraImageCaptureImpl::callErrorCallback, cameraImageCapture, CameraError::PIPELINE_ERR));
        return;
    }

    GstElement* cameraSource = cameraImageCapture->cameraSource();
    if (!cameraSource) {
        callOnMainThread(bind(&CameraImageCaptureImpl::callErrorCallback, cameraImageCapture, CameraError::PIPELINE_ERR));
        return;
    }

    GstCameraControl* control = GST_CAMERA_CONTROL(cameraSource);
    gst_camera_control_stop_auto_focus(control);
    gst_camera_control_set_capture_command(control, GST_CAMERA_CONTROL_CAPTURE_COMMAND_STOP);
    g_object_set(G_OBJECT(cameraSource), "stop-video", FALSE, NULL);

    GstBuffer* copiedBuffer = gst_buffer_copy(mainImage);
    callOnMainThread(bind(&CameraImageCaptureImpl::writeGstBufferToFile, cameraImageCapture, copiedBuffer, cameraImageCapture->m_customFileName, cameraImageCapture->m_pictureSize, cameraImageCapture->m_pictureFormat, false));
}

GstElement* CameraImageCaptureImpl::cameraSource() const
{
    return m_localMediaServer.camSource();
}

void CameraImageCaptureImpl::callErrorCallback(CameraError::Code code)
{
    if (m_localMediaServer.srcType() == LocalMediaServer::V4l2Src)
        m_localMediaServer.changeResolution(m_previewSize.width(), m_previewSize.height());
    if (m_errorCb) {
        RefPtr<CameraError> error = CameraError::create(code);
        m_errorCb->handleEvent(error.get());
    }
}

void CameraImageCaptureImpl::callSuccessCallback()
{
    if (m_localMediaServer.srcType() == LocalMediaServer::V4l2Src)
        m_localMediaServer.changeResolution(m_previewSize.width(), m_previewSize.height());
    if (m_successCb)
        m_successCb->handleEvent();
}

void CameraImageCaptureImpl::takePicture(PassRefPtr<CameraSuccessCallback> successCallback, PassRefPtr<CameraErrorCallback> errorCallback)
{
    m_errorCb = errorCallback;
    m_successCb = successCallback;

    if (!initialize()) {
        callErrorCallback(CameraError::PIPELINE_ERR);
        return;
    }

    ASSERT(cameraSource());

    TIZEN_LOGI("[Media] size : %d X %d", m_pictureSize.width(), m_pictureSize.height());

    if (m_localMediaServer.srcType() == LocalMediaServer::V4l2Src) {
        GRefPtr<GstPad> srcpad = adoptGRef(gst_element_get_static_pad(cameraSource(), "src"));
        m_previewSize = IntSize(m_localMediaServer.width(), m_localMediaServer.height());
        m_localMediaServer.changeResolution(m_pictureSize.width(), m_pictureSize.height());
        m_probeId = gst_pad_add_buffer_probe(srcpad.get(), G_CALLBACK(takePictureCallbackForV4l2Src), static_cast<gpointer>(this));
    } else if (m_localMediaServer.srcType() == LocalMediaServer::CameraSrc) {
        g_object_set(G_OBJECT(cameraSource()), "capture-width", m_pictureSize.width(), NULL);
        g_object_set(G_OBJECT(cameraSource()), "capture-height", m_pictureSize.height(), NULL);
        g_object_set(G_OBJECT(cameraSource()), "capture-count", IMAGE_CAPTURE_COUNT, NULL);

        g_object_set(G_OBJECT(cameraSource()), "stop-video", TRUE, NULL);
        GstCameraControl* control = GST_CAMERA_CONTROL(cameraSource());
        gst_camera_control_set_capture_command(control, GST_CAMERA_CONTROL_CAPTURE_COMMAND_START);
    } else {
        callErrorCallback(CameraError::PIPELINE_ERR);
        return;
    }

    m_cameraControl->image()->onShutter();
}

bool CameraImageCaptureImpl::setPictureFormat(const String& pictureFormat)
{
    if (pictureFormat == "jpeg" || pictureFormat == "jpg")
        m_pictureFormat = PictureFormatJPEG;
    else if (pictureFormat == "png")
        m_pictureFormat = PictureFormatPNG;
    else
        return false;

    return true;
}

bool CameraImageCaptureImpl::initialize()
{
    if (m_initialized)
        return true;

    if (!cameraSource())
        return false;

    if (m_localMediaServer.srcType() == LocalMediaServer::CameraSrc) {
        g_signal_connect(G_OBJECT(cameraSource()), "still-capture", G_CALLBACK(takePictureCallbackForCameraSrc), this);
        CameraDeviceCapabilities& cameraDeviceCapabilities = CameraDeviceCapabilities::instance();
        m_isNV21Format = cameraDeviceCapabilities.supportedCaptureFormats().contains(CAMERA_PIXEL_FORMAT_NV21);

        if (m_isNV21Format)
            g_object_set(G_OBJECT(cameraSource()), "capture-fourcc", GST_MAKE_FOURCC ('N', 'V', '2', '1'), NULL);
        else
            g_object_set(G_OBJECT(cameraSource()), "capture-fourcc", GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'), NULL);
    }
    m_initialized = true;

    return true;
}

void CameraImageCaptureImpl::release()
{
    if (m_localMediaServer.srcType() == LocalMediaServer::CameraSrc)
        g_signal_handlers_disconnect_by_func(G_OBJECT(cameraSource()), reinterpret_cast<gpointer>(takePictureCallbackForCameraSrc), this);

    if (m_probeId) {
        GRefPtr<GstPad> srcpad = adoptGRef(gst_element_get_static_pad(cameraSource(), "src"));
        gst_pad_remove_buffer_probe(srcpad.get(), m_probeId);
        m_probeId = 0;
    }
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

