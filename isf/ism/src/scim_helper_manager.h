/**
 * @file scim_helper_manager.h
 * @brief Defines scim::HelperManager.
 *
 * scim::HelperManager is a class used to manage all Client Helper modules.
 *
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2004-2005 James Su <suzhe@tsinghua.org.cn>
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * Modifications by Samsung Electronics Co., Ltd.
 * 1. Launch scim-launcher process when connection is failed
 * 2. Replace helper manager server with socket frontend
 * 3. Add some new interface APIs in HelperManager class
 *
 * $Id: scim_helper_manager.h,v 1.3 2005/05/17 06:45:15 suzhe Exp $
 */

#ifndef __SCIM_HELPER_MANAGER_H
#define __SCIM_HELPER_MANAGER_H

namespace scim {

/**
 * @addtogroup Helper
 * @ingroup InputServiceFramework
 * @{
 */

/**
 * @brief This class is used to manage all helper objects.
 *
 */
class EAPI HelperManager
{
    class HelperManagerImpl;
    HelperManagerImpl *m_impl;

    HelperManager (const HelperManager &);
    const HelperManager & operator = (const HelperManager &);

public:
    HelperManager ();
    ~HelperManager ();

    /**
     * @brief Get the total number of helpers supported by all helper modules.
     */
    unsigned int number_of_helpers (void) const;

    /**
     * @brief Get the information of a specific helper by its index.
     *
     * @param idx The index of the helper, must between 0 to number_of_helpers () - 1.
     * @param info The HelperInfo object to store the information.
     * @return true if this helper is ok and the information is stored correctly.
     */
    bool get_helper_info (unsigned int idx, HelperInfo &info) const;

    /**
     * @brief Get the connection id.
     *
     * @return the connection id
     */
    int  get_connection_number  (void) const;

    /**
     * @brief Check if there are any events available to be processed.
     *
     * If it returns true then HelperManager object should call
     * HelperManager::filter_event () to process them.
     *
     * @return true if there are any events available.
     */
    bool has_pending_event      (void) const;

    /**
     * @brief Process the pending events.
     *
     * This function will emit the corresponding signals according
     * to the events.
     *
     * @return false if the connection is broken, otherwise return true.
     */
    bool filter_event           (void);

    /**
     * @brief Run a specific helper.
     *
     * The helper will run in a newly forked process, so this function will return as soon
     * as the new process is launched.
     *
     * @param config_name The name of the ConfigModule to be used to read configurations.
     * @param uuid The UUID of the helper to be run.
     * @param display The display in which the helper will be run.
     */
    void run_helper (const String &uuid, const String &config_name, const String &display) const;
    void stop_helper (const String &name) const;
    void get_helper_list (void) const;

    int get_active_ise_list (std::vector<String> &strlist) const;
    int send_display_name (String &name) const;
    int send_ise_list (String &ise_list) const;
    int turn_on_log (uint32 isOn) const;

    void preload_keyboard_ise (const String &uuid) const;
};

/** @} */

} // namespace scim

#endif //__SCIM_HELPER_MANAGER_H

/*
vi:ts=4:nowrap:ai:expandtab
*/

