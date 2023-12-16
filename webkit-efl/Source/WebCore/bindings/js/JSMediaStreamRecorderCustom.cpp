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
#include "JSMediaStreamRecorder.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "CallbackFunction.h"
#include "JSBlobCallback.h"
#include "Navigator.h"

namespace WebCore {

using namespace JSC;

JSValue JSMediaStreamRecorder::getRecordedData(ExecState* exec)
{
    // Arguments: callback

    RefPtr<BlobCallback> callback = createFunctionOnlyCallback<JSBlobCallback>(exec, static_cast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), exec->argument(0));
    if (exec->hadException())
        return jsUndefined();

    m_impl->getRecordedData(callback.release());

    return jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
