/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies)
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
#include "GestureRecognizer.h"

#include "EasingCurves.h"
#include "EwkView.h"
#include "NotImplemented.h"
#include "WKSharedAPICast.h"
#include "WKViewEfl.h"

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
#include "FocusRing.h"
#endif

#if ENABLE(TOUCH_EVENTS)

#if PLATFORM(TIZEN)
#include "PageViewportController.h"
#endif

#if ENABLE(TIZEN_ENABLE_2_CPU) || ENABLE(TIZEN_WAKEUP_GPU) || ENABLE(TIZEN_GESTURE_PERFORMANCE_WORKAROUND)
#include <E_DBus.h>
#endif

#if ENABLE(TIZEN_IMPROVE_GESTURE_PERFORMANCE)
#include <Elementary.h>
#endif

using namespace WebCore;

namespace WebKit {

class GestureHandler {
public:
    static PassOwnPtr<GestureHandler> create(EwkView* ewkView)
    {
        return adoptPtr(new GestureHandler(ewkView));
    }
    ~GestureHandler();

    EwkView* view() { return m_ewkView; }

    void reset();
    void handleSingleTap(const IntPoint&);
    void handleDoubleTap(const IntPoint&);
    void handleTapAndHold(const IntPoint&);
    void handlePanStarted(const IntPoint&);
    void handlePan(const IntPoint&, double timestamp);
    void handlePanFinished();
    void handleFlick(const IntSize&);
    void handlePinchStarted(const Vector<IntPoint>&);
    void handlePinch(const Vector<IntPoint>&);
    void handlePinchFinished();
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    void showFocusRing(const WebCore::IntPoint&, bool immediately = false);
    void hideFocusRing(bool immediately = false);
#endif
#if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
    IntPoint getEstimatedCurrentPoint();
#endif
#if ENABLE(TIZEN_SCROLL_SNAP)
    IntSize getEstimatedFlickedSize(unsigned, const IntSize&);
    void handleTouchEventFinished();
#endif
#if ENABLE(TIZEN_ENABLE_2_CPU)
    void enable2CPU();
#endif
#if ENABLE(TIZEN_WAKEUP_GPU)
    void wakeUpGPU();
#endif
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
    void startMoving();
    void endMoving();
    bool isMoving() { return m_isMoving; }
#endif

private:
    explicit GestureHandler(EwkView*);

    static Eina_Bool panAnimatorCallback(void*);
    static Eina_Bool flickAnimatorCallback(void*);
#if PLATFORM(TIZEN)
    static Eina_Bool pinchAnimatorCallback(void*);
#endif
#if ENABLE(TIZEN_ENABLE_2_CPU)
    static void enable2CpuJobCallback(void*);
#endif
#if ENABLE(TIZEN_WAKEUP_GPU)
    static void gpuWakeUpJobCallback(void*);
#endif
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY) || ENABLE(TIZEN_ENABLE_2_CPU) || ENABLE(TIZEN_WAKEUP_GPU)
    static void mouseDownCallback(void*, Evas*, Evas_Object*, void* eventInfo);
#endif
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
    static void mouseMoveCallback(void*, Evas*, Evas_Object*, void* eventInfo);
#endif
#if ENABLE(TIZEN_GESTURE_PERFORMANCE_WORKAROUND)
    static Eina_Bool enableCPUBoosterCallback(void*);
#endif

    static const int s_historyCapacity = 5;
#if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
#if ENABLE(TIZEN_WEARABLE_PROFILE)
    static constexpr double s_moveEventPerSecond = 46.8;
#else
    static constexpr double s_moveEventPerSecond = 89.95;
#endif
    static constexpr double s_pauseTimeThreshold = 0.05; // seconds
    static constexpr double s_extraPredictionTime = -0.005; // seconds
#endif

    EwkView* m_ewkView;
    IntPoint m_lastPoint;
    IntPoint m_currentPoint;
    bool m_isCurrentPointUpdated;
    Ecore_Animator* m_panAnimator;
    Ecore_Animator* m_flickAnimator;
    IntSize m_flickOffset;
    unsigned m_flickDuration;
    unsigned m_flickIndex;

    struct HistoryItem {
        IntPoint point;
        double timestamp;
    };
    Vector<HistoryItem> m_history;
    size_t m_oldestHistoryItemIndex;

#if PLATFORM(TIZEN)
    Ecore_Animator* m_pinchAnimator;
    bool m_arePinchPointsUpdated;
    IntPoint m_basis;
    int m_baseDistance;
    float m_baseScaleFactor;
    float m_newScaleFactor;
    FloatPoint m_baseScrollPosition;
    FloatPoint m_newScrollPosition;
#if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
    FloatPoint m_lastVelocity;
    double m_panAnimatorTimestamp;
#endif
#if ENABLE(TIZEN_ENABLE_2_CPU)
    Ecore_Job* m_enable2CpuJob;
#endif
#if ENABLE(TIZEN_WAKEUP_GPU)
    Ecore_Job* m_gpuWakeUpJob;
#endif
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
    bool m_isMoving;
#endif
#if ENABLE(TIZEN_GESTURE_PERFORMANCE_WORKAROUND)
    Ecore_Timer* m_cpuBoosterTimer;
#endif
#if ENABLE(TIZEN_SCROLL_SNAP)
    bool m_hasScrollSnap;
    IntSize m_previousScrollSnapOffset;
#endif
#endif // #if PLATFORM(TIZEN)
};

#if ENABLE(TIZEN_ENABLE_2_CPU) || ENABLE(TIZEN_WAKEUP_GPU)
static int invokeDbusMethod(const char* destination, const char* path, const char* name, const char* method, const char* signal, const char* parameters[])
{
    DBusConnection* connection = dbus_bus_get(DBUS_BUS_SYSTEM, 0);
    if (!connection) {
        TIZEN_LOGE("dbus_bus_get error");
        return -EBADMSG;
    }
    dbus_connection_set_exit_on_disconnect(connection, false);

    DBusMessage* message = dbus_message_new_method_call(destination, path, name, method);
    if (!message) {
        TIZEN_LOGE("dbus_message_new_method_call(%s:%s-%s) error", path, name, method);
        dbus_connection_unref(connection);
        return -EBADMSG;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);

    int returnedValue = 0;
    if (signal && parameters) {
        unsigned i = 0;
        int int_type;
        uint64_t int64_type;
        for (char* ch = const_cast<char*>(signal); *ch != '\0'; ++i, ++ch) {
            switch (*ch) {
            case 'i':
                int_type = atoi(parameters[i]);
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &int_type);
                break;
            case 'u':
                int_type = atoi(parameters[i]);
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &int_type);
                break;
            case 't':
                int64_type = atoi(parameters[i]);
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &int64_type);
                break;
            case 's':
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, parameters[i]);
                break;
            default:
                returnedValue = -EINVAL;
            }
        }
    }

    if (returnedValue < 0) {
        TIZEN_LOGE("append_variant error(%d)", returnedValue);
        dbus_connection_unref(connection);
        dbus_message_unref(message);
        return -EBADMSG;
    }

    dbus_bool_t result = dbus_connection_send(connection, message, 0);
    dbus_message_unref(message);
    dbus_connection_flush(connection);
    dbus_connection_unref(connection);
    if (!result) {
        TIZEN_LOGE("dbus_connection_send error");
        return -EBADMSG;
    }

    return result;
}
#endif // #if ENABLE(TIZEN_ENABLE_2_CPU) || ENABLE(TIZEN_WAKEUP_GPU)

GestureHandler::GestureHandler(EwkView* ewkView)
    : m_ewkView(ewkView)
    , m_isCurrentPointUpdated(false)
    , m_panAnimator(0)
    , m_flickAnimator(0)
    , m_flickDuration(0)
    , m_flickIndex(0)
    , m_oldestHistoryItemIndex(0)
#if PLATFORM(TIZEN)
    , m_pinchAnimator(0)
    , m_arePinchPointsUpdated(false)
    , m_baseDistance(0)
    , m_baseScaleFactor(0)
    , m_newScaleFactor(0)
#endif // #if PLATFORM(TIZEN)
#if ENABLE(TIZEN_ENABLE_2_CPU)
    , m_enable2CpuJob(0)
#endif
#if ENABLE(TIZEN_WAKEUP_GPU)
    , m_gpuWakeUpJob(0)
#endif
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
    , m_isMoving(false)
#endif
#if ENABLE(TIZEN_GESTURE_PERFORMANCE_WORKAROUND)
    , m_cpuBoosterTimer(0)
#endif
#if ENABLE(TIZEN_SCROLL_SNAP)
    , m_hasScrollSnap(false)
#endif
{
    m_history.reserveInitialCapacity(s_historyCapacity);
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY) || ENABLE(TIZEN_ENABLE_2_CPU) || ENABLE(TIZEN_WAKEUP_GPU)
    evas_object_event_callback_add(m_ewkView->evasObject(), EVAS_CALLBACK_MOUSE_DOWN, mouseDownCallback, this);
#endif
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
    evas_object_event_callback_add(m_ewkView->evasObject(), EVAS_CALLBACK_MOUSE_MOVE, mouseMoveCallback, this);
#endif
}

GestureHandler::~GestureHandler()
{
#if PLATFORM(TIZEN)
    if (m_panAnimator)
        ecore_animator_del(m_panAnimator);
    if (m_flickAnimator)
        ecore_animator_del(m_flickAnimator);
    if (m_pinchAnimator)
        ecore_animator_del(m_pinchAnimator);
#if ENABLE(TIZEN_ENABLE_2_CPU)
    if (m_enable2CpuJob)
        ecore_job_del(m_enable2CpuJob);
#endif
#if ENABLE(TIZEN_WAKEUP_GPU)
    if (m_gpuWakeUpJob)
        ecore_job_del(m_gpuWakeUpJob);
#endif
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY) || ENABLE(TIZEN_ENABLE_2_CPU) || ENABLE(TIZEN_WAKEUP_GPU)
    evas_object_event_callback_del(m_ewkView->evasObject(), EVAS_CALLBACK_MOUSE_DOWN, mouseDownCallback);
#endif
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
    evas_object_event_callback_del(m_ewkView->evasObject(), EVAS_CALLBACK_MOUSE_MOVE, mouseMoveCallback);
#endif
#else
    reset();
#endif
}

void GestureHandler::reset()
{
    if (m_panAnimator) {
        ecore_animator_del(m_panAnimator);
        m_panAnimator = 0;
#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
        m_ewkView->setScheduleUpdateDisplayWithAnimator(false);
#endif
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)
        m_ewkView->scrollFinished();
#endif

        m_oldestHistoryItemIndex = 0;
        if (m_history.size())
            m_history.resize(0);
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
        endMoving();
#endif
    }

    if (m_flickAnimator) {
        ecore_animator_del(m_flickAnimator);
        m_flickAnimator = 0;
#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
        m_ewkView->setScheduleUpdateDisplayWithAnimator(false);
#endif
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)
        m_ewkView->scrollFinished();
#endif
    }

#if PLATFORM(TIZEN)
    if (m_pinchAnimator) {
        ecore_animator_del(m_pinchAnimator);
        m_pinchAnimator = 0;
#if ENABLE(TIZEN_IMPROVE_ZOOM_PERFORMANCE)
        m_ewkView->page()->resumePainting();
        m_ewkView->scaleTo(m_newScaleFactor, m_newScrollPosition);
#endif
    }
#endif
}

void GestureHandler::handleSingleTap(const IntPoint& position)
{
    Evas* evas = evas_object_evas_get(m_ewkView->evasObject());
    ASSERT(evas);

    // Send mouse move, down and up event to create fake click event.
    Evas_Event_Mouse_Move mouseMove;
    mouseMove.buttons = 0;
    mouseMove.prev.output.x = mouseMove.prev.canvas.x = position.x();
    mouseMove.prev.output.y = mouseMove.prev.canvas.y = position.y();
    mouseMove.cur.output.x = mouseMove.cur.canvas.x = position.x();
    mouseMove.cur.output.y = mouseMove.cur.canvas.y = position.y();
    mouseMove.data = 0;
    mouseMove.modifiers = const_cast<Evas_Modifier*>(evas_key_modifier_get(evas));
    mouseMove.locks = const_cast<Evas_Lock*>(evas_key_lock_get(evas));
    mouseMove.timestamp = ecore_loop_time_get();
    mouseMove.event_flags = EVAS_EVENT_FLAG_NONE;
    mouseMove.dev = 0;
    WKViewSendMouseMoveEvent(m_ewkView->wkView(), &mouseMove);

    Evas_Event_Mouse_Down mouseDown;
    mouseDown.button = 1;
    mouseDown.output.x = mouseDown.canvas.x = position.x();
    mouseDown.output.y = mouseDown.canvas.y = position.y();
    mouseDown.data = 0;
    mouseDown.modifiers = const_cast<Evas_Modifier*>(evas_key_modifier_get(evas));
    mouseDown.locks = const_cast<Evas_Lock*>(evas_key_lock_get(evas));
    mouseDown.flags = EVAS_BUTTON_NONE;
    mouseDown.timestamp = ecore_loop_time_get();
    mouseDown.event_flags = EVAS_EVENT_FLAG_NONE;
    mouseDown.dev = 0;
    WKViewSendMouseDownEvent(m_ewkView->wkView(), &mouseDown);

    Evas_Event_Mouse_Up mouseUp;
    mouseUp.button = 1;
    mouseUp.output.x = mouseUp.canvas.x = position.x();
    mouseUp.output.y = mouseUp.canvas.y = position.y();
    mouseUp.data = 0;
    mouseUp.modifiers = const_cast<Evas_Modifier*>(evas_key_modifier_get(evas));
    mouseUp.locks = const_cast<Evas_Lock*>(evas_key_lock_get(evas));
    mouseUp.flags = EVAS_BUTTON_NONE;
    mouseUp.timestamp = ecore_loop_time_get();
    mouseUp.event_flags = EVAS_EVENT_FLAG_NONE;
    mouseUp.dev = 0;
    WKViewSendMouseUpEvent(m_ewkView->wkView(), &mouseUp);
}

void GestureHandler::handleDoubleTap(const IntPoint& position)
{
    // FIXME: We have to set meaningful size of touch instead of "1, 1".
    WKViewFindZoomableAreaForRect(m_ewkView->wkView(), WKRectMake(position.x(), position.y(), 1, 1));
}

void GestureHandler::handleTapAndHold(const IntPoint&)
{
    notImplemented();
}

#if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
IntPoint GestureHandler::getEstimatedCurrentPoint()
{
    FloatPoint velocity(0, 0);
    IntPoint estimatedPoint = m_lastPoint;

    if (m_history.size() > 1) {
        size_t index = !m_oldestHistoryItemIndex ? m_history.size() - 1 : m_oldestHistoryItemIndex - 1;
        const HistoryItem& item1 = m_history[index];
        index = !index ? m_history.size() - 1 : index - 1;
        const HistoryItem& item2 = m_history[index];
        float timeDifference = ecore_time_get() - item1.timestamp;

        if (fabs(timeDifference) < s_pauseTimeThreshold) {
            velocity.setX((item1.point.x() - item2.point.x()) * s_moveEventPerSecond);
            velocity.setY((item1.point.y() - item2.point.y()) * s_moveEventPerSecond);
            int x = item1.point.x() + (timeDifference + s_extraPredictionTime) * velocity.x();
            int y = item1.point.y() + (timeDifference + s_extraPredictionTime) * velocity.y();

            // Prevent that point goes back even though direction of velocity is not changed.
            if ((m_lastVelocity.x() * velocity.x() >= 0)
                && (!velocity.x() || (velocity.x() < 0 && x > m_lastPoint.x()) || (velocity.x() > 0 && x < m_lastPoint.x())))
                x = m_lastPoint.x();
            if ((m_lastVelocity.y() * velocity.y() >= 0)
                && (!velocity.y() || (velocity.y() < 0 && y > m_lastPoint.y()) || (velocity.y() > 0 && y < m_lastPoint.y())))
                y = m_lastPoint.y();

            estimatedPoint = IntPoint(x, y);
        }
    }
    m_lastVelocity = velocity;

    return estimatedPoint;
}
#endif // #if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)

Eina_Bool GestureHandler::panAnimatorCallback(void* data)
{
    GestureHandler* gestureHandler = static_cast<GestureHandler*>(data);
#if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
    gestureHandler->m_currentPoint = gestureHandler->getEstimatedCurrentPoint();

    if (!gestureHandler->m_isCurrentPointUpdated || gestureHandler->m_lastPoint == gestureHandler->m_currentPoint)
        return ECORE_CALLBACK_RENEW;

    // deltaCalibraion is for the compensation of the irregualr animator time.
    // delatCalibration should be set when m_currentAnimatorTime is between 8ms and 32ms.
    // If not, deltaX and deltaY can have meaningless values.
    double deltaCalibration = 1.0;
    double animatorDuration = gestureHandler->m_panAnimatorTimestamp ? ecore_time_get() - gestureHandler->m_panAnimatorTimestamp : 0;
    gestureHandler->m_panAnimatorTimestamp = ecore_time_get();
    if (animatorDuration > 0.008 && animatorDuration < 0.032)
        deltaCalibration = ecore_animator_frametime_get() / animatorDuration;
    int deltaX = floor((gestureHandler->m_lastPoint.x() - gestureHandler->m_currentPoint.x()) * deltaCalibration + 0.5);
    int deltaY = floor((gestureHandler->m_lastPoint.y() - gestureHandler->m_currentPoint.y()) * deltaCalibration + 0.5);

    gestureHandler->view()->scrollBy(IntSize(deltaX, deltaY));
#else // #if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
    if (!gestureHandler->m_isCurrentPointUpdated || gestureHandler->m_lastPoint == gestureHandler->m_currentPoint)
        return ECORE_CALLBACK_RENEW;

    gestureHandler->view()->scrollBy(gestureHandler->m_lastPoint - gestureHandler->m_currentPoint);
#endif // #if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
    gestureHandler->m_lastPoint = gestureHandler->m_currentPoint;
    gestureHandler->m_isCurrentPointUpdated = false;

    return ECORE_CALLBACK_RENEW;
}

void GestureHandler::handlePanStarted(const IntPoint& point)
{
    ASSERT(!m_panAnimator);
    m_panAnimator = ecore_animator_add(panAnimatorCallback, this);
    m_lastPoint = m_currentPoint = point;
#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
    m_ewkView->setScheduleUpdateDisplayWithAnimator(true);
#endif
}

void GestureHandler::handlePan(const IntPoint& point, double timestamp)
{
    m_currentPoint = point;
    m_isCurrentPointUpdated = true;

    // Save current point to use to calculate offset of flick.
    HistoryItem item = { m_currentPoint, timestamp };
    if (m_history.size() < m_history.capacity())
        m_history.uncheckedAppend(item);
    else {
        m_history[m_oldestHistoryItemIndex++] = item;
        if (m_oldestHistoryItemIndex == m_history.capacity())
            m_oldestHistoryItemIndex = 0;
    }
}

void GestureHandler::handlePanFinished()
{
    ASSERT(m_panAnimator);
    ecore_animator_del(m_panAnimator);
    m_panAnimator = 0;

    if (!m_history.isEmpty()) {
        // Calculate offset to move during one frame.
        const HistoryItem& oldestHistoryItem = m_history[m_oldestHistoryItemIndex];
        double frame = (ecore_time_get() - oldestHistoryItem.timestamp) / ecore_animator_frametime_get();
#if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
        // We have to use latest HistoryItem because m_currentPoint is estimated value.
        size_t index = !m_oldestHistoryItemIndex ? m_history.size() - 1 : m_oldestHistoryItemIndex - 1;
        const HistoryItem& latestHistoryItem = m_history[index];
        IntSize offset = oldestHistoryItem.point - latestHistoryItem.point;
#else
        IntSize offset = oldestHistoryItem.point - m_currentPoint;
#endif // #if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
        offset.scale(1 / frame);
#if ENABLE(TIZEN_GESTURE_PERFORMANCE_WORKAROUND)
        const char* parameters[1];
        char time[5] = "1000"; // ms.
        parameters[0] = time;

        invokeDbusMethod("org.tizen.system.deviced", "/Org/Tizen/System/DeviceD/PmQos",
                         "org.tizen.system.deviced.PmQos", "WebAppFlick", "i", parameters);
#endif
        handleFlick(offset);
    }
#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
    else
        m_ewkView->setScheduleUpdateDisplayWithAnimator(false);
#endif
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)
    if (m_history.isEmpty())
        m_ewkView->scrollFinished();
#endif

    m_oldestHistoryItemIndex = 0;
    if (m_history.size())
        m_history.resize(0);
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
    endMoving();
#endif
}

#if ENABLE(TIZEN_SCROLL_SNAP)
IntSize GestureHandler::getEstimatedFlickedSize(unsigned index, const IntSize& offset)
{
    IntSize flickedSize;
    if (offset.isZero())
        return flickedSize;

    for (unsigned i = index; i > 0; --i) {
#if ENABLE(TIZEN_FLICK_FASTER)
        float multiplier = easeOutCubic(i, 0, 1.0, m_flickDuration);
#else
        float multiplier = easeInOutQuad(i, 0, 1.0, m_flickDuration);
#endif
        float offsetWidth = offset.width() * multiplier;
        float offsetHeight = offset.height() * multiplier;
        offsetWidth = (offsetWidth > 0) ? ceilf(offsetWidth) : floorf(offsetWidth);
        offsetHeight = (offsetHeight > 0) ? ceilf(offsetHeight) : floorf(offsetHeight);

        flickedSize.expand(offsetWidth, offsetHeight);
    }

    return flickedSize;
}

void GestureHandler::handleTouchEventFinished()
{
    if (m_hasScrollSnap && !m_flickAnimator) {
        IntSize remain = m_flickOffset - m_previousScrollSnapOffset;
        if (remain.width())
            remain.setWidth(remain.width() > 0 ? 1 : -1);
        if (remain.height())
            remain.setHeight(remain.height() > 0 ? 1 : -1);
        handleFlick(remain);
    }
}
#endif // #if ENABLE(TIZEN_SCROLL_SNAP)

Eina_Bool GestureHandler::flickAnimatorCallback(void* data)
{
    GestureHandler* gestureHandler = static_cast<GestureHandler*>(data);
#if ENABLE(TIZEN_FLICK_FASTER)
    float multiplier = easeOutCubic(gestureHandler->m_flickIndex, 0, 1.0, gestureHandler->m_flickDuration);
#else
    float multiplier = easeInOutQuad(gestureHandler->m_flickIndex, 0, 1.0, gestureHandler->m_flickDuration);
#endif
    float offsetWidth = gestureHandler->m_flickOffset.width() * multiplier;
    float offsetHeight = gestureHandler->m_flickOffset.height() * multiplier;
    offsetWidth = (offsetWidth > 0) ? ceilf(offsetWidth) : floorf(offsetWidth);
    offsetHeight = (offsetHeight > 0) ? ceilf(offsetHeight) : floorf(offsetHeight);
    IntSize offset(offsetWidth, offsetHeight);

#if ENABLE(TIZEN_SCROLL_SNAP)
    if (gestureHandler->m_hasScrollSnap) {
        if (!multiplier)
            offset = gestureHandler->m_flickOffset;
        IntSize diff = offset - gestureHandler->m_previousScrollSnapOffset;
        gestureHandler->m_previousScrollSnapOffset = offset;
        offset = diff;
        gestureHandler->m_flickIndex++;
    } else
#endif
    gestureHandler->m_flickIndex--;

    if (offset.isZero() || !gestureHandler->view()->scrollBy(offset) || !gestureHandler->m_flickIndex) {
#if ENABLE(TIZEN_SCROLL_SNAP)
        if (gestureHandler->m_hasScrollSnap && gestureHandler->m_previousScrollSnapOffset != gestureHandler->m_flickOffset)
            return ECORE_CALLBACK_RENEW;
        gestureHandler->m_hasScrollSnap = false;
#endif
        gestureHandler->m_flickAnimator = 0;
#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
        gestureHandler->view()->setScheduleUpdateDisplayWithAnimator(false);
#endif
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)
        gestureHandler->view()->scrollFinished();
#endif
        return ECORE_CALLBACK_CANCEL;
    }

    return ECORE_CALLBACK_RENEW;
}

void GestureHandler::handleFlick(const IntSize& offset)
{
    m_flickOffset = offset;
    m_flickIndex = m_flickDuration = 1 / ecore_animator_frametime_get();
    m_flickAnimator = ecore_animator_add(flickAnimatorCallback, this);

#if ENABLE(TIZEN_SCROLL_SNAP)
    IntSize scrollSnapOffset = m_ewkView->findScrollSnapOffset(getEstimatedFlickedSize(m_flickIndex, m_flickOffset));
    m_hasScrollSnap = !scrollSnapOffset.isZero();
    if (m_hasScrollSnap) {
        m_flickOffset = scrollSnapOffset;
        m_flickIndex = 1;
        m_previousScrollSnapOffset = IntSize();
    }
#endif
}

#if PLATFORM(TIZEN)
Eina_Bool GestureHandler::pinchAnimatorCallback(void* data)
{
    GestureHandler* gestureHandler = static_cast<GestureHandler*>(data);
    if (!gestureHandler->m_arePinchPointsUpdated)
        return ECORE_CALLBACK_RENEW;

#if ENABLE(TIZEN_IMPROVE_ZOOM_PERFORMANCE)
    WKViewSetContentScaleFactor(gestureHandler->m_ewkView->wkView(), gestureHandler->m_newScaleFactor);
    WKViewSetContentPosition(gestureHandler->m_ewkView->wkView(), WKPointMake(gestureHandler->m_newScrollPosition.x(), gestureHandler->m_newScrollPosition.y()));
    gestureHandler->m_ewkView->scheduleUpdateDisplay();
#else
    gestureHandler->m_ewkView->scaleTo(gestureHandler->m_newScaleFactor, gestureHandler->m_newScrollPosition);
#endif
    gestureHandler->m_arePinchPointsUpdated = false;

    return ECORE_CALLBACK_RENEW;
}

void GestureHandler::handlePinchStarted(const Vector<IntPoint>& points)
{
    ASSERT(!m_pinchAnimator);
    m_pinchAnimator = ecore_animator_add(pinchAnimatorCallback, this);

    int viewX, viewY;
    evas_object_geometry_get(m_ewkView->evasObject(), &viewX, &viewY, 0, 0);
    m_basis = IntPoint((points[0].x() + points[1].x()) / 2 - viewX, (points[0].y() + points[1].y()) / 2 - viewY);
    m_baseDistance = sqrt(points[0].distanceSquaredToPoint(points[1]));
    m_newScaleFactor = m_baseScaleFactor = WKViewGetContentScaleFactor(m_ewkView->wkView());
    WKPoint contentsPosition = WKViewGetContentPosition(m_ewkView->wkView());
    m_newScrollPosition = m_baseScrollPosition = FloatPoint(contentsPosition.x, contentsPosition.y);
#if ENABLE(TIZEN_IMPROVE_ZOOM_PERFORMANCE)
    m_ewkView->page()->suspendPainting();
#endif
}

void GestureHandler::handlePinch(const Vector<IntPoint>& points)
{
    double distance = sqrt(points[0].distanceSquaredToPoint(points[1]));
    m_newScaleFactor = m_baseScaleFactor * (1 + ((distance - m_baseDistance) / m_baseDistance));
#if USE(ACCELERATED_COMPOSITING)
    if (PageViewportController* viewportController = m_ewkView->pageViewportController()) {
        if (m_newScaleFactor < viewportController->minimumScale())
            m_newScaleFactor = viewportController->minimumScale();
        else if (m_newScaleFactor > viewportController->maximumScale())
            m_newScaleFactor = viewportController->maximumScale();
    }
#endif

    // Calculate new scroll position changed by scaling.
    int viewX, viewY;
    evas_object_geometry_get(m_ewkView->evasObject(), &viewX, &viewY, 0, 0);
    m_newScrollPosition.setX(m_baseScrollPosition.x() + (m_basis.x() / m_baseScaleFactor - m_basis.x() / m_newScaleFactor) / m_ewkView->deviceScaleFactor());
    m_newScrollPosition.setY(m_baseScrollPosition.y() + (m_basis.y() / m_baseScaleFactor - m_basis.y() / m_newScaleFactor) / m_ewkView->deviceScaleFactor());
#if USE(ACCELERATED_COMPOSITING)
    m_newScrollPosition = m_ewkView->pageViewportController()->boundContentsPositionAtScale(m_newScrollPosition, m_newScaleFactor);
#endif

    m_arePinchPointsUpdated = true;
}

void GestureHandler::handlePinchFinished()
{
    ASSERT(m_pinchAnimator);
    ecore_animator_del(m_pinchAnimator);
    m_pinchAnimator = 0;
#if ENABLE(TIZEN_IMPROVE_ZOOM_PERFORMANCE)
    m_ewkView->page()->resumePainting();
    m_ewkView->scaleTo(m_newScaleFactor, m_newScrollPosition);
#endif
}
#else // #if PLATFORM(TIZEN)
void GestureHandler::handlePinchStarted(const Vector<IntPoint>&)
{
    notImplemented();
}

void GestureHandler::handlePinch(const Vector<IntPoint>&)
{
    notImplemented();
}

void GestureHandler::handlePinchFinished()
{
    notImplemented();
}
#endif // #if PLATFORM(TIZEN)

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
void GestureHandler::showFocusRing(const IntPoint& position, bool immediately)
{
    if (m_ewkView->focusRing())
        m_ewkView->focusRing()->requestToShow(position, immediately);
}

void GestureHandler::hideFocusRing(bool immediately)
{
    if (m_ewkView->focusRing())
        m_ewkView->focusRing()->requestToHide(immediately);
}
#endif

#if ENABLE(TIZEN_ENABLE_2_CPU)
void GestureHandler::enable2CpuJobCallback(void* data)
{
    const char* parameters[1];
    char time[4] = "100"; // ms.
    parameters[0] = time;
    int result = invokeDbusMethod("org.tizen.system.deviced", "/Org/Tizen/System/DeviceD/PmQos",
                                  "org.tizen.system.deviced.PmQos", "BrowserScroll", "i", parameters);
    static_cast<GestureHandler*>(data)->m_enable2CpuJob = 0;
    TIZEN_LOGI("[Touch] Enable minimum 2 CPU[%d]", result);

#if ENABLE(TIZEN_GESTURE_PERFORMANCE_WORKAROUND)
    // FIXME: If cpu boosted while gpu wake up, this patch is unnecessary.
    result = invokeDbusMethod("org.tizen.system.deviced", "/Org/Tizen/System/DeviceD/PmQos",
                                  "org.tizen.system.deviced.PmQos", "WebAppDrag", "i", parameters);
    GestureHandler* handler = static_cast<GestureHandler*>(data);
    if (handler->m_cpuBoosterTimer)
        ecore_timer_del(handler->m_cpuBoosterTimer);
    handler->m_cpuBoosterTimer = ecore_timer_add(0.1, enableCPUBoosterCallback, handler);
#endif
}

#if ENABLE(TIZEN_GESTURE_PERFORMANCE_WORKAROUND)
Eina_Bool GestureHandler::enableCPUBoosterCallback(void* data)
{
    GestureHandler* handler = static_cast<GestureHandler*>(data);
    if (!handler->m_panAnimator) {
        handler->m_cpuBoosterTimer = 0;
        return ECORE_CALLBACK_DONE;
    }

    const char* parameters[1];
    char time[4] = "100"; // ms.
    parameters[0] = time;
    invokeDbusMethod("org.tizen.system.deviced", "/Org/Tizen/System/DeviceD/PmQos",
                                  "org.tizen.system.deviced.PmQos", "WebAppDrag", "i", parameters);
    return ECORE_CALLBACK_RENEW;
}
#endif

void GestureHandler::enable2CPU()
{
    // Enable minimum 2 CPU.
    // If we do not enable 2 CPU, other process can interrupt during panning,
    // and it will reduce the touch responsiveness.
    m_enable2CpuJob = ecore_job_add(enable2CpuJobCallback, this);
}
#endif // #if ENABLE(TIZEN_ENABLE_2_CPU)

#if ENABLE(TIZEN_WAKEUP_GPU)
void GestureHandler::gpuWakeUpJobCallback(void* data)
{
    const char* parameters[1];
    char time[2] = "1"; // Use Default time (3000ms).
    parameters[0] = time;
    int result = invokeDbusMethod("org.tizen.system.deviced", "/Org/Tizen/System/DeviceD/PmQos",
                                  "org.tizen.system.deviced.PmQos", "GpuWakeup", "i", parameters);
    static_cast<GestureHandler*>(data)->m_gpuWakeUpJob = 0;
    TIZEN_LOGI("[Touch] Wake up GPU[%d]", result);
}

void GestureHandler::wakeUpGPU()
{
    m_gpuWakeUpJob = ecore_job_add(gpuWakeUpJobCallback, this);
}
#endif // #if ENABLE(TIZEN_WAKEUP_GPU)

#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY) || ENABLE(TIZEN_ENABLE_2_CPU) || ENABLE(TIZEN_WAKEUP_GPU)
void GestureHandler::mouseDownCallback(void* data, Evas* /* canvas */, Evas_Object* /* ewkView */, void* /* eventInfo */)
{
#if ENABLE(TIZEN_ENABLE_2_CPU)
    // We have to enable 2 CPU before waking up GPU in order to reduce consuming time for enabling GPU.
    static_cast<GestureHandler*>(data)->enable2CPU();
#endif
#if ENABLE(TIZEN_WAKEUP_GPU)
    static_cast<GestureHandler*>(data)->wakeUpGPU();
#endif
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
    static_cast<GestureHandler*>(data)->startMoving();
#endif
}
#endif // #if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY) || ENABLE(TIZEN_ENABLE_2_CPU) || ENABLE(TIZEN_WAKEUP_GPU)

#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
void GestureHandler::mouseMoveCallback(void* data, Evas* /* canvas */, Evas_Object* /* ewkView */, void* eventInfo)
{
    GestureHandler* gestureHandler = static_cast<GestureHandler*>(data);
    if (!gestureHandler->isMoving())
        return;

    Evas_Event_Mouse_Move* mouseMove = static_cast<Evas_Event_Mouse_Move*>(eventInfo);
    gestureHandler->handlePan(IntPoint(mouseMove->cur.output.x, mouseMove->cur.output.y), mouseMove->timestamp / 1000.0);
}

void GestureHandler::startMoving()
{
    m_isMoving = true;
#if ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
    m_lastVelocity.setX(0);
    m_lastVelocity.setY(0);
    m_panAnimatorTimestamp = 0;
#endif
}

void GestureHandler::endMoving()
{
    m_isMoving = false;
    m_oldestHistoryItemIndex = 0;
    if (m_history.size())
        m_history.resize(0);
}
#endif

const double GestureRecognizer::s_doubleTapTimeoutInSeconds = 0.4;
const double GestureRecognizer::s_tapAndHoldTimeoutInSeconds = 1.0;
const int GestureRecognizer::s_squaredDoubleTapThreshold = 10000;
const int GestureRecognizer::s_squaredPanThreshold = 100;

GestureRecognizer::GestureRecognizer(EwkView* ewkView)
    : m_recognizerFunction(&GestureRecognizer::noGesture)
    , m_gestureHandler(GestureHandler::create(ewkView))
    , m_tapAndHoldTimer(0)
    , m_doubleTapTimer(0)
    , m_useDoubleTap(true)
#if ENABLE(TIZEN_MULTI_TOUCH_TAP)
    , m_supportScaling(false)
#endif
{
}

GestureRecognizer::~GestureRecognizer()
{
}

void GestureRecognizer::processTouchEvent(WKTouchEventRef eventRef)
{
    WKEventType type = WKTouchEventGetType(eventRef);
    if (type == kWKEventTypeTouchCancel) {
        reset();
        return;
    }

    (this->*m_recognizerFunction)(eventRef);

#if ENABLE(TIZEN_SCROLL_SNAP)
    if (type == kWKEventTypeTouchEnd)
        m_gestureHandler->handleTouchEventFinished();
#endif
}

Eina_Bool GestureRecognizer::doubleTapTimerCallback(void* data)
{
    GestureRecognizer* gestureRecognizer = static_cast<GestureRecognizer*>(data);
    gestureRecognizer->m_doubleTapTimer = 0;

    // If doubleTapTimer is fired we should not process double tap,
    // so process single tap here if touched point is already released
    // or do nothing for processing single tap when touch is released.
    if (gestureRecognizer->m_recognizerFunction == &GestureRecognizer::doubleTapGesture) {
        gestureRecognizer->m_gestureHandler->handleSingleTap(gestureRecognizer->m_firstPressedPoint);
        gestureRecognizer->m_recognizerFunction = &GestureRecognizer::noGesture;
    }

    return ECORE_CALLBACK_CANCEL;
}

Eina_Bool GestureRecognizer::tapAndHoldTimerCallback(void* data)
{
    GestureRecognizer* gestureRecognizer = static_cast<GestureRecognizer*>(data);
    gestureRecognizer->m_tapAndHoldTimer = 0;
    gestureRecognizer->m_gestureHandler->handleTapAndHold(gestureRecognizer->m_firstPressedPoint);
    gestureRecognizer->m_recognizerFunction = &GestureRecognizer::noGesture;
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
    gestureRecognizer->m_gestureHandler->endMoving();
#endif

    return ECORE_CALLBACK_CANCEL;
}

inline bool GestureRecognizer::exceedsPanThreshold(const IntPoint& first, const IntPoint& last) const
{
#if ENABLE(TIZEN_IMPROVE_GESTURE_PERFORMANCE)
    unsigned moveThreshold = elm_config_scroll_thumbscroll_threshold_get();
    return first.distanceSquaredToPoint(last) > (moveThreshold * moveThreshold);
#else
    return first.distanceSquaredToPoint(last) > s_squaredPanThreshold;
#endif
}

inline bool GestureRecognizer::exceedsDoubleTapThreshold(const IntPoint& first, const IntPoint& last) const
{
    return first.distanceSquaredToPoint(last) > s_squaredDoubleTapThreshold;
}

static inline WKPoint getPointAtIndex(WKArrayRef array, size_t index)
{
    WKTouchPointRef pointRef = static_cast<WKTouchPointRef>(WKArrayGetItemAtIndex(array, index));
    ASSERT(pointRef);

    return WKTouchPointGetPosition(pointRef);
}

static inline Vector<IntPoint> createVectorWithWKArray(WKArrayRef array, size_t size)
{
    Vector<IntPoint> points;
    points.reserveCapacity(size);
    for (size_t i = 0; i < size; ++i)
        points.uncheckedAppend(toIntPoint(getPointAtIndex(array, i)));

    return points;
}

void GestureRecognizer::noGesture(WKTouchEventRef eventRef)
{
    switch (WKTouchEventGetType(eventRef)) {
    case kWKEventTypeTouchStart: {
        WKArrayRef touchPoints = WKTouchEventGetTouchPoints(eventRef);
        switch (WKArrayGetSize(touchPoints)) {
        case 1:
            m_gestureHandler->reset();
            m_recognizerFunction = &GestureRecognizer::singleTapGesture;
            m_firstPressedPoint = toIntPoint(getPointAtIndex(WKTouchEventGetTouchPoints(eventRef), 0));

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
            m_gestureHandler->showFocusRing(m_firstPressedPoint);
#endif
#if ENABLE(TIZEN_SET_FOCUS_TO_EWK_VIEW)
            evas_object_focus_set(m_gestureHandler->view()->evasObject(), true);
#endif
#if !ENABLE(TIZEN_WEARABLE_PROFILE)
            ASSERT(!m_tapAndHoldTimer);
            m_tapAndHoldTimer = ecore_timer_add(s_tapAndHoldTimeoutInSeconds, tapAndHoldTimerCallback, this);
#endif
            if (m_useDoubleTap)
                m_doubleTapTimer = ecore_timer_add(s_doubleTapTimeoutInSeconds, doubleTapTimerCallback, this);
            break;
        case 2:
#if ENABLE(TIZEN_MULTI_TOUCH_TAP)
            if (!m_supportScaling)
                break;
#endif
            m_recognizerFunction = &GestureRecognizer::pinchGesture;
            m_gestureHandler->handlePinchStarted(createVectorWithWKArray(touchPoints, 2));
            break;
        default:
            // There's no defined gesture when we touch three or more points.
            notImplemented();
            break;
        }
        break;
    }
    case kWKEventTypeTouchMove: {
#if ENABLE(TIZEN_GESTURE_WITH_PREVENT_DEFAULT)
        IntPoint currentPoint = toIntPoint(getPointAtIndex(WKTouchEventGetTouchPoints(eventRef), 0));
        if (exceedsPanThreshold(m_firstPressedPoint, currentPoint)) {
            m_recognizerFunction = &GestureRecognizer::panGesture;
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
            m_gestureHandler->view()->findScrollableArea(m_firstPressedPoint);
#endif
            m_gestureHandler->handlePanStarted(currentPoint);
        }
#endif
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        break;
#endif
    }
    case kWKEventTypeTouchEnd:
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        if (!m_tapAndHoldTimer)
            m_gestureHandler->hideFocusRing(true);
#endif
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void GestureRecognizer::singleTapGesture(WKTouchEventRef eventRef)
{
    WKArrayRef touchPoints = WKTouchEventGetTouchPoints(eventRef);
#if ENABLE(TIZEN_MULTI_TOUCH_TAP)
    size_t numberOfTouchPoints = WKArrayGetSize(touchPoints);
#endif

    switch (WKTouchEventGetType(eventRef)) {
    case kWKEventTypeTouchStart:
#if ENABLE(TIZEN_MULTI_TOUCH_TAP)
        if (!m_supportScaling)
            break;
#endif
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        m_gestureHandler->hideFocusRing(true);
#endif
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
        m_gestureHandler->endMoving();
#endif
        stopTapTimers();
        m_recognizerFunction = &GestureRecognizer::pinchGesture;
        m_gestureHandler->handlePinchStarted(createVectorWithWKArray(touchPoints, 2));
        break;
    case kWKEventTypeTouchMove: {
        IntPoint currentPoint = toIntPoint(getPointAtIndex(touchPoints, 0));
        if (exceedsPanThreshold(m_firstPressedPoint, currentPoint)) {
            stopTapTimers();
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
            m_gestureHandler->hideFocusRing(true);
#endif
            m_recognizerFunction = &GestureRecognizer::panGesture;
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
            m_gestureHandler->view()->findScrollableArea(m_firstPressedPoint);
#endif
#if ENABLE(TIZEN_GESTURE_PERFORMANCE_WORKAROUND)
            m_gestureHandler->handlePanStarted(m_firstPressedPoint);
#else
            m_gestureHandler->handlePanStarted(currentPoint);
#endif
        }
        break;
    }
    case kWKEventTypeTouchEnd:
#if ENABLE(TIZEN_MULTI_TOUCH_TAP)
        if (numberOfTouchPoints > 1)
            break;
        if (WKTouchPointGetID(static_cast<WKTouchPointRef>(WKArrayGetItemAtIndex(touchPoints, 0)))) {
            m_recognizerFunction = &GestureRecognizer::noGesture;
            break;
        }
#endif
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        m_gestureHandler->hideFocusRing();
#endif
        if (m_tapAndHoldTimer) {
            ecore_timer_del(m_tapAndHoldTimer);
            m_tapAndHoldTimer = 0;
        }

        if (m_doubleTapTimer)
            m_recognizerFunction = &GestureRecognizer::doubleTapGesture;
        else {
            m_gestureHandler->handleSingleTap(m_firstPressedPoint);
            m_recognizerFunction = &GestureRecognizer::noGesture;
        }
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
        m_gestureHandler->endMoving();
#endif
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void GestureRecognizer::doubleTapGesture(WKTouchEventRef eventRef)
{
    WKArrayRef touchPoints = WKTouchEventGetTouchPoints(eventRef);

    switch (WKTouchEventGetType(eventRef)) {
    case kWKEventTypeTouchStart: {
        if (m_doubleTapTimer) {
            ecore_timer_del(m_doubleTapTimer);
            m_doubleTapTimer = 0;
        }

        size_t numberOfTouchPoints = WKArrayGetSize(touchPoints);
        if (numberOfTouchPoints == 1) {
            if (exceedsDoubleTapThreshold(m_firstPressedPoint, toIntPoint(getPointAtIndex(touchPoints, 0))))
                m_recognizerFunction = &GestureRecognizer::singleTapGesture;
        } else {
#if ENABLE(TIZEN_MULTI_TOUCH_TAP)
            if (!m_supportScaling)
                break;
#endif
            m_recognizerFunction = &GestureRecognizer::pinchGesture;
            m_gestureHandler->handlePinchStarted(createVectorWithWKArray(touchPoints, 2));
        }
        break;
    }
    case kWKEventTypeTouchMove: {
        IntPoint currentPoint = toIntPoint(getPointAtIndex(touchPoints, 0));
        if (exceedsPanThreshold(m_firstPressedPoint, currentPoint)) {
            m_recognizerFunction = &GestureRecognizer::panGesture;
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
            m_gestureHandler->view()->findScrollableArea(m_firstPressedPoint);
#endif
            m_gestureHandler->handlePanStarted(currentPoint);
        }
        break;
    }
    case kWKEventTypeTouchEnd:
        m_gestureHandler->handleDoubleTap(m_firstPressedPoint);
        m_recognizerFunction = &GestureRecognizer::noGesture;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void GestureRecognizer::panGesture(WKTouchEventRef eventRef)
{
    WKArrayRef touchPoints = WKTouchEventGetTouchPoints(eventRef);

    switch (WKTouchEventGetType(eventRef)) {
    case kWKEventTypeTouchStart:
#if ENABLE(TIZEN_MULTI_TOUCH_TAP)
        if (!m_supportScaling)
            break;
#endif
        m_recognizerFunction = &GestureRecognizer::pinchGesture;
#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY)
        m_gestureHandler->endMoving();
#endif
        m_gestureHandler->handlePanFinished();
#endif
        m_gestureHandler->handlePinchStarted(createVectorWithWKArray(touchPoints, 2));
        break;
    case kWKEventTypeTouchMove:
#if !ENABLE(TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE)
        m_gestureHandler->handlePan(toIntPoint(getPointAtIndex(touchPoints, 0)), WKTouchEventGetTimestamp(eventRef));
#endif
        break;
    case kWKEventTypeTouchEnd:
#if ENABLE(TIZEN_MULTI_TOUCH_TAP)
        if (WKArrayGetSize(touchPoints) > 1)
            break;
#endif
        m_gestureHandler->handlePanFinished();
        m_recognizerFunction = &GestureRecognizer::noGesture;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void GestureRecognizer::pinchGesture(WKTouchEventRef eventRef)
{
    WKArrayRef touchPoints = WKTouchEventGetTouchPoints(eventRef);
    size_t numberOfTouchPoints = WKArrayGetSize(touchPoints);
    ASSERT(numberOfTouchPoints >= 2);

    switch (WKTouchEventGetType(eventRef)) {
    case kWKEventTypeTouchMove: {
        m_gestureHandler->handlePinch(createVectorWithWKArray(touchPoints, 2));
        break;
    }
    case kWKEventTypeTouchEnd:
        if (numberOfTouchPoints == 2) {
            m_gestureHandler->handlePinchFinished();
            m_recognizerFunction = &GestureRecognizer::panGesture;
            WKTouchPointRef pointRef;
            for (size_t i = 0; i < numberOfTouchPoints; ++i) {
                pointRef = static_cast<WKTouchPointRef>(WKArrayGetItemAtIndex(touchPoints, i));
                WKTouchPointState state = WKTouchPointGetState(pointRef);
                if (state != kWKTouchPointStateTouchReleased && state != kWKTouchPointStateTouchCancelled)
                    break;
            }
            ASSERT(pointRef);
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
            m_gestureHandler->view()->findScrollableArea(toIntPoint(WKTouchPointGetPosition(pointRef)));
#endif
            m_gestureHandler->handlePanStarted(toIntPoint(WKTouchPointGetPosition(pointRef)));
        }
        break;
    case kWKEventTypeTouchStart:
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void GestureRecognizer::reset()
{
    stopTapTimers();
    m_gestureHandler->reset();

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    m_gestureHandler->hideFocusRing(true);
#endif
    m_recognizerFunction = &GestureRecognizer::noGesture;
}

void GestureRecognizer::setUseDoubleTap(bool use)
{
    if (!use && m_doubleTapTimer) {
        ecore_timer_del(m_doubleTapTimer);
        m_doubleTapTimer = 0;
    }

    m_useDoubleTap = use;
#if ENABLE(TIZEN_MULTI_TOUCH_TAP)
    m_supportScaling = use;
#endif
}

void GestureRecognizer::stopTapTimers()
{
    if (m_doubleTapTimer) {
        ecore_timer_del(m_doubleTapTimer);
        m_doubleTapTimer = 0;
    }

    if (m_tapAndHoldTimer) {
        ecore_timer_del(m_tapAndHoldTimer);
        m_tapAndHoldTimer = 0;
    }
}

} // namespace WebKit

#endif // ENABLE(TOUCH_EVENTS)
