/*
   Copyright (C) 2011 Samsung Electronics

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
#include "ewk_text_style.h"

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
//FIXME: I am not sure whether we should support it. Just disabled until we have requirement about it.
#include "EditorState.h"
#include <WebCore/IntPoint.h>
#include <wtf/text/CString.h>

struct _Ewk_Text_Style {
    Ewk_Text_Style_State underlineState;
    Ewk_Text_Style_State italicState;
    Ewk_Text_Style_State boldState;

    struct {
        Evas_Point startPoint;
        Evas_Point endPoint;
    } position;

    struct {
        int r;
        int g;
        int b;
        int a;
    } bgColor;

    struct {
        int r;
        int g;
        int b;
        int a;
    } color;

    const char* fontSize;
    Eina_Bool hasComposition;
};

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
Ewk_Text_Style* ewkTextStyleCreate(const WebKit::EditorState editorState, const WebCore::IntPoint& startPoint, const WebCore::IntPoint& endPoint)
{
    Ewk_Text_Style* textStyle = new Ewk_Text_Style;

    textStyle->underlineState = static_cast<Ewk_Text_Style_State>(editorState.underlineState);
    textStyle->italicState = static_cast<Ewk_Text_Style_State>(editorState.italicState);
    textStyle->boldState = static_cast<Ewk_Text_Style_State>(editorState.boldState);

    textStyle->position.startPoint.x = startPoint.x();
    textStyle->position.startPoint.y = startPoint.y();
    textStyle->position.endPoint.x = endPoint.x();
    textStyle->position.endPoint.y = endPoint.y();

    // for example - rgba(255, 255, 255, 255)
    // for example - rgb(255, 255, 255)
    if (equalIgnoringCase(editorState.bgColor.left(3), "rgb")) {
        size_t startPos = editorState.bgColor.find("(");
        size_t endPos = editorState.bgColor.find(")");

        String value = editorState.bgColor.substring(startPos + 1, endPos - startPos - 1);
        Vector<String> colorValues;
        value.split(",", colorValues);
        textStyle->bgColor.r = colorValues[0].toInt();
        textStyle->bgColor.g = colorValues[1].toInt();
        textStyle->bgColor.b = colorValues[2].toInt();
        if (equalIgnoringCase(editorState.bgColor.left(4), "rgba"))
            textStyle->bgColor.a = colorValues[3].toInt();
        else
            textStyle->bgColor.a = 255;
    }

    if (equalIgnoringCase(editorState.color.left(3), "rgb")) {
        size_t startPos = editorState.color.find("(");
        size_t endPos = editorState.color.find(")");

        String value = editorState.color.substring(startPos + 1, endPos - startPos - 1);
        Vector<String> colorValues;
        value.split(",", colorValues);
        textStyle->color.r = colorValues[0].toInt();
        textStyle->color.g = colorValues[1].toInt();
        textStyle->color.b = colorValues[2].toInt();
        if (equalIgnoringCase(editorState.color.left(4), "rgba"))
            textStyle->color.a = colorValues[3].toInt();
        else
            textStyle->color.a = 255;
    }

    textStyle->fontSize = eina_stringshare_add(editorState.fontSize.utf8().data());
    textStyle->hasComposition = editorState.hasComposition;

    return textStyle;
}

void ewkTextStyleDelete(Ewk_Text_Style* textStyle)
{
    EINA_SAFETY_ON_NULL_RETURN(textStyle);

    if (textStyle->fontSize)
        eina_stringshare_del(textStyle->fontSize);

    delete textStyle;
}
#endif

Ewk_Text_Style_State ewk_text_style_underline_get(Ewk_Text_Style* textStyle)
{
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(textStyle, EWK_TEXT_STYLE_STATE_FALSE);

    return textStyle->underlineState;
#else
    return EWK_TEXT_STYLE_STATE_FALSE;
#endif
}

Ewk_Text_Style_State ewk_text_style_italic_get(Ewk_Text_Style* textStyle)
{
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(textStyle, EWK_TEXT_STYLE_STATE_FALSE);

    return textStyle->italicState;
#else
    return EWK_TEXT_STYLE_STATE_FALSE;
#endif
}

Ewk_Text_Style_State ewk_text_style_bold_get(Ewk_Text_Style* textStyle)
{
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(textStyle, EWK_TEXT_STYLE_STATE_FALSE);

    return textStyle->boldState;
#else
    return EWK_TEXT_STYLE_STATE_FALSE;
#endif
}

Eina_Bool ewk_text_style_position_get(Ewk_Text_Style* textStyle, Evas_Point* startPoint, Evas_Point* endPoint)
{
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(textStyle, false);

    startPoint->x = textStyle->position.startPoint.x;
    startPoint->y = textStyle->position.startPoint.y;

    endPoint->x = textStyle->position.endPoint.x;
    endPoint->y = textStyle->position.endPoint.y;

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_text_style_bg_color_get(Ewk_Text_Style* textStyle, int* r, int* g, int* b, int* a)
{
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(textStyle, false);

    if (r)
        *r = textStyle->bgColor.r;
    if (g)
        *g = textStyle->bgColor.g;
    if (b)
        *b = textStyle->bgColor.b;
    if (a)
        *a = textStyle->bgColor.a;

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_text_style_color_get(Ewk_Text_Style* textStyle, int* r, int* g, int* b, int* a)
{
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(textStyle, false);

    if (r)
        *r = textStyle->color.r;
    if (g)
        *g = textStyle->color.g;
    if (b)
        *b = textStyle->color.b;
    if (a)
        *a = textStyle->color.a;

    return true;
#else
    return false;
#endif
}

const char* ewk_text_style_font_size_get(Ewk_Text_Style* textStyle)
{
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(textStyle, 0);
    return textStyle->fontSize;
#else
    return 0;
#endif
}

Eina_Bool ewk_text_style_has_composition_get(Ewk_Text_Style* textStyle)
{
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(textStyle, false);
    return textStyle->hasComposition;
#else
    return false;
#endif
}
#endif
