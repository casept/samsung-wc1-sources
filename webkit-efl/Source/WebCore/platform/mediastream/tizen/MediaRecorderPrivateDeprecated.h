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

#ifndef MediaRecorderPrivateDeprecated_h
#define MediaRecorderPrivateDeprecated_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "MediaRecorderPrivate.h"
#include <wtf/PassOwnPtr.h>

#include <gst/gst.h>

namespace WebCore {

class MediaRecorder;
class MediaStreamDescriptor;
class MediaStreamPlayer;
class SensorItem;

class MediaRecorderPrivate : public MediaRecorderPrivateInterface {
public:
    static PassOwnPtr<MediaRecorderPrivateInterface> create(MediaRecorder* recorder);
    virtual ~MediaRecorderPrivate();

    virtual void record(MediaStreamDescriptor*);
    virtual void save(String& filename);

private:
    MediaRecorderPrivate(MediaRecorder*);

    MediaRecorder* m_recorder;
    MediaStreamPlayer& m_player;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaRecorderPrivateDeprecated_h
