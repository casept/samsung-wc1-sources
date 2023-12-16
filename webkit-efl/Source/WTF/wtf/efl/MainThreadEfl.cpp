/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 * Copyright (C) 2008 Diego Gonzalez
 * Copyright (C) 2008 Kenneth Rohde Christiansen
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MainThread.h"

#include <Ecore.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/StdLibExtras.h>

#if ENABLE(TIZEN_MAIN_THREAD_SCHEDULE_DISCARD_DUPLICATE_REQUEST)
#include "Threading.h"
#endif // ENABLE(TIZEN_MAIN_THREAD_SCHEDULE_DISCARD_DUPLICATE_REQUEST)

namespace WTF {

static OwnPtr<Ecore_Pipe>& pipeObject()
{
    DEFINE_STATIC_LOCAL(OwnPtr<Ecore_Pipe>, pipeObject, ());
    return pipeObject;
}

#if ENABLE(TIZEN_MAIN_THREAD_SCHEDULE_DISCARD_DUPLICATE_REQUEST)
static Mutex& scheduleRequestedMutex()
{
    DEFINE_STATIC_LOCAL(Mutex, staticMutex, ());
    return staticMutex;
}

static bool& scheduleRequested()
{
    DEFINE_STATIC_LOCAL(bool, staticScheduleRequested, ());
    return staticScheduleRequested;
}
#endif // ENABLE(TIZEN_MAIN_THREAD_SCHEDULE_DISCARD_DUPLICATE_REQUEST)

static void monitorDispatchFunctions(void*, void*, unsigned int)
{
#if ENABLE(TIZEN_MAIN_THREAD_SCHEDULE_DISCARD_DUPLICATE_REQUEST)
    {
        MutexLocker locker(scheduleRequestedMutex());
        scheduleRequested() = false;
    }
#endif // ENABLE(TIZEN_MAIN_THREAD_SCHEDULE_DISCARD_DUPLICATE_REQUEST)
    dispatchFunctionsFromMainThread();
}

void initializeMainThreadPlatform()
{
#if ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)
    if (!ecore_init())
        return;
#endif // ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)
    pipeObject() = adoptPtr(ecore_pipe_add(monitorDispatchFunctions, 0));
}

void scheduleDispatchFunctionsOnMainThread()
{
#if ENABLE(TIZEN_MAIN_THREAD_SCHEDULE_DISCARD_DUPLICATE_REQUEST)
    {
        MutexLocker locker(scheduleRequestedMutex());
        if (scheduleRequested())
            return;
        scheduleRequested() = true;
    }

    Eina_Bool result = false;
    while (!result) {
        result = ecore_pipe_write(pipeObject().get(), "", 0);
        if (!result)
            pipeObject() = adoptPtr(ecore_pipe_add(monitorDispatchFunctions, 0));
    }
#else
    ecore_pipe_write(pipeObject().get(), "", 0);
#endif // ENABLE(TIZEN_MAIN_THREAD_SCHEDULE_DISCARD_DUPLICATE_REQUEST)
}

}
