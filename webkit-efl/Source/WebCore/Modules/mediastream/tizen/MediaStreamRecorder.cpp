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

#include "config.h"
#include "MediaStreamRecorder.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "Blob.h"
#include "BlobCallback.h"
#include "MediaRecorder.h"
#include "MediaStreamDescriptor.h"

namespace WebCore {

MediaStreamRecorder::MediaStreamRecorder()
    : m_recorder(MediaRecorder::create(this))
    , m_callback(0)
{
}

MediaStreamRecorder::~MediaStreamRecorder()
{
}

void MediaStreamRecorder::record(MediaStreamDescriptor* descriptor)
{
    m_recorder->record(descriptor);
}

void MediaStreamRecorder::getRecordedData(PassRefPtr<BlobCallback> callback)
{
    if (!callback)
        return;

    m_callback = callback;
    m_recorder->save();
}

void MediaStreamRecorder::cancel()
{
    m_recorder->cancel();
}

void MediaStreamRecorder::onRecorded(const String& path)
{
    if (!m_callback)
        return;

    OwnPtr<BlobData> blobData = BlobData::create();
    blobData->appendFile(path);
    RefPtr<Blob> blob = Blob::create(blobData.release(), BlobDataItem::toEndOfFile);

    m_callback->handleEvent(blob.get());
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
