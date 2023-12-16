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

#ifndef DataObjectTizen_h
#define DataObjectTizen_h

#include "KURL.h"
#include "Range.h"
#include "SharedBuffer.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

namespace WebCore {

class DataObjectTizen : public RefCounted<DataObjectTizen> {
public:
    static PassRefPtr<DataObjectTizen> create()
    {
        return adoptRef(new DataObjectTizen());
    }

    String text() const;
    String markup() const;
    String uriList() const { return m_uriList; }
    KURL url() const { return m_url; }
    String image() const { return m_image; }
    Vector<String> filenames() const { return m_filenames; }
    String urlLabel() const;

    void setText(const String& text);
    void setMarkup(const String& markup);
    void setURIList(const String& uriListString);
    void setURL(const KURL& url, const String& label);
    void setImage(const String& image) { m_image = image; }
    void setRange(PassRefPtr<Range> range) { m_range = range; }

    bool hasText() const { return m_range || !m_text.isEmpty(); }
    bool hasMarkup() const { return m_range || !m_markup.isEmpty(); }
    bool hasURIList() const { return !m_uriList.isEmpty(); }
    bool hasURL() const { return !m_url.isEmpty() && m_url.isValid(); }
    bool hasImage() const { return !m_image.isEmpty(); }
    bool hasFilenames() const { return !m_filenames.isEmpty(); }

    void clearAllExceptFilenames();
    void clearAll();
    void clearText();
    void clearMarkup();
    void clearURIList() { m_uriList = ""; }
    void clearURL() { m_url = KURL(); }
    void clearImage() { m_image = ""; }

    static DataObjectTizen* sharedDataObject();

private:
    DataObjectTizen();

    String m_text;
    String m_markup;
    String m_uriList;
    String m_image;
    KURL m_url;
    Vector<String> m_filenames;
    RefPtr<Range> m_range;
};

}

#endif // DataObjectTizen_h
