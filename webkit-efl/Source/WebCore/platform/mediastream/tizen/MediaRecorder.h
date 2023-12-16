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

#ifndef MediaRecorder_h
#define MediaRecorder_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "MediaRecorderPrivate.h"
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class MediaStreamDescriptor;

class MediaRecorderClient {
public:
    virtual ~MediaRecorderClient() { }

    virtual void onRecorded(const String&) { }
    virtual void onRecordingStateChange(const String&) { }
};

class MediaRecorder {
    WTF_MAKE_NONCOPYABLE(MediaRecorder); 
    WTF_MAKE_FAST_ALLOCATED;
public:

    static PassOwnPtr<MediaRecorder> create(MediaRecorderClient* client)
    {
        return adoptPtr(new MediaRecorder(client));
    }
    virtual ~MediaRecorder();

    bool record(MediaStreamDescriptor*);
    void save();
    void cancel();

    void onRecorded(const String& path);
    void onRecordingStateChange(const String& state);

    void setCustomFileName(const String& fileName) { m_private->setCustomFileName(fileName); }
    void setMaxFileSizeBytes(int maxFileSizeBytes) { m_private->setMaxFileSizeBytes(maxFileSizeBytes); }
    void setRecordingFormat(const String& format) { m_private->setRecordingFormat(format); }
private:
    explicit MediaRecorder(MediaRecorderClient*);

    MediaRecorderClient* m_client;
    OwnPtr<MediaRecorderPrivateInterface> m_private;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaRecorder_h
