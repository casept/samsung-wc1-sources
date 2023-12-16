/*
 * Copyright (C) 2014 Samsung Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InputPicker_h
#define InputPicker_h

#if ENABLE(TIZEN_INPUT_DATE_TIME)
#include <string.h>

#if USE(EO)
typedef struct _Eo_Opaque Eo;
typedef Eo Evas_Object;
#else
typedef struct _Evas_Object Evas_Object;
#endif

namespace WebKit {

class InputPicker {
public:
    InputPicker(Evas_Object* ewkView);
    ~InputPicker();

    void show(bool isTimeType, const char* value);
private:
    static void clickedCallback(void* data, Evas_Object* obj, void* event_info);
    static void canceledCallback(void* data, Evas_Object* obj, void* event_info);

    bool m_isTimeType;
    Evas_Object* m_ewkView;
    Evas_Object* m_layout;
    Evas_Object* m_datetime;
};

} // namespace WebKit

#endif // TIZEN_INPUT_DATE_TIME
#endif // InputPicker_h
