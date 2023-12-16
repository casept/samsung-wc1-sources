/*
 * Copyright (C) 2014 Lukasz Marek (l.marek@samsung.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef HTMLTimeElement_h
#define HTMLTimeElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLTimeElement : public HTMLElement {
public:
    static PassRefPtr<HTMLTimeElement> create(const QualifiedName&, Document*);

    String dateTime() const;
    void setDateTime(const String&);

private:
    HTMLTimeElement(const QualifiedName&, Document*);
};

inline bool isHTMLTimeElement(const Node* node)
{
    return node->hasTagName(HTMLNames::timeTag);
}

inline bool isHTMLTimeElement(const Element* element)
{
    return element->hasTagName(HTMLNames::timeTag);
}

inline HTMLTimeElement* toHTMLTimeElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isHTMLTimeElement(node));
    return static_cast<HTMLTimeElement*>(node);
}

} //namespace

#endif
