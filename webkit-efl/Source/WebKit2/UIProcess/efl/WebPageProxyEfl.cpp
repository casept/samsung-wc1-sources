/*
 * Copyright (C) 2011 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebPageProxy.h"

#include "EwkView.h"
#include "NativeWebKeyboardEvent.h"
#include "NotImplemented.h"
#if ENABLE(TIZEN_SCREEN_CAPTURE)
#include "WebImage.h"
#endif
#include "WebKitVersion.h"
#include "WebPageMessages.h"
#include "WebProcessProxy.h"
#include "WebView.h"

#include <sys/utsname.h>

#if ENABLE(TIZEN_CSP)
#include <WebCore/ContentSecurityPolicy.h>
#endif // ENABLE(TIZEN_CSP)

#if ENABLE(TIZEN_TEXT_SELECTION)
#include <WebCore/IntPoint.h>
#include <WebCore/VisibleSelection.h>
#endif

#if ENABLE(TIZEN_CONTEXT_MENU)
#include "WebContextMenuProxy.h"
#endif

#if ENABLE(TIZEN_USER_AGENT)
#include "WebKitVersion.h"
#include <system_info.h>
#include <system_info_internal.h>
#endif

#if ENABLE(TIZEN_PAGE_SUSPEND_RESUME)
#include "DrawingAreaMessages.h"
#endif

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
#include <Elementary.h>
#endif

namespace WebKit {

String WebPageProxy::standardUserAgent(const String& applicationNameForUserAgent)
{
    WTF::String platform;
    WTF::String version;
    WTF::String osVersion;

#if ENABLE(TIZEN_USER_AGENT)
    char* tizenVersion = 0;
    system_info_get_value_string(SYSTEM_INFO_KEY_TIZEN_VERSION, &tizenVersion);
    osVersion = tizenVersion;
    free(tizenVersion);

    version = String::number(WEBKIT_MAJOR_VERSION) + '.' + String::number(WEBKIT_MINOR_VERSION);
#if ENABLE(TIZEN_EMULATOR)
    WTF::String userAgent = "Mozilla/5.0 (Linux; Tizen " + osVersion + "; sdk) AppleWebKit/" + version + " (KHTML, like Gecko)";
#else
    char* tizenManufacturer;
    char* tizenModelName;

    system_info_get_value_string(SYSTEM_INFO_KEY_MANUFACTURER, &tizenManufacturer);
    WTF::String manufacturer(tizenManufacturer);
    manufacturer.makeUpper();
    free(tizenManufacturer);

    system_info_get_value_string(SYSTEM_INFO_KEY_MODEL, &tizenModelName);

    WTF::String userAgent = "Mozilla/5.0 (Linux; Tizen " + osVersion;
    if (!manufacturer.isEmpty() && tizenModelName) {
        userAgent.append("; ");
        userAgent.append(manufacturer);
        userAgent.append(" ");
        userAgent.append(tizenModelName);
    }
    else if (!manufacturer.isEmpty()) {
        userAgent.append("; ");
        userAgent.append(manufacturer);
    }
    else if (tizenModelName) {
        userAgent.append("; ");
        userAgent.append(tizenModelName);
    }
    free(tizenModelName);

    userAgent.append(") AppleWebKit/");
    userAgent.append(version);
    userAgent.append(" (KHTML, like Gecko)");
#endif

    if (applicationNameForUserAgent.isEmpty()) {
        userAgent.append(" Version/");
        userAgent.append(osVersion);
    } else
        userAgent.append(applicationNameForUserAgent);

    userAgent.append(" Mobile Safari/");
    userAgent.append(version);

    return userAgent;
#else // #if ENABLE(TIZEN_USER_AGENT)
#if PLATFORM(X11)
    platform = "X11";
#else
    platform = "Unknown";
#endif
    version = String::number(WEBKIT_MAJOR_VERSION) + '.' + String::number(WEBKIT_MINOR_VERSION) + '+';
    struct utsname name;
    if (uname(&name) != -1)
        osVersion = WTF::String(name.sysname) + " " + WTF::String(name.machine);
    else
        osVersion = "Unknown";

    return "Mozilla/5.0 (" + platform + "; " + osVersion + ") AppleWebKit/" + version
        + " (KHTML, like Gecko) Version/5.0 Safari/" + version;
#endif // #if ENABLE(TIZEN_USER_AGENT)
}

void WebPageProxy::getEditorCommandsForKeyEvent(Vector<WTF::String>& /*commandsList*/)
{
    notImplemented();
}

void WebPageProxy::saveRecentSearches(const String&, const Vector<String>&)
{
    notImplemented();
}

void WebPageProxy::loadRecentSearches(const String&, Vector<String>&)
{
    notImplemented();
}

void WebPageProxy::setThemePath(const String& themePath)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SetThemePath(themePath), m_pageID, 0);
}

void WebPageProxy::createPluginContainer(uint64_t&)
{
    notImplemented();
}

void WebPageProxy::windowedPluginGeometryDidChange(const WebCore::IntRect&, const WebCore::IntRect&, uint64_t)
{
    notImplemented();
}

void WebPageProxy::handleInputMethodKeydown(bool& handled)
{
    handled = m_keyEventQueue.first().isFiltered();
}

void WebPageProxy::confirmComposition(const String& compositionString)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::ConfirmComposition(compositionString), m_pageID, 0);
}

void WebPageProxy::setComposition(const String& compositionString, Vector<WebCore::CompositionUnderline>& underlines, int cursorPosition)
{
    if (!isValid())
        return;

#if ENABLE(TIZEN_REDUCE_KEY_LAGGING)
    // Suspend layout&paint at the key input, and resume layout&paint after 150 ms.
    suspendActiveDOMObjectsAndAnimations();
    if (m_pageContentResumeTimer)
        ecore_timer_del(m_pageContentResumeTimer);
    m_pageContentResumeTimer = ecore_timer_add(150.0/1000.0, pageContentResumeTimerFired, this);
#endif // ENABLE(TIZEN_REDUCE_KEY_LAGGING)

    process()->send(Messages::WebPage::SetComposition(compositionString, underlines, cursorPosition), m_pageID, 0);
}

void WebPageProxy::cancelComposition()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::CancelComposition(), m_pageID, 0);
}

void WebPageProxy::initializeUIPopupMenuClient(const WKPageUIPopupMenuClient* client)
{
    m_uiPopupMenuClient.initialize(client);
}

#if PLATFORM(TIZEN)
void WebPageProxy::initializeTizenClient(const WKPageTizenClient* client)
{
    m_tizenClient.initialize(client);
}
#endif // PLATFORM(TIZEN)

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
void WebPageProxy::lockOrientation(int willLockOrientation, bool& result)
{
    result = m_pageClient->lockOrientation(willLockOrientation);
}

void WebPageProxy::unlockOrientation()
{
    m_pageClient->unlockOrientation();
}
#endif

#if ENABLE(TIZEN_TEXT_CARET_HANDLING_WK2)
void WebPageProxy::setCaretPosition(const WebCore::IntPoint& pos)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SetCaretPosition(pos), m_pageID);
}

void WebPageProxy::getCaretPosition(WebCore::IntRect& rect)
{
    if (!isValid())
        return;

    process()->sendSync(Messages::WebPage::GetCaretPosition(), Messages::WebPage::GetCaretPosition::Reply(rect), m_pageID);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#if ENABLE(TOUCH_ADJUSTMENT)
WebHitTestResult::Data WebPageProxy::hitTestResultAtPoint(const WebCore::IntPoint& point, int hitTestMode, const IntSize& area)
#else
WebHitTestResult::Data WebPageProxy::hitTestResultAtPoint(const WebCore::IntPoint& point, int hitTestMode)
#endif
{
    WebHitTestResult::Data hitTestResultData;
    if (!isValid())
        return hitTestResultData;

#if ENABLE(TOUCH_ADJUSTMENT)
    process()->sendSync(Messages::WebPage::HitTestResultAtPoint(point, hitTestMode, area),
                        Messages::WebPage::HitTestResultAtPoint::Reply(hitTestResultData), m_pageID);
#else
    process()->sendSync(Messages::WebPage::HitTestResultAtPoint(point, hitTestMode),
                        Messages::WebPage::HitTestResultAtPoint::Reply(hitTestResultData), m_pageID);
#endif

    return hitTestResultData;
}
#endif // ENABLE(TIZEN_WEBKIT2_HIT_TEST)

#if ENABLE(TIZEN_CSP)
void WebPageProxy::setContentSecurityPolicy(const String& policy, WebCore::ContentSecurityPolicy::HeaderType type)
{
    process()->send(Messages::WebPage::SetContentSecurityPolicy(policy, type), m_pageID);
}
#endif // ENABLE(TIZEN_CSP)

#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
void WebPageProxy::createPagesToPDF(const WebCore::IntSize& surfaceSize, const String& fileName)
{
    process()->send(Messages::WebPage::CreatePagesToPDF(surfaceSize, fileName), m_pageID);
}
#endif // ENABLE(TIZEN_MOBILE_WEB_PRINT)

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
void WebPageProxy::getStandaloneStatus(bool& standalone)
{
    standalone = m_pageClient->getStandaloneStatus();
}

void WebPageProxy::getWebAppCapable(PassRefPtr<BooleanCallback> prpCallback)
{
    RefPtr<BooleanCallback> callback = prpCallback;
    if (!isValid()) {
        callback->invalidate();
        return;
    }

    uint64_t callbackID = callback->callbackID();
    m_booleanCallbacks.set(callbackID, callback.get());
    process()->send(Messages::WebPage::GetWebAppCapable(callbackID), m_pageID);
}

void WebPageProxy::didGetWebAppCapable(const bool capable, uint64_t callbackID)
{
    RefPtr<BooleanCallback> callback = m_booleanCallbacks.take(callbackID);
    m_booleanCallbacks.remove(callbackID);
    callback->performCallbackWithReturnValue(WebBoolean::create(capable).leakRef());
}

void WebPageProxy::getWebAppIconURL(PassRefPtr<StringCallback> prpCallback)
{
    RefPtr<StringCallback> callback = prpCallback;
    if (!isValid()) {
        callback->invalidate();
        return;
    }

    uint64_t callbackID = callback->callbackID();
    m_stringCallbacks.set(callbackID, callback.get());
    process()->send(Messages::WebPage::GetWebAppIconURL(callbackID), m_pageID);
}

void WebPageProxy::didGetWebAppIconURL(const String& iconURL, uint64_t callbackID)
{
    RefPtr<StringCallback> callback = m_stringCallbacks.take(callbackID);
    if (!callback) {
        // FIXME: Log error or assert.
        // this can validly happen if a load invalidated the callback, though
        return;
    }

    m_stringCallbacks.remove(callbackID);
    callback->performCallbackWithReturnValue(iconURL.impl());
}

void WebPageProxy::getWebAppIconURLs(PassRefPtr<DictionaryCallback> prpCallback)
{
    RefPtr<DictionaryCallback> callback = prpCallback;
    if (!isValid()) {
        callback->invalidate();
        return;
    }

    uint64_t callbackID = callback->callbackID();
    m_dictionaryCallbacks.set(callbackID, callback.get());
    process()->send(Messages::WebPage::GetWebAppIconURLs(callbackID), m_pageID);
}

//void WebPageProxy::didGetWebAppIconURLs(const StringPairVector& iconURLs, uint64_t callbackID)
void WebPageProxy::didGetWebAppIconURLs(const Vector<std::pair<String, String>>& iconURLs, uint64_t callbackID)
{
    RefPtr<DictionaryCallback> callback = m_dictionaryCallbacks.take(callbackID);
    ImmutableDictionary::MapType iconURLList;
    //size_t iconURLCount = iconURLs.stringPairVector().size();
    size_t iconURLCount = iconURLs.size();
    for (size_t i = 0; i < iconURLCount; ++i)
        //iconURLList.set(iconURLs.stringPairVector()[i].first, WebString::create(iconURLs.stringPairVector()[i].second));
        iconURLList.set(iconURLs[i].first, WebString::create(iconURLs[i].second));

    callback->performCallbackWithReturnValue(ImmutableDictionary::adopt(iconURLList).get());
}
#endif

#if ENABLE(TIZEN_SCRIPTED_SPEECH)
void WebPageProxy::requestPermissionToUseMicrophone(uint64_t requestID, const String& host, const String& protocol, uint32_t port)
{
    RefPtr<WebSpeechPermissionRequestProxy> request = m_webSpeechPermissionRequestManager.createRequest(requestID);
    
    if (!m_tizenClient.decidePolicyForWebSpeechPermissionRequest(this, request.get(), host, protocol, port))
        request->deny();
}
#endif

#if ENABLE(TIZEN_REDUCE_KEY_LAGGING)
Eina_Bool WebPageProxy::pageContentResumeTimerFired(void* data)
{
    static_cast<WebPageProxy*>(data)->resumeActiveDOMObjectsAndAnimations();
    static_cast<WebPageProxy*>(data)->m_pageContentResumeTimer = 0;
    return ECORE_CALLBACK_CANCEL;
}
#endif // ENABLE(TIZEN_REDUCE_KEY_LAGGING)

#if ENABLE(TIZEN_USE_SETTINGS_FONT)
void WebPageProxy::useSettingsFont()
{
    process()->send(Messages::WebPage::UseSettingsFont(), m_pageID, 0);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_NOTIFY_SUSPEND_BY_REMOTE_WEB_INSPECTOR)
void WebPageProxy::setContentSuspendedByInspector(bool isSuspended)
{
    m_contentSuspendedByInspector = isSuspended;
}
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
void WebPageProxy::saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName)
{
    m_pageClient->saveSerializedHTMLDataForMainPage(serializedData, fileName);
}

void WebPageProxy::saveSubresourcesData(Vector<WebSubresourceTizen> subresourceData)
{
    m_pageClient->saveSubresourcesData(subresourceData);
}

void WebPageProxy::startOfflinePageSave(String subresourceFolderName)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::StartOfflinePageSave(subresourceFolderName), m_pageID);
}
#endif

#if ENABLE(TIZEN_JAVASCRIPT_AND_RESOURCE_SUSPEND_RESUME)
void WebPageProxy::suspendJavaScriptAndResource()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SuspendJavaScriptAndResource(), m_pageID);
}

void WebPageProxy::resumeJavaScriptAndResource()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::ResumeJavaScriptAndResource(), m_pageID);
}
#endif

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
void WebPageProxy::suspendPlugin()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SuspendPlugin(), m_pageID);
}

void WebPageProxy::resumePlugin()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::ResumePlugin(), m_pageID);
}
#endif // ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
void WebPageProxy::setClipboardData(const String& data, const String& type)
{
    static_cast<WebViewEfl*>(m_pageClient)->setClipboardData(data, type);
}

void WebPageProxy::clearClipboardData()
{
    static_cast<WebViewEfl*>(m_pageClient)->clearClipboardData();
}
#endif // ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)

#if ENABLE(TIZEN_WEB_STORAGE)
void WebPageProxy::getWebStorageQuotaBytes(PassRefPtr<WebStorageQuotaCallback> prpCallback)
{
    RefPtr<WebStorageQuotaCallback> callback = prpCallback;
    if (!isValid()) {
        callback->invalidate();
        return;
    }

    uint64_t callbackID = callback->callbackID();
    m_quotaCallbacks.set(callbackID, callback.get());
    process()->send(Messages::WebPage::GetStorageQuotaBytes(callbackID), m_pageID);
}

void WebPageProxy::didGetWebStorageQuotaBytes(const uint32_t quota, uint64_t callbackID)
{
    RefPtr<WebStorageQuotaCallback> callback = m_quotaCallbacks.take(callbackID);
    if (!callback) {
        // FIXME: Log error or assert.
        // this can validly happen if a load invalidated the callback, though
        return;
    }

    m_quotaCallbacks.remove(callbackID);

    RefPtr<WebUInt32> uint32Object = WebUInt32::create(quota);
    callback->performCallbackWithReturnValue(uint32Object.release().leakRef());
}

void WebPageProxy::setWebStorageQuotaBytes(uint32_t quota)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SetStorageQuotaBytes(quota), m_pageID, 0);
}
#endif // ENABLE(TIZEN_WEB_STORAGE)

#if ENABLE(TIZEN_CONTEXT_MENU)
void WebPageProxy::hideContextMenu()
{
    if (m_activeContextMenu)
        m_activeContextMenu->hideContextMenu();
}

String WebPageProxy::contextMenuAbsoluteLinkURLString()
{
    if (!m_activeContextMenu)
        return String();

    return m_activeContextMenuHitTestResultData.absoluteLinkURL;
}

String WebPageProxy::contextMenuAbsoluteImageURLString()
{
    if (!m_activeContextMenu)
        return String();

    return m_activeContextMenuHitTestResultData.absoluteImageURL;
}
#endif // ENABLE(TIZEN_CONTEXT_MENU)

#if ENABLE(TIZEN_TEXT_SELECTION)
bool WebPageProxy::selectClosestWord(const WebCore::IntPoint& point)
{
    if (!isValid())
        return false;

    bool result = false;
    process()->sendSync(Messages::WebPage::SelectClosestWord(point), Messages::WebPage::SelectClosestWord::Reply(result), m_pageID);
    return result;
}

int WebPageProxy::setLeftSelection(const WebCore::IntPoint& point, const int direction)
{
    if (!isValid())
        return 0;

    int result = 0;
    process()->sendSync(Messages::WebPage::SetLeftSelection(point, direction), Messages::WebPage::SetLeftSelection::Reply(result), m_pageID);
    return result;
}

int WebPageProxy::setRightSelection(const WebCore::IntPoint& point, const int direction)
{
    if (!isValid())
        return 0;

    int result = 0;
    process()->sendSync(Messages::WebPage::SetRightSelection(point, direction), Messages::WebPage::SetRightSelection::Reply(result), m_pageID);
    return result;
}

bool WebPageProxy::getSelectionHandlers(WebCore::IntRect& leftRect, WebCore::IntRect& rightRect)
{
    if (!isValid())
        return false;

    bool result = false;
    process()->sendSync(Messages::WebPage::GetSelectionHandlers(), Messages::WebPage::GetSelectionHandlers::Reply(leftRect, rightRect), m_pageID);
    if (!leftRect.size().isZero() || !rightRect.size().isZero())
        result = true;

    return result;
}

String WebPageProxy::getSelectionText()
{
    String ret;
    if (!isValid())
        return ret;

    process()->sendSync(Messages::WebPage::GetSelectionText(), Messages::WebPage::GetSelectionText::Reply(ret), m_pageID);
    return ret;
}

bool WebPageProxy::selectionRangeClear()
{
    if (!isValid())
        return false;

    bool result = false;
    process()->sendSync(Messages::WebPage::SelectionRangeClear(), Messages::WebPage::SelectionRangeClear::Reply(result), m_pageID);
    return result;
}

bool WebPageProxy::scrollContentByCharacter(const WebCore::IntPoint& point, WebCore::SelectionDirection direction)
{
    if (!isValid())
        return false;

    bool result = false;
    process()->sendSync(Messages::WebPage::ScrollContentByCharacter(point, direction), Messages::WebPage::ScrollContentByCharacter::Reply(result), m_pageID);
    return result;
}

bool WebPageProxy::scrollContentByLine(const WebCore::IntPoint& point, WebCore::SelectionDirection direction)
{
    if (!isValid())
        return false;

    bool result = false;
    process()->sendSync(Messages::WebPage::ScrollContentByLine(point, direction), Messages::WebPage::ScrollContentByLine::Reply(result), m_pageID);
    return result;
}
#endif // ENABLE(TIZEN_TEXT_SELECTION)

#if ENABLE(TIZEN_SQL_DATABASE)
void WebPageProxy::replyExceededDatabaseQuota(bool allow)
{
    if (!m_exceededDatabaseQuotaReply) {
        // TIZEN_LOGE("m_exceededDatabaseQuotaReply does not exist");
        return;
    }

    m_exceededDatabaseQuotaReply->send(allow);
    m_exceededDatabaseQuotaReply = nullptr;
}
#endif // ENABLE(TIZEN_SQL_DATABASE)

#if ENABLE(TIZEN_INDEXED_DATABASE)
void WebPageProxy::exceededIndexedDatabaseQuota(uint64_t frameID, const String& originIdentifier, int64_t currentUsage, PassRefPtr<Messages::WebPageProxy::ExceededIndexedDatabaseQuota::DelayedReply> reply)
{
    WebFrameProxy* frame = process()->webFrame(frameID);

    // Since exceededIndexedDatabaseQuota() can spin a nested run loop we need to turn off the responsiveness timer.
    process()->responsivenessTimer()->stop();

    m_exceededIndexedDatabaseQuotaReply = reply;

    RefPtr<WebSecurityOrigin> origin = WebSecurityOrigin::createFromDatabaseIdentifier(originIdentifier);

    if (!m_tizenClient.exceededIndexedDatabaseQuota(this, origin.get(), currentUsage, frame))
        replyExceededIndexedDatabaseQuota(false);
}

void WebPageProxy::replyExceededIndexedDatabaseQuota(bool allow)
{
    if (!m_exceededIndexedDatabaseQuotaReply)
        return;

    m_exceededIndexedDatabaseQuotaReply->send(allow);
    m_exceededIndexedDatabaseQuotaReply = nullptr;
}
#endif // ENABLE(TIZEN_INDEXED_DATABASE)

#if PLATFORM(TIZEN)
void WebPageProxy::replyJavaScriptAlert()
{
    if (!m_alertReply)
        return;

    m_alertReply->send();
    m_alertReply = nullptr;
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
    m_uiClient.notifyPopupReplyWaitingState(this, false);
#endif // ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
}

void WebPageProxy::replyJavaScriptConfirm(bool result)
{
    if (!m_confirmReply)
        return;

    m_confirmReply->send(result);
    m_confirmReply = nullptr;
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
    m_uiClient.notifyPopupReplyWaitingState(this, false);
#endif // ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
}

void WebPageProxy::replyJavaScriptPrompt(const String& result)
{
    if (!m_promptReply)
        return;

    m_promptReply->send(result);
    m_promptReply = nullptr;
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
    m_uiClient.notifyPopupReplyWaitingState(this, false);
#endif // ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
}

bool WebPageProxy::makeContextCurrent()
{
    return m_pageClient->makeContextCurrent();
}
#endif // PLATFORM(TIZEN)

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
bool WebPageProxy::useGLBackend()
{
    return m_pageClient->useGLBackend();
}
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
void WebPageProxy::replyPolicyForCertificateError(bool result)
{
    if (!m_allowedReply)
        return;

    m_allowedReply->send(result);
    m_allowedReply = nullptr;
}
#endif

#if ENABLE(TIZEN_SCREEN_CAPTURE)
PassRefPtr<WebImage> WebPageProxy::createSnapshot(const WebCore::IntRect& rect, float scaleFactor)
{
    if (!isValid())
        return 0;

    ShareableBitmap::Handle snapshotHandle;
    // Do not wait for more than a second (arbitrary) for the WebProcess to get the snapshot so
    // that the UI Process is not permanently stuck waiting on a potentially crashing Web Process.
    static const double createSnapshotSyncMessageTimeout = 1.0;
    float baseScaleFactor = static_cast<float>(pageScaleFactor()) * deviceScaleFactor();
    scaleFactor = scaleFactor * baseScaleFactor;

    WebCore::IntRect visibleContentRect = static_cast<WebViewEfl*>(m_pageClient)->ewkView()->visibleContentRect();
    WebCore::IntRect scaledRect = rect;
    scaledRect.move(visibleContentRect.x(), visibleContentRect.y());
    scaledRect.scale(1/baseScaleFactor);
    process()->sendSync(Messages::WebPage::CreateSnapshot(scaledRect, scaleFactor), Messages::WebPage::CreateSnapshot::Reply(snapshotHandle), m_pageID, createSnapshotSyncMessageTimeout);

    if (snapshotHandle.isNull())
        return 0;
    return WebImage::create(ShareableBitmap::create(snapshotHandle));
}
#endif

#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
void WebPageProxy::findScrollableArea(const WebCore::IntPoint& point, bool& isScrollableVertically, bool& isScrollableHorizontally)
{
    if (!isValid())
        return;

    isScrollableVertically = false;
    isScrollableHorizontally = false;
    process()->sendSync(Messages::WebPage::FindScrollableArea(point), Messages::WebPage::FindScrollableArea::Reply(isScrollableVertically, isScrollableHorizontally), m_pageID);
}

WebCore::IntSize WebPageProxy::scrollScrollableArea(const WebCore::IntSize& offset)
{
    if (!isValid())
        return WebCore::IntSize();

    if (offset.isZero())
        return WebCore::IntSize();

    WebCore::IntSize scrolledSize;
    process()->sendSync(Messages::WebPage::ScrollScrollableArea(offset), Messages::WebPage::ScrollScrollableArea::Reply(scrolledSize), m_pageID);

    return scrolledSize;
}
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)

#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
void WebPageProxy::didSetVisibleContentsRect()
{
    static_cast<WebViewEfl*>(m_pageClient)->ewkView()->didSetVisibleContentsRect();
}
#endif

#if ENABLE(TIZEN_PAGE_SUSPEND_RESUME)
void WebPageProxy::suspendPainting()
{
    if (!isValid())
        return;

    m_uiSideAnimationEnabled = false;
    process()->send(Messages::DrawingArea::SuspendPainting(), m_pageID);
}

void WebPageProxy::resumePainting()
{
    if (!isValid())
        return;

    m_uiSideAnimationEnabled = true;
    process()->send(Messages::DrawingArea::ResumePainting(), m_pageID);
}

bool WebPageProxy::uiSideAnimationEnabled()
{
    return m_uiSideAnimationEnabled;
}
#endif

#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
void WebPageProxy::scrollMainFrameBy(const WebCore::IntSize& offset)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::ScrollMainFrameBy(offset), m_pageID);
}

void WebPageProxy::scrollMainFrameTo(const WebCore::IntPoint& position)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::ScrollMainFrameTo(position), m_pageID);
}

void WebPageProxy::updateScrollPosition()
{
    if (!isValid())
        return;

    process()->sendSync(Messages::WebPage::GetScrollPosition(), Messages::WebPage::GetScrollPosition::Reply(m_scrollPosition), m_pageID);
}
#endif // #if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
void WebPageProxy::activateSpatialNavigation()
{
    if (m_spatialNavigationActivated)
        return;

    m_spatialNavigationActivated = true;
    static_cast<WebViewEfl*>(m_pageClient)->ewkView()->focusRing()->setImage(SPATIAL_NAVIGATION_FOCUS_RING_IMAGE_PATH, 3, 2);
    process()->send(Messages::WebPage::SetSpatialNavigationEnabled(true), m_pageID);
}

void WebPageProxy::stopSpatialNavigation()
{
    if (!m_spatialNavigationActivated)
        return;

    m_spatialNavigationActivated = false;
    static_cast<WebViewEfl*>(m_pageClient)->ewkView()->focusRing()->setImage(String(), 0, 0);
    process()->send(Messages::WebPage::SetSpatialNavigationEnabled(false), m_pageID);
}

void WebPageProxy::suspendSpatialNavigation()
{
    if (!m_spatialNavigationActivated)
        return;

    process()->send(Messages::WebPage::SetSpatialNavigationEnabled(false), m_pageID);
}

void WebPageProxy::resumeSpatialNavigation()
{
    if (!m_spatialNavigationActivated)
        return;

    process()->send(Messages::WebPage::SetSpatialNavigationEnabled(true), m_pageID);
}
#endif // #if ENABLE(TIZEN_SPATIAL_NAVIGATION)

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
void WebPageProxy::didFocusedRectsChanged(const Vector<WebCore::IntRect>& rects)
{
    EwkView* view = static_cast<WebViewEfl*>(m_pageClient)->ewkView();
    FocusRing* focusRing = view->focusRing();
    if (!focusRing)
        return;

    bool useSpatialNavigation = false;
#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    useSpatialNavigation = m_spatialNavigationActivated;
#endif

    if (rects.isEmpty() || (!useSpatialNavigation && focusRing->rectsChanged(rects)))
        focusRing->hide(false);
    else if (focusRing->canUpdate() || useSpatialNavigation)
        focusRing->show(rects);
}

void WebPageProxy::clearFocusedNode()
{
    process()->send(Messages::WebPage::ClearFocusedNode(), m_pageID);
}
#endif

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
void WebPageProxy::addMessageToConsole(uint32_t level, const String& message, uint32_t lineNumber, const String& source)
{
    static_cast<WebViewEfl*>(m_pageClient)->ewkView()->addMessageToConsole(level, message, lineNumber, source);
}
#endif

#if ENABLE(TIZEN_CHECK_MODAL_POPUP)
bool WebPageProxy::isOpenModalPopup()
{
    return static_cast<WebViewEfl*>(m_pageClient)->ewkView()->isOpenModalPopup();
}
#endif

} // namespace WebKit
