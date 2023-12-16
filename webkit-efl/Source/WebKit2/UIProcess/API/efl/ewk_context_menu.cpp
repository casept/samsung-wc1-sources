/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ewk_context_menu.h"

#include "APIObject.h"
#include "EwkView.h"
#include "WKArray.h"
#include "WKString.h"
#include "ewk_context_menu_item.h"
#include "ewk_context_menu_item_private.h"
#include "ewk_context_menu_private.h"

using namespace WebKit;

static WKContextMenuItemTag getWKTagFromEwkAction(Ewk_Context_Menu_Item_Action action);

EwkContextMenu::EwkContextMenu(EwkView* view, WKArrayRef items)
    : m_viewImpl(view)
    , m_contextMenuItems(0)
{
    size_t size = WKArrayGetSize(items);
    for (size_t i = 0; i < size; ++i)
        m_contextMenuItems = eina_list_append(m_contextMenuItems, Ewk_Context_Menu_Item::create(static_cast<WKContextMenuItemRef>(WKArrayGetItemAtIndex(items, i)), this).leakPtr());
}

EwkContextMenu::EwkContextMenu()
    : m_viewImpl(0)
    , m_contextMenuItems(0)
{
}

EwkContextMenu::EwkContextMenu(Eina_List* items)
    : m_viewImpl(0)
    , m_contextMenuItems(0)
{
    Eina_List* l;
    void* data;
    EINA_LIST_FOREACH(items, l, data) {
        if (EwkContextMenuItem* item = static_cast<EwkContextMenuItem*>(data)) {
            item->setParentMenu(this);
            m_contextMenuItems = eina_list_append(m_contextMenuItems, item);
        }
    }
}

EwkContextMenu::~EwkContextMenu()
{
    void* data;
    EINA_LIST_FREE(m_contextMenuItems, data)
        delete static_cast<Ewk_Context_Menu_Item*>(data);
}

void EwkContextMenu::hide()
{
    if (!m_viewImpl)
        return;

    m_viewImpl->hideContextMenu();
}

void EwkContextMenu::appendItem(EwkContextMenuItem* item)
{
    item->setParentMenu(this);

    if (item->type() == EWK_SUBMENU_TYPE)
        item->subMenu()->setEwkView(this->ewkView());

    m_contextMenuItems = eina_list_append(m_contextMenuItems, item);
}

void EwkContextMenu::removeItem(EwkContextMenuItem* item)
{
    m_contextMenuItems = eina_list_remove(m_contextMenuItems, item);
}

bool EwkContextMenu::contextMenuItemSelected(WKContextMenuItemRef item)
{
    if (!m_viewImpl)
        return false;

    WKPageSelectContextMenuItem(m_viewImpl->wkPage(), item);

    return true;
}

Ewk_Context_Menu* ewk_context_menu_new()
{
    return EwkContextMenu::create().leakRef();
}

Ewk_Context_Menu* ewk_context_menu_new_with_items(Eina_List* items)
{
    return EwkContextMenu::create(items).leakRef();
}

Eina_Bool ewk_context_menu_item_append(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item_Tag tag, const char* title, const char* /* iconFile */, Eina_Bool enabled)
{
    // FIXME: Parameters of context_menu_item_append() are changed to sync with mobile profile.
    // iconFile is used to define icon type's option to context menu on mobile profile.
    // But it isn't supported on wearable profile yet.
    return ewk_context_menu_item_append_as_action(menu, tag, title, enabled);
}

Eina_Bool ewk_context_menu_item_append_as_action(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item_Tag tag, const char* title, Eina_Bool enabled)
{
    if (!menu)
        return false;

    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContextMenu, menu, impl, false);

    WKRetainPtr<WKStringRef> titleString(AdoptWK, WKStringCreateWithUTF8CString(title));
    WKContextMenuItemRef wkItem = WKContextMenuItemCreateAsAction(static_cast<WKContextMenuItemTag>(tag), titleString.get(), enabled);
    PassOwnPtr<EwkContextMenuItem> item = EwkContextMenuItem::create(wkItem, 0);

    impl->appendItem(item.get());

    return true;
}

Eina_Bool ewk_context_menu_item_append_with_object(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item* item)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContextMenu, menu, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    impl->appendItem(item);

    return true;
}

Eina_Bool ewk_context_menu_item_remove(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item* item)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContextMenu, menu, impl, false);

    impl->removeItem(item);

    return true;
}

Eina_Bool ewk_context_menu_hide(Ewk_Context_Menu* menu)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContextMenu, menu, impl, false);

    impl->hide();

    return true;
}

const Eina_List* ewk_context_menu_items_get(const Ewk_Context_Menu* menu)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkContextMenu, menu, impl, 0);

    return impl->items();
}

Eina_Bool ewk_context_menu_item_select(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item* item)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContextMenu, menu, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    WKContextMenuItemRef wkItem;

    switch (item->type()) {
    case EWK_ACTION_TYPE:
        wkItem = WKContextMenuItemCreateAsAction(getWKTagFromEwkAction(item->action()), adoptWK(WKStringCreateWithUTF8CString(item->title())).get(), item->enabled());
        break;
    case EWK_CHECKABLE_ACTION_TYPE:
        wkItem = WKContextMenuItemCreateAsCheckableAction(getWKTagFromEwkAction(item->action()), adoptWK(WKStringCreateWithUTF8CString(item->title())).get(), item->enabled(), item->checked());
        break;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }

    return impl->contextMenuItemSelected(wkItem);
}

static WKContextMenuItemTag getWKTagFromEwkAction(Ewk_Context_Menu_Item_Action action)
{
    switch (action) {
    case EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION:
        return kWKContextMenuItemTagNoAction;
    case EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW:
        return kWKContextMenuItemTagOpenLinkInNewWindow;
    case EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_LINK_TO_DISK:
        return kWKContextMenuItemTagDownloadLinkToDisk;
    case EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD:
        return kWKContextMenuItemTagCopyLinkToClipboard;
    case EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_NEW_WINDOW:
        return kWKContextMenuItemTagOpenImageInNewWindow;
    case EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_IMAGE_TO_DISK:
        return kWKContextMenuItemTagDownloadImageToDisk;
    case EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_TO_CLIPBOARD:
        return kWKContextMenuItemTagCopyImageToClipboard;
    case EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_URL_TO_CLIPBOARD:
        return kWKContextMenuItemTagCopyImageUrlToClipboard;
    case EWK_CONTEXT_MENU_ITEM_TAG_OPEN_FRAME_IN_NEW_WINDOW:
        return kWKContextMenuItemTagOpenFrameInNewWindow;
    case EWK_CONTEXT_MENU_ITEM_TAG_COPY:
        return kWKContextMenuItemTagCopy;
    case EWK_CONTEXT_MENU_ITEM_TAG_GO_BACK:
        return kWKContextMenuItemTagGoBack;
    case EWK_CONTEXT_MENU_ITEM_TAG_GO_FORWARD:
        return kWKContextMenuItemTagGoForward;
    case EWK_CONTEXT_MENU_ITEM_TAG_STOP:
        return kWKContextMenuItemTagStop;
    case EWK_CONTEXT_MENU_ITEM_TAG_RELOAD:
        return kWKContextMenuItemTagReload;
    case EWK_CONTEXT_MENU_ITEM_TAG_CUT:
        return kWKContextMenuItemTagCut;
    case EWK_CONTEXT_MENU_ITEM_TAG_PASTE:
        return kWKContextMenuItemTagPaste;
    case EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL:
        return kWKContextMenuItemTagSelectAll;
#if ENABLE(TIZEN_CONTEXT_MENU_SELECT)
    case EWK_CONTEXT_MENU_ITEM_TAG_SELECT_WORD:
        return kWKContextMenuItemTagSelectWord;
#endif // ENABLE(TIZEN_CONTEXT_MENU_SELECT)
    case EWK_CONTEXT_MENU_ITEM_TAG_SPELLING_GUESS:
        return kWKContextMenuItemTagSpellingGuess;
    case EWK_CONTEXT_MENU_ITEM_TAG_NO_GUESSES_FOUND:
        return kWKContextMenuItemTagNoGuessesFound;
    case EWK_CONTEXT_MENU_ITEM_TAG_IGNORE_SPELLING:
        return kWKContextMenuItemTagIgnoreSpelling;
    case EWK_CONTEXT_MENU_ITEM_TAG_LEARN_SPELLING:
        return kWKContextMenuItemTagLearnSpelling;
    case EWK_CONTEXT_MENU_ITEM_TAG_OTHER:
        return kWKContextMenuItemTagOther;
    case EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_IN_SPOTLIGHT:
        return kWKContextMenuItemTagSearchInSpotlight;
    case EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_WEB:
        return kWKContextMenuItemTagSearchWeb;
    case EWK_CONTEXT_MENU_ITEM_TAG_LOOK_UP_IN_DICTIONARY:
        return kWKContextMenuItemTagLookUpInDictionary;
    case EWK_CONTEXT_MENU_ITEM_TAG_OPEN_WITH_DEFAULT_APPLICATION:
        return kWKContextMenuItemTagOpenWithDefaultApplication;
    case EWK_CONTEXT_MENU_ITEM_PDFACTUAL_SIZE:
        return kWKContextMenuItemTagPDFActualSize;
    case EWK_CONTEXT_MENU_ITEM_PDFZOOM_IN:
        return kWKContextMenuItemTagPDFZoomIn;
    case EWK_CONTEXT_MENU_ITEM_PDFZOOM_OUT:
        return kWKContextMenuItemTagPDFZoomOut;
    case EWK_CONTEXT_MENU_ITEM_PDFAUTO_SIZE:
        return kWKContextMenuItemTagPDFAutoSize;
    case EWK_CONTEXT_MENU_ITEM_PDFSINGLE_PAGE:
        return kWKContextMenuItemTagPDFSinglePage;
    case EWK_CONTEXT_MENU_ITEM_PDFFACING_PAGES:
        return kWKContextMenuItemTagPDFFacingPages;
    case EWK_CONTEXT_MENU_ITEM_PDFCONTINUOUS:
        return kWKContextMenuItemTagPDFContinuous;
    case EWK_CONTEXT_MENU_ITEM_PDFNEXT_PAGE:
        return kWKContextMenuItemTagPDFNextPage;
    case EWK_CONTEXT_MENU_ITEM_PDFPREVIOUS_PAGE:
        return kWKContextMenuItemTagPDFPreviousPage;
    case EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK:
        return kWKContextMenuItemTagOpenLink;
    case EWK_CONTEXT_MENU_ITEM_TAG_IGNORE_GRAMMAR:
        return kWKContextMenuItemTagIgnoreGrammar;
    case EWK_CONTEXT_MENU_ITEM_TAG_SPELLING_MENU:
        return kWKContextMenuItemTagSpellingMenu;
    case EWK_CONTEXT_MENU_ITEM_TAG_SHOW_SPELLING_PANEL:
        return kWKContextMenuItemTagShowSpellingPanel;
    case EWK_CONTEXT_MENU_ITEM_TAG_CHECK_SPELLING:
        return kWKContextMenuItemTagCheckSpelling;
    case EWK_CONTEXT_MENU_ITEM_TAG_CHECK_SPELLING_WHILE_TYPING:
        return kWKContextMenuItemTagCheckSpellingWhileTyping;
    case EWK_CONTEXT_MENU_ITEM_TAG_CHECK_GRAMMAR_WITH_SPELLING:
        return kWKContextMenuItemTagCheckGrammarWithSpelling;
    case EWK_CONTEXT_MENU_ITEM_TAG_FONT_MENU:
        return kWKContextMenuItemTagFontMenu;
    case EWK_CONTEXT_MENU_ITEM_TAG_SHOW_FONTS:
        return kWKContextMenuItemTagShowFonts;
    case EWK_CONTEXT_MENU_ITEM_TAG_BOLD:
        return kWKContextMenuItemTagBold;
    case EWK_CONTEXT_MENU_ITEM_TAG_ITALIC:
        return kWKContextMenuItemTagItalic;
    case EWK_CONTEXT_MENU_ITEM_TAG_UNDERLINE:
        return kWKContextMenuItemTagUnderline;
    case EWK_CONTEXT_MENU_ITEM_TAG_OUTLINE:
        return kWKContextMenuItemTagOutline;
    case EWK_CONTEXT_MENU_ITEM_TAG_STYLES:
        return kWKContextMenuItemTagStyles;
    case EWK_CONTEXT_MENU_ITEM_TAG_SHOW_COLORS:
        return kWKContextMenuItemTagShowColors;
    case EWK_CONTEXT_MENU_ITEM_TAG_SPEECH_MENU:
        return kWKContextMenuItemTagSpeechMenu;
    case EWK_CONTEXT_MENU_ITEM_TAG_START_SPEAKING:
        return kWKContextMenuItemTagStartSpeaking;
    case EWK_CONTEXT_MENU_ITEM_TAG_STOP_SPEAKING:
        return kWKContextMenuItemTagStopSpeaking;
    case EWK_CONTEXT_MENU_ITEM_TAG_WRITING_DIRECTION_MENU:
        return kWKContextMenuItemTagWritingDirectionMenu;
    case EWK_CONTEXT_MENU_ITEM_TAG_DEFAULT_DIRECTION:
        return kWKContextMenuItemTagDefaultDirection;
    case EWK_CONTEXT_MENU_ITEM_TAG_LEFT_TO_RIGHT:
        return kWKContextMenuItemTagLeftToRight;
    case EWK_CONTEXT_MENU_ITEM_TAG_RIGHT_TO_LEFT:
        return kWKContextMenuItemTagRightToLeft;
    case EWK_CONTEXT_MENU_ITEM_TAG_PDFSINGLE_PAGE_SCROLLING:
        return kWKContextMenuItemTagPDFSinglePageScrolling;
    case EWK_CONTEXT_MENU_ITEM_TAG_PDFFACING_PAGES_SCROLLING:
        return kWKContextMenuItemTagPDFFacingPagesScrolling;
    case EWK_CONTEXT_MENU_ITEM_TAG_INSPECT_ELEMENT:
        return kWKContextMenuItemTagInspectElement;
    case EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_MENU:
        return kWKContextMenuItemTagTextDirectionMenu;
    case EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_DEFAULT:
        return kWKContextMenuItemTagTextDirectionDefault;
    case EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_LEFT_TO_RIGHT:
        return kWKContextMenuItemTagTextDirectionLeftToRight;
    case EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_RIGHT_TO_LEFT:
        return kWKContextMenuItemTagTextDirectionRightToLeft;
    case EWK_CONTEXT_MENU_ITEM_OPEN_MEDIA_IN_NEW_WINDOW:
        return kWKContextMenuItemTagOpenMediaInNewWindow;
    case EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_MEDIA_TO_DISK:
        return kWKContextMenuItemTagDownloadMediaToDisk;
    case EWK_CONTEXT_MENU_ITEM_TAG_COPY_MEDIA_LINK_TO_CLIPBOARD:
        return kWKContextMenuItemTagCopyMediaLinkToClipboard;
    case EWK_CONTEXT_MENU_ITEM_TAG_TOGGLE_MEDIA_CONTROLS:
        return kWKContextMenuItemTagToggleMediaControls;
    case EWK_CONTEXT_MENU_ITEM_TAG_TOGGLE_MEDIA_LOOP:
        return kWKContextMenuItemTagToggleMediaLoop;
    case EWK_CONTEXT_MENU_ITEM_TAG_ENTER_VIDEO_FULLSCREEN:
        return kWKContextMenuItemTagEnterVideoFullscreen;
    case EWK_CONTEXT_MENU_ITEM_TAG_MEDIA_PLAY_PAUSE:
        return kWKContextMenuItemTagMediaPlayPause;
    case EWK_CONTEXT_MENU_ITEM_TAG_MEDIA_MUTE:
        return kWKContextMenuItemTagMediaMute;
    case EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG:
        return kWKContextMenuItemBaseApplicationTag;
    default:
        return static_cast<WKContextMenuItemTag>(action);
    }
}

unsigned ewk_context_menu_item_count(Ewk_Context_Menu* menu)
{
#if ENABLE(TIZEN_CONTEXT_MENU)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContextMenu, menu, impl, false);

    return eina_list_count(impl->items());
#else
    return 0;
#endif
}

Ewk_Context_Menu_Item* ewk_context_menu_nth_item_get(Ewk_Context_Menu* menu, unsigned int n)
{
#if ENABLE(TIZEN_CONTEXT_MENU)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContextMenu, menu, impl, false);

    return static_cast<Ewk_Context_Menu_Item*>(eina_list_nth(impl->items(), n));
#else
    return 0;
#endif
}

Ewk_Context_Menu_Item_Tag ewk_context_menu_item_tag_get(Ewk_Context_Menu_Item* item)
{
#if ENABLE(TIZEN_CONTEXT_MENU)
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION);

    return item->type();
#else
    return 0;
#endif
}

const char* ewk_context_menu_item_link_url_get(Ewk_Context_Menu_Item* /* item */)
{
    // FIXME: Need to implement
    return 0;
}

const char* ewk_context_menu_item_image_url_get(Ewk_Context_Menu_Item* /* item */)
{
    // FIXME: Need to implement
    return 0;
}