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
#include "URIUtils.h"

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "MediaStream.h"
#include "MediaStreamRegistry.h"
#endif // ENABLE(TIZEN_MEDIA_STREAM)

namespace WebCore {

static void crackBlobURI(const String& url)
{
#if ENABLE(TIZEN_MEDIA_STREAM)
    MediaStreamDescriptor* descriptor = MediaStreamRegistry::registry().lookupMediaStreamDescriptor(url);
    if (descriptor) {
        for (unsigned int i = 0; i < descriptor->numberOfVideoComponents(); i++) {
            MediaStreamSource* source = descriptor->videoComponent(i)->source();
            if (source) {
                int cameraId = (source->name() == "Self camera") ? 1 : 0;
                String& mutableUrl = const_cast<String&>(url);
                String cameraUrl = String::format("camera://%d", cameraId);
                mutableUrl.swap(cameraUrl);
                return;
            }
        }
    }
#endif // ENABLE(TIZEN_MEDIA_STREAM)
}

void crackURI(const String& url)
{
    if (url.contains("blob:"))
        crackBlobURI(url);
}

} // namespace WebCore
