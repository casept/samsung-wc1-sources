/*
 * Copyright (C) 2001 Robert Penner. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS"AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Robert Penner's easing equations. http://www.robertpenner.com/easing/
 * static function easeInOutQuad (t:Number, b:Number, c:Number, d:Number):Number {
 *     if ((t/=d/2) < 1) return c/2*t*t + b;
 *     return -c/2 * ((--t)*(t-2) - 1) + b;
 * }
 */

#include "config.h"
#include "EasingCurves.h"

namespace WebKit {

float easeInOutQuad(float time, const float base, const float constant, const unsigned duration)
{
    if (time > duration || time < 0)
        return 0;

    if ((time /= (duration / 2)) < 1)
        return (constant / 2) * time * time + base;

    time -= 1;
    return -1 * (constant / 2) * (time * (time - 2) - 1) + base;
}

#if ENABLE(TIZEN_FLICK_FASTER)
/*
 * float Cubic::easeOut(float t,float b , float c, float d) {
 *   return c * ((t = t / d - 1) * t * t + 1) + b;
 *}
 */
float easeOutCubic(float time, const float base, const float constant, const unsigned duration)
{
    if (time > duration || time < 0)
        return 0;

    time = time / duration - 1;
    return constant * (time * time * time + 1) + base;
}
#endif
} // namespace WebKit
