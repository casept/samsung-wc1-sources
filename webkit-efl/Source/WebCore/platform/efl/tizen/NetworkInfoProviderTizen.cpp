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
#include "NetworkInfoProviderTizen.h"
#include <wtf/UnusedParam.h>

#if ENABLE(TIZEN_NETWORK_INFO)

#include <math.h>
#include <telephony/telephony_network.h>

namespace WebCore {

static const double defaultBandwidth = -1.0;
static const double maxBandwidth = INFINITY;
static const double networkWifi  = 20.0; // 20 Mb/s
static const double networkSpeed2G = 60 / 1024; // 60 kb/s
static const double networkSpeedGPRS = 170 / 1024; // 170 kb/s
static const double networkSpeed3G = 7.0; // 7 Mb/s
static const double networkSpeedHSDPA  = 42.0; // 42 Mb/s

static void connectionTypeChangedCallback(connection_type_e type, void* userData)
{
}

static void connectionIPChangedCallback(const char* ipv4Address, const char* ipv6Address, void* userData)
{
}

static void connectionProxyChangedCallback(const char* ipv4Address, const char* ipv6Address, void* userData)
{
}

NetworkInfoProviderTizen::NetworkInfoProviderTizen()
    : m_connection(0)
{
}

void NetworkInfoProviderTizen::startUpdating()
{
    int error = connection_create(&m_connection);
    if (CONNECTION_ERROR_NONE == error) {
        connection_set_type_changed_cb(m_connection, connectionTypeChangedCallback, 0);
        connection_set_ip_address_changed_cb(m_connection, connectionIPChangedCallback, 0);
        connection_set_proxy_address_changed_cb(m_connection, connectionProxyChangedCallback, 0);
    } else
        LOG_ERROR("Client registration failed %d\n", error);
}

void NetworkInfoProviderTizen::stopUpdating()
{
    int error = connection_destroy(&m_connection);
    if (error != CONNECTION_ERROR_NONE)
        LOG_ERROR("Client deregistration fail [%d]\n", error);
}

double NetworkInfoProviderTizen::bandwidth() const
{
    connection_type_e networkType;
    int result = connection_get_type(m_connection, &networkType);
    if (result != CONNECTION_ERROR_NONE) {
        LOG_ERROR("Fail to get network state [%d]\n", result);
        return defaultBandwidth;
    }

    switch (networkType) {
    case CONNECTION_TYPE_DISCONNECTED:
        return defaultBandwidth;
    case CONNECTION_TYPE_WIFI:
        return networkWifi;
    case CONNECTION_TYPE_CELLULAR:
        return getCellularBandwidth();
    case CONNECTION_TYPE_ETHERNET :
        return maxBandwidth;
    default:
        ASSERT_NOT_REACHED();
    }

    return defaultBandwidth;
}

bool NetworkInfoProviderTizen::metered() const
{
    connection_type_e networkType;
    int result = connection_get_type(m_connection, &networkType);
    UNUSED_PARAM(result);

    if (networkType == CONNECTION_TYPE_CELLULAR)
        return true;

    return false;
}

double NetworkInfoProviderTizen::getCellularBandwidth() const
{
    network_info_type_e cellularType;
    int error = network_info_get_type(&cellularType);
    if (error != NETWORK_INFO_ERROR_NONE) {
        LOG_ERROR("Fail to get network state [%d]\n", error);
        return defaultBandwidth;
    }

    switch (cellularType) {
    case NETWORK_INFO_TYPE_UNKNOWN:
        return defaultBandwidth;
    case NETWORK_INFO_TYPE_GSM:
        return networkSpeed2G;
    case NETWORK_INFO_TYPE_GPRS:
    case NETWORK_INFO_TYPE_EDGE:
        return networkSpeedGPRS;
    case NETWORK_INFO_TYPE_UMTS:
        return networkSpeed3G;
    case NETWORK_INFO_TYPE_HSDPA:
        return networkSpeedHSDPA;
    default:
        ASSERT_NOT_REACHED();
        return defaultBandwidth;
    }
}

} // namespace WebCore

#endif // ENABLE(TIZEN_NETWORK_INFO)
