/*
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Virtual keys defined in http://www.atsc.org/cms/standards/a100/a_100_2.pdf
// Same volume virtual keys are already defined in Windows virtual keys but we overlap with the keys defined in DTV.
#undef VK_PLAY
#undef VK_VOLUME_UP
#undef VK_VOLUME_DOWN
#undef VK_VOLUME_MUTE

#define VK_COLORED_KEY_0 0x193
#define VK_COLORED_KEY_1 0x194
#define VK_COLORED_KEY_2 0x195
#define VK_COLORED_KEY_3 0x196

#define VK_REWIND 0x19C
#define VK_STOP 0x19D
#define VK_PLAY 0x19F
#define VK_RECORD 0x1A0
#define VK_FAST_FWD 0x1A1

#define VK_CHANNEL_UP 0x1AB
#define VK_CHANNEL_DOWN 0x1AC

#define VK_VOLUME_UP 0x1BF
#define VK_VOLUME_DOWN 0x1C0
#define VK_VOLUME_MUTE 0x1C1

#define VK_INFO 0x1C9
