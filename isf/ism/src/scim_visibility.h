/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Li Zhang <li2012.zhang@samsung.com>
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
 
#ifndef __SCIM_VISIBILITY_H
#define __SCIM_VISIBILITY_H

#ifdef EAPI
# undef EAPI
#endif

#ifdef IAPI
# undef IAPI
#endif

#if defined _WIN32 || defined __CYGWIN__
#  ifdef ISF_BUILDING_DLL
#    ifdef __GNUC__
#      define EAPI __attribute__ ((dllexport))
#    else
#      define EAPI __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#    endif
#  else
#    ifdef __GNUC__
#      define EAPI __attribute__ ((dllimport))
#    else
#      define EAPI __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#    endif
#  endif
#  define IAPI
#else
#  if __GNUC__ >= 4
#    define EAPI __attribute__ ((visibility ("default")))
#    define IAPI  __attribute__ ((visibility ("hidden")))
#  else
#    define EAPI
#    define IAPI
#  endif
#endif

#endif
