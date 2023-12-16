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
#include "MediaStreamManager.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "LocalMediaServer.h"
#include "MediaStreamSourcesQueryClient.h"
#include <wtf/MainThread.h>

namespace WebCore {

MediaStreamManager& MediaStreamManager::instance()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(MediaStreamManager, manager, ());
    return manager;
}

MediaStreamManager::MediaStreamManager()
{
}

MediaStreamManager::~MediaStreamManager()
{
}

bool MediaStreamManager::startLocalMediaStream(PassRefPtr<MediaStreamSourcesQueryClient> client)
{
#if ENABLE(TIZEN_EMULATOR)
    if (!client.get()->video() && client.get()->audio())
        return LocalMediaServer::instance().audioStreamIsAvailable();

    if (client.get()->video() && client.get()->audio())
        return LocalMediaServer::instance().audioStreamIsAvailable() && LocalMediaServer::instance().startStream();
#endif

    if (client.get()->video())
        return LocalMediaServer::instance().startStream();

    return true;
}

void MediaStreamManager::stopLocalMediaStream()
{
    LocalMediaServer::instance().stopStream();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
