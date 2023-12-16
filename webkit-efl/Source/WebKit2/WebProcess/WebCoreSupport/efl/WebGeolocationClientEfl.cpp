/*
    Copyright (C) 2012 Samsung Electronics

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
#include "WebGeolocationClient.h"

#if ENABLE(TIZEN_GEOLOCATION)
#include "WebPage.h"
#include <WebCore/GeolocationController.h>
#include <WebCore/Page.h>

using namespace WebCore;

namespace WebKit {

bool WebGeolocationClient::startLocationManager()
{
    location_error_e ret = (location_error_e)location_manager_create(LOCATIONS_METHOD_HYBRID, &m_location_manager);
    if (ret != LOCATIONS_ERROR_NONE)
        return false;

    ret = (location_error_e)location_manager_set_position_updated_cb(m_location_manager, positionChangedCallback, 1, (void*)this);
    if (ret != LOCATIONS_ERROR_NONE)
        return false;

    ret = (location_error_e)location_manager_start(m_location_manager);
    if (ret != LOCATIONS_ERROR_NONE) {
        location_manager_destroy(m_location_manager);
        return false;
    }
    return true;
}

void WebGeolocationClient::stopLocationManager()
{
    location_error_e ret = (location_error_e)location_manager_stop(m_location_manager);
    if (ret != LOCATIONS_ERROR_NONE)
        location_manager_destroy(m_location_manager);

    ret = (location_error_e)location_manager_unset_position_updated_cb(m_location_manager);
    if (ret != LOCATIONS_ERROR_NONE)
        return;

    ret = (location_error_e)location_manager_destroy(m_location_manager);
    if (ret != LOCATIONS_ERROR_NONE)
        return;
}

void WebGeolocationClient::updatePosition()
{
    m_lastPosition = GeolocationPosition::create(m_timestamp, m_latitude, m_longitude, m_accuracy);
    if (m_page->corePage())
        GeolocationController::from(m_page->corePage())->positionChanged(m_lastPosition.get());
}

void WebGeolocationClient::positionChangedCallback(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
    WebGeolocationClient* client = (WebGeolocationClient*)user_data;
    client->m_timestamp = timestamp;
    client->m_latitude = latitude;
    client->m_longitude = longitude;
    client->m_altitude = altitude;
    client->updatePosition();
}

} // namespace WebKit

#endif // ENABLE(TIZEN_GEOLOCATION)
