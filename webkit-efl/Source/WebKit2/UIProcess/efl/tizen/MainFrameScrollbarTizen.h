/*
   Copyright (C) 2014 Samsung Electronics

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

#ifndef MainFrameScrollbarTizen_h
#define MainFrameScrollbarTizen_h

#include "ScrollTypes.h"
#include <WebCore/IntRect.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>


#if ENABLE(TIZEN_SCROLLBAR)

namespace WebKit {

class MainFrameScrollbarTizen : public RefCounted<MainFrameScrollbarTizen> {
public:
    static PassRefPtr<MainFrameScrollbarTizen> createNativeScrollbar(Evas_Object* ewkView, WebCore::ScrollbarOrientation orientation);

    ~MainFrameScrollbarTizen();

    void frameRectChanged();

    void updateThumbPositionAndProportion();

    void setFrameRect(const WebCore::IntRect&);
    void setPosition(int position);
    void setProportion(int visibleSize, int totalSize);

    void setEnabled(bool enabled);

    Evas_Object* ewkView() { return m_view; }
    int totalSize() { return m_totalSize; }
    int visibleSize() { return m_visibleSize; }

private:
    MainFrameScrollbarTizen(Evas_Object* view, WebCore::ScrollbarOrientation orientation);

    Evas_Object* m_view;
    Evas_Object* m_object;

    WebCore::IntRect m_frameRect;

    int m_position;
    int m_totalSize;
    int m_visibleSize;
    bool m_isEnabled;
};

} // namespace WebKit
#endif // ENABLE(TIZEN_SCROLLBAR)


#endif // MainFrameScrollbarTizen_h
