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

#ifndef GeolocationProviderTizen_h
#define GeolocationProviderTizen_h

#if ENABLE(TIZEN_GEOLOCATION)

#include <WebKit2/WKContext.h>
#include <WebKit2/WKRetainPtr.h>
#include <location/locations.h>

namespace WebKit {

class GeolocationProviderTizen {
public:
    explicit GeolocationProviderTizen(WKContextRef);
    virtual ~GeolocationProviderTizen();

private:
    static void startUpdating(WKGeolocationManagerRef, const void*);
    static void stopUpdating(WKGeolocationManagerRef, const void*);

    static void didUpdateServiceState(location_service_state_e, void*);
    static void didChangePosition(double, double, double, time_t, void*);

    bool createLocationManager();
    bool startLocationManager();
    void stopLocationManager();
    void destroyLocationManager();

    WKRetainPtr<WKGeolocationManagerRef> m_manager;

    location_manager_h m_locationManager;
    bool m_started;
};

} // namespace WebKit

#endif // ENABLE(TIZEN_GEOLOCATION)

#endif // GeolocationProviderTizen_h
