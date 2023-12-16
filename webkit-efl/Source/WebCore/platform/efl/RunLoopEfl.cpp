/*
 * Copyright (C) 2012 ProFUSION embedded systems. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RunLoop.h"

#include <Ecore.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "./mediastream/tizen/LocalMediaServer.h"
#endif

static const int ecorePipeMessageSize = 1;
static const char wakupEcorePipeMessage[] = "W";

namespace WebCore {

RunLoop::RunLoop()
    : m_initEfl(false)
#if ENABLE(TIZEN_RUNLOOP_WAKEUP_ERROR_WORKAROUND)
    , m_wakeUpEventState(WakeUpEventIdle)
#else
    , m_wakeUpEventRequested(false)
#endif
{
    m_pipe = adoptPtr(ecore_pipe_add(wakeUpEvent, this));
    m_initEfl = true;
}

RunLoop::~RunLoop()
{
}

void RunLoop::run()
{
    ecore_main_loop_begin();
}

void RunLoop::stop()
{
#if ENABLE(TIZEN_MEDIA_STREAM)
    LocalMediaServer::instance().clear();
#endif
    ecore_main_loop_quit();
}

void RunLoop::wakeUpEvent(void* data, void*, unsigned int)
{
    RunLoop* loop = static_cast<RunLoop*>(data);

    {
#if ENABLE(TIZEN_RUNLOOP_WAKEUP_ERROR_WORKAROUND)
        MutexLocker locker(loop->m_wakeUpEventStateLock);
        loop->m_wakeUpEventState = WakeUpEventRunning;
#else
        MutexLocker locker(loop->m_wakeUpEventRequestedLock);
        loop->m_wakeUpEventRequested = false;
#endif
    }

    loop->performWork();

#if ENABLE(TIZEN_RUNLOOP_WAKEUP_ERROR_WORKAROUND)
    {
        MutexLocker locker(loop->m_wakeUpEventStateLock);
        loop->m_wakeUpEventState = WakeUpEventIdle;
    }
#endif
}

void RunLoop::wakeUp()
{
#if ENABLE(TIZEN_RUNLOOP_WAKEUP_ERROR_WORKAROUND)
    {
        MutexLocker locker(m_wakeUpEventStateLock);
        if (m_wakeUpEventState == WakeUpEventRequested)
            return;
        else if (m_wakeUpEventState == WakeUpEventRunning)
            m_pipe.clear();
        m_wakeUpEventState = WakeUpEventRequested;
    }

    if (!m_pipe)
        m_pipe = adoptPtr(ecore_pipe_add(wakeUpEvent, this));
#else
    {
        MutexLocker locker(m_wakeUpEventRequestedLock);
        if (m_wakeUpEventRequested)
            return;
        m_wakeUpEventRequested = true;
    }
#endif

    {
        MutexLocker locker(m_pipeLock);
#if ENABLE(TIZEN_RUNLOOP_WAKEUP_ERROR_WORKAROUND)
        while(1) {
            if (ecore_pipe_write(m_pipe.get(), wakupEcorePipeMessage, ecorePipeMessageSize))
                return;

            LOG_ERROR("Failed to write a wakupEcorePipeMessage\n");
            m_pipe = adoptPtr(ecore_pipe_add(wakeUpEvent, this)); // due to OwnPtr, ecore_pipe_del is called automatically.
        }
#else
        ecore_pipe_write(m_pipe.get(), wakupEcorePipeMessage, ecorePipeMessageSize);
#endif
    }
}

RunLoop::TimerBase::TimerBase(RunLoop*)
    : m_timer(0)
    , m_isRepeating(false)
{
}

RunLoop::TimerBase::~TimerBase()
{
    stop();
}

bool RunLoop::TimerBase::timerFired(void* data)
{
    RunLoop::TimerBase* timer = static_cast<RunLoop::TimerBase*>(data);

    if (!timer->m_isRepeating)
        timer->m_timer = 0;

    timer->fired();

    return timer->m_isRepeating ? ECORE_CALLBACK_RENEW : ECORE_CALLBACK_CANCEL;
}

void RunLoop::TimerBase::start(double nextFireInterval, bool repeat)
{
    if (isActive())
        stop();

    m_isRepeating = repeat;
    ASSERT(!m_timer);
    m_timer = ecore_timer_add(nextFireInterval, reinterpret_cast<Ecore_Task_Cb>(timerFired), this);
}

void RunLoop::TimerBase::stop()
{
    if (m_timer) {
        ecore_timer_del(m_timer);
        m_timer = 0;
    }
}

bool RunLoop::TimerBase::isActive() const
{
    return (m_timer) ? true : false;
}

} // namespace WebCore
