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

#include "config.h"
#include "BatteryProviderEfl.h"

#if ENABLE(BATTERY_STATUS)

#include "BatteryProviderEflClient.h"
#include "EventNames.h"
#if ENABLE(TIZEN_BATTERY_STATUS)
//#include <device.h>
#include <vconf.h>
#else // ENABLE(TIZEN_BATTERY_STATUS)
#include <E_Ukit.h>
#endif // ENABLE(TIZEN_BATTERY_STATUS)
#include <limits>

namespace WebCore {

#if ENABLE(TIZEN_BATTERY_STATUS)
static void batteryChargingAndLevelChangeCallback(keynode_t *keynode, void* data)
{
    char *key = vconf_keynode_get_name(keynode);
    if (!key || *key == 0) {
        TIZEN_LOGE("vconf_keynode_get_name failed");
        return;
    }

    BatteryProviderEfl* client = static_cast<BatteryProviderEfl*>(data);
    BatteryStatus* clientBatteryStatus = client->batteryStatus();

    bool charging = false;
    int chargingInt = 0;
    int chargingTime = std::numeric_limits<int>::infinity();
    int dischargingTime = std::numeric_limits<int>::infinity();
    int levelInt = 0;
    double level = 0;

    bool chargingChanged = false;
    bool chargingTimeChanged = false;
    bool dischargingTimeChanged = false;
    bool levelChanged = false;

    if (!strncmp(key, VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, strlen(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW))) {
        vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, &chargingInt);
        chargingInt ? charging = true : charging = false;

        if (clientBatteryStatus->charging() != charging)
            chargingChanged = true;
        if (charging) {
            if (clientBatteryStatus->dischargingTime() != std::numeric_limits<double>::infinity())
                dischargingTimeChanged = true;

            int ret = vconf_get_int(VCONFKEY_PM_BATTERY_TIMETOFULL, &chargingTime);
            if (ret < 0 || chargingTime < 0) {
                TIZEN_LOGE("can't get charing time");
                chargingTime = std::numeric_limits<int>::infinity();
            }

            if (clientBatteryStatus->chargingTime() != static_cast<double>(chargingTime))
                chargingTimeChanged = true;
        } else {
            if (clientBatteryStatus->chargingTime() != std::numeric_limits<double>::infinity())
                chargingTimeChanged = true;

            int ret = vconf_get_int(VCONFKEY_PM_BATTERY_TIMETOEMPTY, &dischargingTime);
            if (ret < 0 || dischargingTime < 0) {
                TIZEN_LOGE("can't get discharing time");
                dischargingTime = std::numeric_limits<int>::infinity();
            }

            if (clientBatteryStatus->dischargingTime() != static_cast<double>(dischargingTime))
                dischargingTimeChanged = true;
        }

        vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CAPACITY, &levelInt);
        level = levelInt / 100.0;
        if (clientBatteryStatus->level() != level && level != 0)
            levelChanged = true;
    }
    else if (!strncmp(key, VCONFKEY_SYSMAN_BATTERY_CAPACITY, strlen(VCONFKEY_SYSMAN_BATTERY_CAPACITY))) {
        vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CAPACITY, &levelInt);
        level = levelInt / 100.0;
        if (clientBatteryStatus->level() != level && level != 0)
            levelChanged = true;
        charging = clientBatteryStatus->charging();
        if (clientBatteryStatus->chargingTime() < 0)
            chargingTime = std::numeric_limits<int>::infinity();
        else
            chargingTime = clientBatteryStatus->chargingTime();
        if (clientBatteryStatus->dischargingTime() < 0)
            dischargingTime = std::numeric_limits<int>::infinity();
        else
            dischargingTime = clientBatteryStatus->dischargingTime();
    }

    WTF::RefPtr<BatteryStatus> batteryStatus = BatteryStatus::create(charging, static_cast<double>(chargingTime), static_cast<double>(dischargingTime), level);
    if (chargingChanged)
        client->setBatteryStatus(eventNames().chargingchangeEvent, batteryStatus);
    if (chargingTimeChanged)
        client->setBatteryStatus(eventNames().chargingtimechangeEvent, batteryStatus);
    if (dischargingTimeChanged)
        client->setBatteryStatus(eventNames().dischargingtimechangeEvent, batteryStatus);
    if (levelChanged)
        client->setBatteryStatus(eventNames().levelchangeEvent, batteryStatus);
}

static void batteryTimeToFullyChargedChangeCallback(keynode_t* keynode, void* data)
{
    char* keyname = vconf_keynode_get_name(keynode);
    if (keyname == 0 || strcmp(keyname, VCONFKEY_PM_BATTERY_TIMETOFULL) == 0)
        return;

    int chargingTime = std::numeric_limits<int>::infinity();
    int ret = vconf_get_int(VCONFKEY_PM_BATTERY_TIMETOFULL, &chargingTime);
    if (ret < 0 || chargingTime < 0)
        return;

    BatteryProviderEfl* client = static_cast<BatteryProviderEfl*>(data);
    BatteryStatus* clientBatteryStatus = client->batteryStatus();

    WTF::RefPtr<BatteryStatus> batteryStatus = BatteryStatus::create(clientBatteryStatus->charging(), static_cast<double>(chargingTime), clientBatteryStatus->dischargingTime(), clientBatteryStatus->level());
    client->setBatteryStatus(eventNames().chargingtimechangeEvent, batteryStatus);
}

static void batteryTimeToDischargedChangeCallback(keynode_t* keynode, void* data)
{
    char* keyname = vconf_keynode_get_name(keynode);
    if (keyname == 0 || strcmp(keyname, VCONFKEY_PM_BATTERY_TIMETOEMPTY) == 0)
        return;

    int dischargingTime = std::numeric_limits<int>::infinity();
    int ret = vconf_get_int(VCONFKEY_PM_BATTERY_TIMETOEMPTY, &dischargingTime);
    if (ret < 0 || dischargingTime < 0)
        return;

    BatteryProviderEfl* client = static_cast<BatteryProviderEfl*>(data);
    BatteryStatus* clientBatteryStatus = client->batteryStatus();

    WTF::RefPtr<BatteryStatus> batteryStatus = BatteryStatus::create(clientBatteryStatus->charging(), clientBatteryStatus->chargingTime(), static_cast<double>(dischargingTime), clientBatteryStatus->level());
    client->setBatteryStatus(eventNames().chargingtimechangeEvent, batteryStatus);
}
#endif // ENABLE(TIZEN_BATTERY_STATUS)

BatteryProviderEfl::BatteryProviderEfl(BatteryProviderEflClient* client)
    : m_client(client)
#if !ENABLE(TIZEN_BATTERY_STATUS)
    , m_timer(this, &BatteryProviderEfl::timerFired)
    , m_batteryStatusRefreshInterval(1.0)
#endif // !ENABLE(TIZEN_BATTERY_STATUS)
{
}

BatteryStatus* BatteryProviderEfl::batteryStatus() const
{
    return m_batteryStatus.get();
}

void BatteryProviderEfl::startUpdating()
{
#if ENABLE(TIZEN_BATTERY_STATUS)
    bool chargingBool = false;
    int chargingInt = 0;
    int chargingTime = std::numeric_limits<int>::infinity();
    int dischargingTime = std::numeric_limits<int>::infinity();
    int levelInt = 0;

    vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, &chargingInt);
    chargingInt ? chargingBool = true : chargingBool = false;

    vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CAPACITY, &levelInt);

    if (chargingBool) {
        int ret;
        ret = vconf_get_int(VCONFKEY_PM_BATTERY_TIMETOFULL, &chargingTime);
        if (ret < 0 || chargingTime < 0)
            chargingTime = std::numeric_limits<int>::infinity();
        ret = vconf_get_int(VCONFKEY_PM_BATTERY_TIMETOEMPTY, &dischargingTime);
        if (ret < 0 || dischargingTime < 0)
            dischargingTime = std::numeric_limits<int>::infinity();
    }

    double level = levelInt / 100.0;

    WTF::RefPtr<BatteryStatus> batteryStatus = BatteryStatus::create(chargingBool, static_cast<double>(chargingTime), static_cast<double>(dischargingTime), level);
    this->setBatteryStatus(eventNames().chargingchangeEvent, batteryStatus);
    this->setBatteryStatus(eventNames().chargingtimechangeEvent, batteryStatus);
    this->setBatteryStatus(eventNames().dischargingtimechangeEvent, batteryStatus);
    this->setBatteryStatus(eventNames().levelchangeEvent, batteryStatus);

    vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, batteryChargingAndLevelChangeCallback, static_cast<void*>(this));
    vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_CAPACITY, batteryChargingAndLevelChangeCallback, static_cast<void*>(this));
    vconf_notify_key_changed(VCONFKEY_PM_BATTERY_TIMETOFULL, batteryTimeToFullyChargedChangeCallback, static_cast<void*>(this));
    vconf_notify_key_changed(VCONFKEY_PM_BATTERY_TIMETOEMPTY, batteryTimeToDischargedChangeCallback, static_cast<void*>(this));
#else // ENABLE(TIZEN_BATTERY_STATUS)
    if (m_timer.isActive())
        return;

    if (!e_dbus_init())
        return;

    if (!e_ukit_init()) {
        e_dbus_shutdown();
        return;
    }

    m_timer.startRepeating(m_batteryStatusRefreshInterval);
#endif // ENABLE(TIZEN_BATTERY_STATUS)
}

void BatteryProviderEfl::stopUpdating()
{
#if ENABLE(TIZEN_BATTERY_STATUS)
    vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, batteryChargingAndLevelChangeCallback);
    vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_CAPACITY, batteryChargingAndLevelChangeCallback);
    vconf_ignore_key_changed(VCONFKEY_PM_BATTERY_TIMETOFULL, batteryTimeToFullyChargedChangeCallback);
    vconf_ignore_key_changed(VCONFKEY_PM_BATTERY_TIMETOEMPTY, batteryTimeToDischargedChangeCallback);
#else // ENABLE(TIZEN_BATTERY_STATUS)
    if (!m_timer.isActive())
        return;

    m_timer.stop();
    e_ukit_shutdown();
    e_dbus_shutdown();
#endif // ENABLE(TIZEN_BATTERY_STATUS)
}

void BatteryProviderEfl::setBatteryStatus(const AtomicString& eventType, PassRefPtr<BatteryStatus> batteryStatus)
{
    m_batteryStatus = batteryStatus;
    m_client->didChangeBatteryStatus(eventType, m_batteryStatus);
}

#if !ENABLE(TIZEN_BATTERY_STATUS)
void BatteryProviderEfl::timerFired(Timer<BatteryProviderEfl>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_timer);
    E_DBus_Connection* edbusConnection = e_dbus_bus_get(DBUS_BUS_SYSTEM);
    if (edbusConnection)
        e_upower_get_all_devices(edbusConnection, getBatteryStatus, static_cast<void*>(this));
}

void BatteryProviderEfl::getBatteryStatus(void* data, void* replyData, DBusError* dBusError)
{
    E_Ukit_Get_All_Devices_Return* eukitDeviceNames = static_cast<E_Ukit_Get_All_Devices_Return*>(replyData);
    if (!eukitDeviceNames || !eukitDeviceNames->strings || dbus_error_is_set(dBusError)) {
         dbus_error_free(dBusError);
         return;
    }

    E_DBus_Connection* edbusConnection = e_dbus_bus_get(DBUS_BUS_SYSTEM);
    Eina_List* list;
    void* deviceName;
    EINA_LIST_FOREACH(eukitDeviceNames->strings, list, deviceName)
        e_upower_get_all_properties(edbusConnection, static_cast<char*>(deviceName), setBatteryClient, data);
}

void BatteryProviderEfl::setBatteryClient(void* data, void* replyData, DBusError* dBusError)
{
    E_Ukit_Get_All_Properties_Return* eukitPropertyNames = static_cast<E_Ukit_Get_All_Properties_Return*>(replyData);

    if (!eukitPropertyNames || dbus_error_is_set(dBusError)) {
        dbus_error_free(dBusError);
        return;
    }

    if (!eukitPropertyNames->properties)
        return;

    E_Ukit_Property* property = static_cast<E_Ukit_Property*>(eina_hash_find(eukitPropertyNames->properties, "Type"));
    if (!property || property->val.u != E_UPOWER_SOURCE_BATTERY)
        return;

    BatteryProviderEfl* client = static_cast<BatteryProviderEfl*>(data);
    BatteryStatus* clientBatteryStatus = client->batteryStatus();
    bool charging = false;
    bool chargingChanged = false;
    static unsigned chargingState = 0;

    property = static_cast<E_Ukit_Property*>(eina_hash_find(eukitPropertyNames->properties, "State"));
    if (!property)
        return;
    if (!clientBatteryStatus || chargingState != property->val.u) {
        chargingChanged = true;
        chargingState = property->val.u;
        (chargingState == E_UPOWER_STATE_FULL || chargingState == E_UPOWER_STATE_CHARGING) ? charging = true : charging = false;
    } else
        charging = clientBatteryStatus->charging();

    bool chargingTimeChanged = false;
    bool dischargingTimeChanged = false;
    double chargingTime = std::numeric_limits<double>::infinity();
    double dischargingTime = std::numeric_limits<double>::infinity();

    if (charging) {
        if (!clientBatteryStatus || clientBatteryStatus->dischargingTime() != std::numeric_limits<double>::infinity())
            dischargingTimeChanged = true;
        dischargingTime = std::numeric_limits<double>::infinity();
        property = static_cast<E_Ukit_Property*>(eina_hash_find(eukitPropertyNames->properties, "TimeToFull"));
        if (!property)
            return;
        if (!clientBatteryStatus || clientBatteryStatus->chargingTime() != property->val.x)
            chargingTimeChanged = true;
        chargingTime = property->val.x;
    } else {
        if (!clientBatteryStatus || clientBatteryStatus->chargingTime() != std::numeric_limits<double>::infinity())
            chargingTimeChanged = true;
        chargingTime = std::numeric_limits<double>::infinity();
        property = static_cast<E_Ukit_Property*>(eina_hash_find(eukitPropertyNames->properties, "TimeToEmpty"));
        if (!property)
            return;
        if (!clientBatteryStatus || clientBatteryStatus->dischargingTime() != property->val.x)
            dischargingTimeChanged = true;
        dischargingTime = property->val.x;
    }

    bool levelChanged = false;
    property = static_cast<E_Ukit_Property*>(eina_hash_find(eukitPropertyNames->properties, "Percentage"));
    if (!property)
        return;

    double level = property->val.d / 100;
    if (!clientBatteryStatus || clientBatteryStatus->level() != level)
        levelChanged = true;

    WTF::RefPtr<BatteryStatus> batteryStatus = BatteryStatus::create(charging, chargingTime, dischargingTime, level);
    if (chargingChanged)
        client->setBatteryStatus(eventNames().chargingchangeEvent, batteryStatus);
    if (chargingTimeChanged)
        client->setBatteryStatus(eventNames().chargingtimechangeEvent, batteryStatus);
    if (dischargingTimeChanged)
        client->setBatteryStatus(eventNames().dischargingtimechangeEvent, batteryStatus);
    if (levelChanged)
        client->setBatteryStatus(eventNames().levelchangeEvent, batteryStatus);
}
#endif // !ENABLE(TIZEN_BATTERY_STATUS)

}

#endif // BATTERY_STATUS

