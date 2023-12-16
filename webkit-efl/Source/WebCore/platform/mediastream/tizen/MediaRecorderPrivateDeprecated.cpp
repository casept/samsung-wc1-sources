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
#include "MediaRecorderPrivateDeprecated.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "MediaRecorder.h"
#include "MediaStreamPlayer.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

PassOwnPtr<MediaRecorderPrivateInterface> MediaRecorderPrivate::create(MediaRecorder* recorder)
{
    return adoptPtr(new MediaRecorderPrivate(recorder));
}

MediaRecorderPrivate::MediaRecorderPrivate(MediaRecorder* recorder)
    : m_recorder(recorder)
    , m_player(MediaStreamPlayer::instance())
{
}

MediaRecorderPrivate::~MediaRecorderPrivate()
{
}

void MediaRecorderPrivate::record(MediaStreamDescriptor* descriptor)
{
    m_player.record(descriptor);
}

void MediaRecorderPrivate::save(String& filename)
{
    m_player.save(filename);
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
