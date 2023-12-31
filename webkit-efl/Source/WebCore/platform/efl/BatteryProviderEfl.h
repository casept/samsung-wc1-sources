/*
 *  Copyright (C) 2012 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef BatteryProviderEfl_h
#define BatteryProviderEfl_h

#if ENABLE(BATTERY_STATUS)

#include "BatteryClient.h"
#include "BatteryStatus.h"
#include "Timer.h"
#include <wtf/text/AtomicString.h>

#if !ENABLE(TIZEN_BATTERY_STATUS)
typedef struct DBusError DBusError;
#endif // !ENABLE(TIZEN_BATTERY_STATUS)

namespace WebCore {

class BatteryProviderEflClient;

class BatteryProviderEfl {
public:
    BatteryProviderEfl(BatteryProviderEflClient*);
    ~BatteryProviderEfl() { }

    virtual void startUpdating();
    virtual void stopUpdating();

    void setBatteryStatus(const AtomicString& eventType, PassRefPtr<BatteryStatus>);
    BatteryStatus* batteryStatus() const;

private:
#if !ENABLE(TIZEN_BATTERY_STATUS)
    void timerFired(Timer<BatteryProviderEfl>*);
    static void getBatteryStatus(void* data, void* replyData, DBusError*);
    static void setBatteryClient(void* data, void* replyData, DBusError*);
#endif // !ENABLE(TIZEN_BATTERY_STATUS)

    BatteryProviderEflClient* m_client;
#if !ENABLE(TIZEN_BATTERY_STATUS)
    Timer<BatteryProviderEfl> m_timer;
#endif // !ENABLE(TIZEN_BATTERY_STATUS)
    RefPtr<BatteryStatus> m_batteryStatus;
#if !ENABLE(TIZEN_BATTERY_STATUS)
    const double m_batteryStatusRefreshInterval;
#endif // !ENABLE(TIZEN_BATTERY_STATUS)
};

}

#endif // ENABLE(BATTERY_STATUS)
#endif // BatteryProviderEfl_h

