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

/**
 * @file    ewk_console_message.h
 * @brief   Describes the Console Message API.
 */

#ifndef ewk_console_message_internal_h
#define ewk_console_message_internal_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for @a Ewk_Console_Message. */
typedef struct Ewk_Console_Message Ewk_Console_Message;

/// Creates a type name for Ewk_Console_Message_Level.
typedef enum {
    EWK_CONSOLE_MESSAGE_LEVEL_TIP,
    EWK_CONSOLE_MESSAGE_LEVEL_LOG,
    EWK_CONSOLE_MESSAGE_LEVEL_WARNING,
    EWK_CONSOLE_MESSAGE_LEVEL_ERROR,
    EWK_CONSOLE_MESSAGE_LEVEL_DEBUG
} Ewk_Console_Message_Level;

EAPI Ewk_Console_Message_Level ewk_console_message_level_get(const Ewk_Console_Message *message);

EAPI const char *ewk_console_message_text_get(const Ewk_Console_Message *message);

EAPI unsigned ewk_console_message_line_get(const Ewk_Console_Message *message);

EAPI const char *ewk_console_message_source_get(const Ewk_Console_Message *message);

#ifdef __cplusplus
}
#endif

#endif // ewk_error_internal_h
