/*
   Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

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
#include "DataObjectTizen.h"

#include "EditorClient.h"
#include "markup.h"
#include <Ecore_File.h>
#include <Evas.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

static void replaceNonBreakingSpaceWithSpace(String& str)
{
    static const UChar NonBreakingSpaceCharacter = 0xA0;
    static const UChar SpaceCharacter = ' ';
    str.replace(NonBreakingSpaceCharacter, SpaceCharacter);
}

DataObjectTizen::DataObjectTizen()
{
}

String DataObjectTizen::text() const
{
    if (m_range)
        return m_range->text();
    return m_text;
}

String DataObjectTizen::markup() const
{
    if (m_range)
        return createMarkup(m_range.get(), 0, AnnotateForInterchange, false, ResolveNonLocalURLs);
    return m_markup;
}

void DataObjectTizen::setText(const String& text)
{
    m_range = 0;
    m_text = text;
    replaceNonBreakingSpaceWithSpace(m_text);
}

void DataObjectTizen::setMarkup(const String& markup)
{
    m_range = 0;
    m_markup = markup;
}

void DataObjectTizen::setURIList(const String& uriListString)
{
    m_uriList = uriListString;

    // This code is originally from: platform/chromium/ChromiumDataObject.cpp.
    // FIXME: We should make this code cross-platform eventually.

    // Line separator is \r\n per RFC 2483 - however, for compatibility
    // reasons we also allow just \n here.
    Vector<String> uriList;
    uriListString.split('\n', uriList);

    // Process the input and copy the first valid URL into the url member.
    // In case no URLs can be found, subsequent calls to getData("URL")
    // will get an empty string. This is in line with the HTML5 spec (see
    // "The DragEvent and DataTransfer interfaces"). Also extract all filenames
    // from the URI list.
    bool setURL = false;
    for (size_t i = 0; i < uriList.size(); ++i) {
        String& line = uriList[i];
        line = line.stripWhiteSpace();
        if (line.isEmpty())
            continue;
        if (line[0] == '#')
            continue;

        KURL url = KURL(KURL(), line);
        if (url.isValid()) {
            if (!setURL) {
                m_url = url;
                setURL = true;
            }
            m_filenames.append(String::fromUTF8(ecore_file_file_get(line.utf8().data())));
        }
    }
}

void DataObjectTizen::setURL(const KURL& url, const String& label)
{
    m_url = url;
    m_uriList = url;
    setText(url.string());

    String actualLabel(label);
    if (actualLabel.isEmpty())
        actualLabel = url;

    StringBuilder markup;
    markup.append("<a href=\"");
    markup.append(url.string());
    markup.append("\">");
    markup.append(evas_textblock_text_markup_to_utf8(0, actualLabel.utf8().data()));
    markup.append("</a>");
    setMarkup(markup.toString());
}

void DataObjectTizen::clearText()
{
    m_range = 0;
    m_text = "";
}

void DataObjectTizen::clearMarkup()
{
    m_range = 0;
    m_markup = "";
}

String DataObjectTizen::urlLabel() const
{
    if (hasText())
        return text();

    if (hasURL())
        return url();

    return String();
}

void DataObjectTizen::clearAllExceptFilenames()
{
    m_text = "";
    m_markup = "";
    m_uriList = "";
    m_url = KURL();
    m_image = "";
    m_range = 0;
}

void DataObjectTizen::clearAll()
{
    clearAllExceptFilenames();
    m_filenames.clear();
}

DataObjectTizen* DataObjectTizen::sharedDataObject()
{
    static RefPtr<DataObjectTizen> dataObject = DataObjectTizen::create();
    return dataObject.get();
}

}