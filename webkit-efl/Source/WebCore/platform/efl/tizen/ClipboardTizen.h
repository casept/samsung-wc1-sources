/*
 *  Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 *  Copyright (C) 2009-2010 ProFUSION embedded systems
 *  Copyright (C) 2009-2013 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ClipboardTizen_h
#define ClipboardTizen_h

#if ENABLE(TIZEN_CLIPBOARD)

#include "Clipboard.h"
#include "CachedImageClient.h"
#include "DataObjectTizen.h"

namespace WebCore {

class CachedImage;

// State available during IE's events for drag and drop and copy/paste
class ClipboardTizen : public Clipboard, public CachedImageClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<ClipboardTizen> create(ClipboardAccessPolicy policy, ClipboardType clipboardType, Frame* frame)
    {
        return adoptRef(new ClipboardTizen(policy, clipboardType, frame));
    }

    static PassRefPtr<ClipboardTizen> create(ClipboardAccessPolicy policy, PassRefPtr<DataObjectTizen> dataObject, ClipboardType clipboardType, Frame* frame)
    {
        return adoptRef(new ClipboardTizen(policy, dataObject, clipboardType, frame));
    }
    ~ClipboardTizen();

    void clearData(const String&);
    void clearData();
    void clearAllData();
    String getData(const String&) const;
    void setData(const String&, const String&);

    ListHashSet<String> types() const;
    virtual PassRefPtr<FileList> files() const;

    IntPoint dragLocation() const;
    CachedImage* dragImage() const;
    void setDragImage(CachedImage*, const IntPoint&);
    Node* dragImageElement();
    void setDragImageElement(Node*, const IntPoint&);

    virtual DragImageRef createDragImage(IntPoint&) const;
    virtual void declareAndWriteDragImage(Element*, const KURL&, const String&, Frame*);
    virtual void writeURL(const KURL&, const String&, Frame*);
    virtual void writeRange(Range*, Frame*);

    virtual bool hasData();

    virtual void writePlainText(const String&);

#if ENABLE(DATA_TRANSFER_ITEMS)
    virtual PassRefPtr<DataTransferItemList> items();
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
    PassRefPtr<DataObjectTizen> dataObject() { return m_dataObject; }
#endif // ENABLE(TIZEN_DRAG_SUPPORT)

private:
    ClipboardTizen(ClipboardAccessPolicy, ClipboardType, Frame*);
    ClipboardTizen(ClipboardAccessPolicy, PassRefPtr<DataObjectTizen>, ClipboardType, Frame*);

    void setDragImage(CachedImage*, Node*, const IntPoint&);
    void makeImageDirectory();

    RefPtr<DataObjectTizen> m_dataObject;
    Frame* m_frame;
    String m_imageDirectory;
};
}

#endif // ENABLE(TIZEN_CLIPBOARD)

#endif // ClipboardTizen_h
