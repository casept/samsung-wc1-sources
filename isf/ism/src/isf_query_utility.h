/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef __ISF_QUERY_UTILITY_H
#define __ISF_QUERY_UTILITY_H

using namespace scim;


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#define MAXLINE                         4096
#define USER_ENGINE_LIST_PATH           "/home/app/.scim"

#ifndef SCIM_SYSCONFDIR
  #define SCIM_SYSCONFDIR               "/usr/etc"
#endif

#define USER_ENGINE_FILE_NAME           (USER_ENGINE_LIST_PATH "/engines_list")
#define SYS_ENGINE_FILE_NAME            (SCIM_SYSCONFDIR "/scim/engines_list")


typedef struct {
    String name;
    String uuid;
    String module;
    String language;
    String icon;
    TOOLBAR_MODE_T mode;
    uint32 option;
    String locales;
} ISEINFO;

EAPI String isf_get_normalized_language (String src_str);
EAPI String isf_combine_ise_info_string (String name, String uuid, String module, String language,
                                         String icon, String mode, String option, String locales);
EAPI void isf_get_ise_info_from_string (const char *str, ISEINFO &info);
EAPI bool isf_read_ise_info_list (const char *filename, std::vector<ISEINFO> &info_list);
EAPI bool isf_write_ise_info_list (const char *filename, std::vector<ISEINFO> &info_list);
EAPI bool isf_add_keyboard_info_to_file (const char *filename, const char *module_name, const ConfigPointer &config);
EAPI bool isf_add_helper_info_to_file (const char *filename, const char *module_name);
EAPI bool isf_remove_ise_info_from_file (const char *filename, const char *module_name);
EAPI bool isf_remove_ise_info_from_file_by_uuid (const char *filename, const char *uuid);
EAPI void isf_update_ise_info_to_file (const char *filename, const ConfigPointer &config);

#endif /* __ISF_QUERY_UTILITY_H */

/*
vi:ts=4:ai:nowrap:expandtab
*/
