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
 */

#include "config.h"
#include "HTMLTimeElement.h"

#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLTimeElement::HTMLTimeElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(timeTag));
}

PassRefPtr<HTMLTimeElement> HTMLTimeElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLTimeElement(tagName, document));
}

String HTMLTimeElement::dateTime() const
{
    String value = getAttribute(datetimeAttr);
    if (value.isEmpty() || value.isNull())
        value = innerHTML();
    return value;
}

void HTMLTimeElement::setDateTime(const String &value)
{
    setAttribute(datetimeAttr, value);
}

}
