/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2008 INdT Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "LocalizedStrings.h"

#include "NotImplemented.h"
#include <wtf/text/WTFString.h>

#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
#include <wtf/text/CString.h>
#include <libintl.h>
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)

namespace WebCore {

String submitButtonDefaultLabel()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_SUBMIT"));
#else
    return String::fromUTF8("Submit");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String inputElementAltText()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_SUBMIT"));
#else
    return String::fromUTF8("Submit");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String resetButtonDefaultLabel()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_RESET"));
#else
    return String::fromUTF8("Reset");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String defaultDetailsSummaryText()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_DETAILS"));
#else
    return String::fromUTF8("Details");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String searchableIndexIntroduction()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_YOU_CAN_SEARCH_THIS_INDEX_ENTER_KEYWORDS_C"));
#else
    return String::fromUTF8("This is a searchable index. Enter search keywords: ");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String fileButtonChooseFileLabel()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_UPLOAD_FILE"));
#else
    return String::fromUTF8("Choose File");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String fileButtonChooseMultipleFilesLabel()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_UPLOAD_MULTIPLE_FILES"));
#else
    return String::fromUTF8("Choose Files");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String fileButtonNoFileSelectedLabel()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_NO_FILES_HAVE_BEEN_SELECTED"));
#else
    return String::fromUTF8("No file selected");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String fileButtonNoFilesSelectedLabel()
{
    return String::fromUTF8("No files selected");
}

String contextMenuItemTagOpenLinkInNewWindow()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_OPEN_LINK_IN_NEW_TAB_ABB"));
#else
    return String::fromUTF8("Open Link in New Window");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagDownloadLinkToDisk()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_BR_BODY_SAVE_LINK"));
#else
    return String::fromUTF8("Download Linked File");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagCopyLinkToClipboard()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY_LINK_URL_ABB"));
#else
    return String::fromUTF8("Copy Link Location");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagOpenImageInNewWindow()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_OPEN_IMAGE_IN_NEW_TAB_ABB"));
#else
    return String::fromUTF8("Open Image in New Window");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagDownloadImageToDisk()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_SAVE_IMAGE_ABB"));
#else
    return String::fromUTF8("Save Image As");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagCopyImageToClipboard()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY_TO_CLIPBOARD"));
#else
    return String::fromUTF8("Copy Image");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagCopyImageUrlToClipboard()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_COPIED_TO_CLIPBOARD"));
#else
    return String::fromUTF8("Copy Image Address");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagOpenVideoInNewWindow()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_OPEN_VIDEO_IN_NEW_WINDOW_ABB"));
#else
    return String::fromUTF8("Open Video in New Window");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagOpenAudioInNewWindow()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_PLAY_AUDIO_IN_NEW_TAB_ABB"));
#else
    return String::fromUTF8("Open Audio in New Window");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagDownloadVideoToDisk()
{
    return String::fromUTF8("Download Video");
}

String contextMenuItemTagDownloadAudioToDisk()
{
    return String::fromUTF8("Download Audio");
}

String contextMenuItemTagCopyVideoLinkToClipboard()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY_VIDEO_URL_ABB"));
#else
    return String::fromUTF8("Copy Video Link Location");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagCopyAudioLinkToClipboard()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY_AUDIO_URL_ABB"));
#else
    return String::fromUTF8("Copy Audio Link Location");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagToggleMediaControls()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_SHOW_HIDE_MEDIA_CONTROLS_ABB"));
#else
    return String::fromUTF8("Toggle Media Controls");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagShowMediaControls()
{
    return String::fromUTF8("Show Media Controls");
}

String contextMenuitemTagHideMediaControls()
{
    return String::fromUTF8("Hide Media Controls");
}

String contextMenuItemTagToggleMediaLoop()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_ENABLE_DISABLE_REPEAT_MEDIA_ABB"));
#else
    return String::fromUTF8("Toggle Media Loop Playback");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagEnterVideoFullscreen()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_FULL_VIEW_ABB"));
#else
    return String::fromUTF8("Switch Video to Fullscreen");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagMediaPlay()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_PLAY"));
#else
    return String::fromUTF8("Play");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagMediaPause()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_PAUSE"));
#else
    return String::fromUTF8("Pause");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagMediaMute()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_MUTE"));
#else
    return String::fromUTF8("Mute");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagMediaUnMute()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_UNMUTE"));
#else
    return String::fromUTF8("Unmute");
#endif
}

String contextMenuItemTagOpenFrameInNewWindow()
{
    return String::fromUTF8("Open Frame in New Window");
}

String contextMenuItemTagCopy()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY"));
#else
    return String::fromUTF8("Copy");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagDelete()
{
    return String::fromUTF8("Delete");
}

String contextMenuItemTagSelectAll()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_SELECT_ALL_ABB"));
#else
    return String::fromUTF8("Select All");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

#if ENABLE(TIZEN_CONTEXT_MENU_SELECT)
String contextMenuItemTagSelectWord()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_SELECT_ABB"));
#else
    return String::fromUTF8("Select Word");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}
#endif // ENABLE(TIZEN_CONTEXT_MENU_SELECT)

String contextMenuItemTagUnicode()
{
    return String::fromUTF8("Insert Unicode Control Character");
}

String contextMenuItemTagInputMethods()
{
    return String::fromUTF8("Input Methods");
}

String contextMenuItemTagGoBack()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_BR_OPT_NAVIGATE_GO_BACKWARD"));
#else
    return String::fromUTF8("Go Back");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagGoForward()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_BR_OPT_NAVIGATE_GO_FORWARD"));
#else
    return String::fromUTF8("Go Forward");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagStop()
{
    return String::fromUTF8("Stop");
}

String contextMenuItemTagReload()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_BR_OPT_RELOAD"));
#else
    return String::fromUTF8("Reload");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagCut()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_CUT_ABB"));
#else
    return String::fromUTF8("Cut");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagPaste()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_PASTE"));
#else
    return String::fromUTF8("Paste");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagNoGuessesFound()
{
    return String::fromUTF8("No Guesses Found");
}

String contextMenuItemTagIgnoreSpelling()
{
    return String::fromUTF8("Ignore Spelling");
}

String contextMenuItemTagLearnSpelling()
{
    return String::fromUTF8("Learn Spelling");
}

String contextMenuItemTagSearchWeb()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_SEARCH_THE_INTERNET_ABB"));
#else
    return String::fromUTF8("Search the Web");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String contextMenuItemTagLookUpInDictionary(const String&)
{
    return String::fromUTF8("Look Up in Dictionary");
}

String contextMenuItemTagOpenLink()
{
    return String::fromUTF8("Open Link");
}

String contextMenuItemTagIgnoreGrammar()
{
    return String::fromUTF8("Ignore Grammar");
}

String contextMenuItemTagSpellingMenu()
{
    return String::fromUTF8("Spelling and Grammar");
}

String contextMenuItemTagShowSpellingPanel(bool show)
{
    return String::fromUTF8(show ? "Show Spelling and Grammar" : "Hide Spelling and Grammar");
}

String contextMenuItemTagCheckSpelling()
{
    return String::fromUTF8("Check Document Now");
}

String contextMenuItemTagCheckSpellingWhileTyping()
{
    return String::fromUTF8("Check Spelling While Typing");
}

String contextMenuItemTagCheckGrammarWithSpelling()
{
    return String::fromUTF8("Check Grammar With Spelling");
}

String contextMenuItemTagFontMenu()
{
    return String::fromUTF8("Font");
}

String contextMenuItemTagBold()
{
    return String::fromUTF8("Bold");
}

String contextMenuItemTagItalic()
{
    return String::fromUTF8("Italic");
}

String contextMenuItemTagUnderline()
{
    return String::fromUTF8("Underline");
}

String contextMenuItemTagOutline()
{
    return String::fromUTF8("Outline");
}

String contextMenuItemTagInspectElement()
{
    return String::fromUTF8("Inspect Element");
}

String contextMenuItemTagRightToLeft()
{
    return String::fromUTF8("Right to Left");
}

String contextMenuItemTagLeftToRight()
{
    return String::fromUTF8("Left to Right");
}

String contextMenuItemTagWritingDirectionMenu()
{
    return String::fromUTF8("Writing Direction");
}

String contextMenuItemTagTextDirectionMenu()
{
    return String::fromUTF8("Text Direction");
}

String contextMenuItemTagDefaultDirection()
{
    return String::fromUTF8("Default");
}

String searchMenuNoRecentSearchesText()
{
    return String::fromUTF8("No recent searches");
}

String searchMenuRecentSearchesText()
{
    return String::fromUTF8("Recent searches");
}

String searchMenuClearRecentSearchesText()
{
    return String::fromUTF8("Clear recent searches");
}

String AXDefinitionText()
{
    return String::fromUTF8("definition");
}

String AXDescriptionListText()
{
    return String::fromUTF8("description list");
}

String AXDescriptionListTermText()
{
    return String::fromUTF8("term");
}

String AXDescriptionListDetailText()
{
    return String::fromUTF8("description");
}

String AXFooterRoleDescriptionText()
{
    return String::fromUTF8("footer");
}

String AXButtonActionVerb()
{
    return String::fromUTF8("press");
}

String AXRadioButtonActionVerb()
{
    return String::fromUTF8("select");
}

String AXTextFieldActionVerb()
{
    return String::fromUTF8("activate");
}

String AXCheckedCheckBoxActionVerb()
{
    return String::fromUTF8("uncheck");
}

String AXUncheckedCheckBoxActionVerb()
{
    return String::fromUTF8("check");
}

String AXLinkActionVerb()
{
    return String::fromUTF8("jump");
}

String unknownFileSizeText()
{
    return String::fromUTF8("Unknown");
}

#if ENABLE(INPUT_TYPE_WEEK) && ENABLE(TIZEN_TV_PROFILE)
String weekFormatInLDML()
{
    return String::fromUTF8("ww yyyy");
}
#endif

String imageTitle(const String&, const IntSize&)
{
    notImplemented();
    return String();
}

String AXListItemActionVerb()
{
    notImplemented();
    return String();
}

#if ENABLE(VIDEO)
String localizedMediaControlElementString(const String&)
{
    notImplemented();
    return String();
}

String localizedMediaControlElementHelpText(const String&)
{
    notImplemented();
    return String();
}

String localizedMediaTimeDescription(float)
{
    notImplemented();
    return String();
}
#endif

String mediaElementLoadingStateText()
{
    return String::fromUTF8("Loading...");
}

String mediaElementLiveBroadcastStateText()
{
    return String::fromUTF8("Live Broadcast");
}

String validationMessagePatternMismatchText()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_INVALID_FORMAT_ENTERED"));
#else
    return String::fromUTF8("pattern mismatch");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

#pragma GCC diagnostic ignored "-Wformat-extra-args"
String validationMessageRangeOverflowText(const String& maximum)
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    char message[256] = {0x00, };
    snprintf(message, 256, dgettext("WebKit", "IDS_WEBVIEW_BODY_VALUE_MUST_BE_NO_HIGHER_THAN_PD"), maximum.utf8().data());
    return String::fromUTF8(message);
#else
    return ASCIILiteral("Value must be less than or equal to ") + maximum;
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String validationMessageRangeUnderflowText(const String& minimum)
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    char message[256] = {0x00, };
    snprintf(message, 256, dgettext("WebKit", "IDS_WEBVIEW_BODY_VALUE_MUST_BE_AT_LEAST_PD"), minimum.utf8().data());
    return String::fromUTF8(message);
#else
    return ASCIILiteral("Value must be greater than or equal to ") + minimum;
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}
#pragma GCC diagnostic warning "-Wformat-extra-args"

String validationMessageStepMismatchText(const String&, const String&)
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_INVALID_VALUE_ENTERED"));
#else
    return String::fromUTF8("step mismatch");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String validationMessageTooLongText(int, int)
{
    return String::fromUTF8("too long");
}

String validationMessageTypeMismatchText()
{
    return String::fromUTF8("type mismatch");
}

String validationMessageTypeMismatchForEmailText()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_ENTER_AN_EMAIL_ADDRESS"));
#else
    return ASCIILiteral("Please enter an email address");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String validationMessageTypeMismatchForMultipleEmailText()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_ENTER_A_LIST_OF_EMAIL_ADDRESSES_SEPARATED_BY_COMMAS"));
#else
    return ASCIILiteral("Please enter an email address");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String validationMessageTypeMismatchForURLText()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_ENTER_A_URL"));
#else
    return ASCIILiteral("Please enter a URL");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String validationMessageValueMissingText()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_FIELD_CANNOT_BE_BLANK"));
#else
    return String::fromUTF8("value missing");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String validationMessageValueMissingForCheckboxText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForFileText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForMultipleFileText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForRadioText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForSelectText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageBadInputForNumberText()
{
    notImplemented();
    return validationMessageTypeMismatchText();
}

String missingPluginText()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_PLUG_IN_MISSING"));
#else
    return String::fromUTF8("missing plugin");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}

String AXMenuListPopupActionVerb()
{
    return String();
}

String AXMenuListActionVerb()
{
    return String();
}

String multipleFileUploadText(unsigned numberOfFiles)
{
    return String::number(numberOfFiles) + String::fromUTF8(" files");
}

String crashedPluginText()
{
    return String::fromUTF8("plugin crashed");
}

String blockedPluginByContentSecurityPolicyText()
{
    notImplemented();
    return String();
}

String insecurePluginVersionText()
{
    notImplemented();
    return String();
}

String inactivePluginText()
{
    notImplemented();
    return String();
}

String unacceptableTLSCertificate()
{
    return String::fromUTF8("Unacceptable TLS certificate");
}

String localizedString(const char* key)
{
    return String::fromUTF8(key, strlen(key));
}

#if ENABLE(VIDEO_TRACK)
String textTrackClosedCaptionsText()
{
    return String::fromUTF8("Closed Captions");
}

String textTrackSubtitlesText()
{
    return String::fromUTF8("Subtitles");
}

String textTrackOffText()
{
    return String::fromUTF8("Off");
}

String textTrackNoLabelText()
{
    return String::fromUTF8("No label");
}
#endif

String snapshottedPlugInLabelTitle()
{
    return String("Snapshotted Plug-In");
}

String snapshottedPlugInLabelSubtitle()
{
    return String("Click to restart");
}

#if ENABLE(TIZEN_DRAG_SUPPORT)
String contextMenuItemTagDrag()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    static String stockLabel = String::fromUTF8(dgettext("WebKit","IDS_BR_OPT_DRAG_AND_DROP"));
    return stockLabel;
#else
    return String::fromUTF8("Drag and Drop");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
}
#endif // ENABLE(TIZEN_DRAG_SUPPORT)

}
