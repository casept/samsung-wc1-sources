/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"
#include "TizenLinkEffect.h"

#if ENABLE(TIZEN_UI_TAP_SOUND)
#include <feedback/feedback.h>
#include <dlfcn.h>

namespace WebCore {

static int (*feedback_play_type)(int, int);

static void* getHandle()
{
    void* handle = dlopen("libfeedback.so.0", RTLD_NOW);
    if (!handle)
        return 0;

    int (*feedback_initialize)() = (int(*)(void))dlsym(handle, "feedback_initialize");
    if (!feedback_initialize) {
        dlclose(handle);
        return 0;
    }

    feedback_play_type = (int(*)(int, int))dlsym(handle, "feedback_play_type");
    if (!feedback_play_type) {
        dlclose(handle);
        return 0;
    }

    feedback_initialize();
    return handle;
}

void TizenLinkEffect::playLinkEffect()
{
    static void* handle = getHandle();

    if (!handle)
        return;

    feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TAP);
}

}
#endif