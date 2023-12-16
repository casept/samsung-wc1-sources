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

#include "config.h"
#include "WebDateTimeChooser.h"

#if ENABLE(TIZEN_INPUT_DATE_TIME)
#include "WebPage.h"
#include <WebCore/DateTimeChooserClient.h>

using namespace WebCore;

namespace WebKit {
WebDateTimeChooser::WebDateTimeChooser(WebPage* page, DateTimeChooserClient* client, const DateTimeChooserParameters& parameter)
    : m_dateTimeChooserClient(client)
    , m_page(page)
{
    m_page->setActiveDateTimeChooser(this, parameter);
}

WebDateTimeChooser::~WebDateTimeChooser()
{
    m_dateTimeChooserClient = 0;
}

void WebDateTimeChooser::didEndChooser(const String& value)
{
    if (!m_dateTimeChooserClient)
        return;

    if (!value.isEmpty())
        m_dateTimeChooserClient->didChooseValue(value);

    m_dateTimeChooserClient->didEndChooser();
}

void WebDateTimeChooser::endChooser()
{
    m_dateTimeChooserClient = 0;
}

} // namespace WebKit

#endif // ENABLE(TIZEN_INPUT_DATE_TIME)
