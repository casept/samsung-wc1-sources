/*
   Copyright (C) 2013 Samsung Electronics

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
#include "ewk_console_message.h"

#include "ewk_console_message_private.h"

Ewk_Console_Message::Ewk_Console_Message(unsigned level, const String& message, unsigned int lineNumber, const String& source)
    : m_message(message.utf8())
    , m_source(source.utf8())
    , m_level(level)
    , m_line(lineNumber)
{
}

Ewk_Console_Message_Level ewk_console_message_level_get(const Ewk_Console_Message *message)
{
    return static_cast<Ewk_Console_Message_Level>(message->m_level);
}

const char* ewk_console_message_text_get(const Ewk_Console_Message *message)
{
    return message->m_message.data();
}

unsigned ewk_console_message_line_get(const Ewk_Console_Message *message)
{
    return message->m_line;
}

const char* ewk_console_message_source_get(const Ewk_Console_Message *message)
{
    return message->m_source.data();
}

