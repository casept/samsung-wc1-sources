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

#ifndef WebDateTimeChooser_h
#define WebDateTimeChooser_h

#if ENABLE(TIZEN_INPUT_DATE_TIME)
#include <WebCore/DateTimeChooser.h>

namespace WebCore {
class DateTimeChooserClient;
}

namespace WebKit {

class WebPage;

class WebDateTimeChooser : public WebCore::DateTimeChooser {
public:
    WebDateTimeChooser(WebPage*, WebCore::DateTimeChooserClient*, const WebCore::DateTimeChooserParameters& parameter);
    virtual ~WebDateTimeChooser();

    void didEndChooser(const String& value);
    virtual void endChooser() OVERRIDE;

private:
    WebCore::DateTimeChooserClient* m_dateTimeChooserClient;
    WebPage* m_page;
};

} // namespace WebKit

#endif // ENABLE(TIZEN_INPUT_DATE_TIME)

#endif // WebDateTimeChooser_h
