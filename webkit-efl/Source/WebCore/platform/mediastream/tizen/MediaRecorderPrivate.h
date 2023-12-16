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

#ifndef MediaRecorderPrivate_h
#define MediaRecorderPrivate_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class MediaStreamDescriptor;

class MediaRecorderPrivateInterface {
    WTF_MAKE_NONCOPYABLE(MediaRecorderPrivateInterface);
    WTF_MAKE_FAST_ALLOCATED;
public:
    MediaRecorderPrivateInterface() { }
    virtual ~MediaRecorderPrivateInterface() { }

    virtual bool record(MediaStreamDescriptor*) = 0;
    virtual void save(String& filename) = 0;
    virtual void cancel() = 0;
    virtual void setCustomFileName(const String& fileName) = 0;
    virtual void setMaxFileSizeBytes(int) = 0;
    virtual void setRecordingFormat(const String& format) = 0;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaRecorderPrivate_h
