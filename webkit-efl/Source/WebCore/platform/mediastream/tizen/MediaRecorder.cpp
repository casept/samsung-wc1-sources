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
#include "MediaRecorder.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "MediaRecorderPrivate.h"
#include "MediaRecorderPrivateImpl.h"
#include "MediaStreamDescriptor.h"

namespace WebCore {

MediaRecorder::MediaRecorder(MediaRecorderClient* client)
    : m_client(client)
    , m_private(MediaRecorderPrivate::create(this))
{
}

MediaRecorder::~MediaRecorder()
{
    m_client = 0;
}

bool MediaRecorder::record(MediaStreamDescriptor* descriptor)
{
    return m_private->record(descriptor);
}

void MediaRecorder::save()
{
    String filename;
    m_private->save(filename);

    onRecorded(filename);
}

void MediaRecorder::cancel()
{
    m_private->cancel();
}

void MediaRecorder::onRecorded(const String& path)
{
    if (m_client)
        m_client->onRecorded(path);
}

void MediaRecorder::onRecordingStateChange(const String& state)
{
    if (m_client)
        m_client->onRecordingStateChange(state);
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
