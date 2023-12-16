/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2002-2005 James Su <suzhe@tsinghua.org.cn>
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
 * 1. Add time and logs functions for performance profile
 *
 * $Id: scim_utility.cpp,v 1.48.2.5 2006/11/02 04:11:51 suzhe Exp $
 */

#define Uses_SCIM_UTILITY
#define Uses_SCIM_CONFIG_PATH
#define Uses_C_LOCALE
#define Uses_C_ICONV
#define Uses_C_STDLIB
#define Uses_C_STRING

#include <langinfo.h>
#include <pwd.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/times.h>
#include <dlog.h>

#include "scim_private.h"
#include "scim.h"

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG     "ISF_UTILITY"

namespace scim {

EAPI int
utf8_mbtowc (ucs4_t *pwc, const unsigned char *src, int src_len)
{
    if (!pwc)
        return 0;

    unsigned char c = src [0];

    if (c < 0x80) {
        *pwc = c;
        return 1;
    } else if (c < 0xc2) {
        return RET_ILSEQ;
    } else if (c < 0xe0) {
        if (src_len < 2)
            return RET_TOOFEW(0);
        unsigned char utf8_x1 = src [1] ^ 0x80;
        if (utf8_x1 >= 0x40)
            return RET_ILSEQ;
        *pwc = ((ucs4_t) (c & 0x1f) << 6)
                 | (ucs4_t) utf8_x1;
        return 2;
    } else if (c < 0xf0) {
        if (src_len < 3)
            return RET_TOOFEW(0);
        unsigned char utf8_x1 = src[1] ^ 0x80;
        unsigned char utf8_x2 = src[2] ^ 0x80;
        if (!((utf8_x1 | utf8_x2) < 0x40
                && (c >= 0xe1 || src [1] >= 0xa0)))
            return RET_ILSEQ;
        *pwc = ((ucs4_t) (c & 0x0f) << 12)
                 | ((ucs4_t) (utf8_x1) << 6)
                 | (ucs4_t) (utf8_x2);
        return 3;
    } else if (c < 0xf8) {
        if (src_len < 4)
            return RET_TOOFEW(0);
        unsigned char utf8_x1 = src[1] ^ 0x80;
        unsigned char utf8_x2 = src[2] ^ 0x80;
        unsigned char utf8_x3 = src[3] ^ 0x80;
        if (!((utf8_x1 | utf8_x2 | utf8_x3) < 0x40
                && (c >= 0xf1 || src [1] >= 0x90)))
            return RET_ILSEQ;
        *pwc = ((ucs4_t) (c & 0x07) << 18)
                 | ((ucs4_t) (utf8_x1) << 12)
                 | ((ucs4_t) (utf8_x2) << 6)
                 | (ucs4_t) (utf8_x3);
        return 4;
    } else if (c < 0xfc) {
        if (src_len < 5)
            return RET_TOOFEW(0);
        unsigned char utf8_x1 = src[1] ^ 0x80;
        unsigned char utf8_x2 = src[2] ^ 0x80;
        unsigned char utf8_x3 = src[3] ^ 0x80;
        unsigned char utf8_x4 = src[4] ^ 0x80;
        if (!((utf8_x1 | utf8_x2 | utf8_x3 | utf8_x4) < 0x40
                && (c >= 0xf9 || src [1] >= 0x88)))
            return RET_ILSEQ;
        *pwc = ((ucs4_t) (c & 0x03) << 24)
                 | ((ucs4_t) (utf8_x1) << 18)
                 | ((ucs4_t) (utf8_x2) << 12)
                 | ((ucs4_t) (utf8_x3) << 6)
                 | (ucs4_t) (utf8_x4);
        return 5;
    } else if (c < 0xfe) {
        if (src_len < 6)
            return RET_TOOFEW(0);
        unsigned char utf8_x1 = src[1] ^ 0x80;
        unsigned char utf8_x2 = src[2] ^ 0x80;
        unsigned char utf8_x3 = src[3] ^ 0x80;
        unsigned char utf8_x4 = src[4] ^ 0x80;
        unsigned char utf8_x5 = src[5] ^ 0x80;
        if (!((utf8_x1 | utf8_x2 | utf8_x3 | utf8_x4 | utf8_x5) < 0x40
                && (c >= 0xfd || src [1] >= 0x84)))
            return RET_ILSEQ;
        *pwc = ((ucs4_t) (c & 0x01) << 30)
                 | ((ucs4_t) (utf8_x1) << 24)
                 | ((ucs4_t) (utf8_x2) << 18)
                 | ((ucs4_t) (utf8_x3) << 12)
                 | ((ucs4_t) (utf8_x4) << 6)
                 | (ucs4_t) (utf8_x5);
        return 6;
    } else
        return RET_ILSEQ;
}

EAPI int
utf8_wctomb (unsigned char *dest, ucs4_t wc, int dest_size)
{
    if (!dest)
        return 0;

    int count;
    if (wc < 0x80) { // most offen case
        if (dest_size < 1)
            return RET_TOOSMALL;
        dest[0] = wc;
        return 1;
    }
    else if (wc < 0x800)
        count = 2;
    else if (wc < 0x10000)
        count = 3;
    else if (wc < 0x200000)
        count = 4;
    else if (wc < 0x4000000)
        count = 5;
    else if (wc <= 0x7fffffff)
        count = 6;
    else
        return RET_ILSEQ;
    if (dest_size < count)
        return RET_TOOSMALL;
    switch (count) { /* note: code falls through cases! */
        case 6: dest [5] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x4000000;
        case 5: dest [4] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x200000;
        case 4: dest [3] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x10000;
        case 3: dest [2] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x800;
        case 2: dest [1] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0xc0;
        case 1: dest [0] = wc;
    }
    return count;
}

EAPI ucs4_t
utf8_read_wchar (std::istream &is)
{
    unsigned char utf8[5] = {0};
    unsigned char utf8_x_;

    is.read ((char*)(&utf8_x_), sizeof(unsigned char)); // reducing utf8 char sequence to ucs4 convertations
    if (utf8_x_ < 0x80) {
        return (ucs4_t)utf8_x_;
    } else if (utf8_x_ < 0xc2) {
        return 0;
    } else if (utf8_x_ < 0xe0) {
        unsigned char utf8_x1;
        is.read ((char*)(&utf8_x1), sizeof(unsigned char));
        utf8_x1 ^= 0x80;
        if (utf8_x1 >= 0x40)
            return 0;
        return ((ucs4_t) (utf8_x_ & 0x1f) << 6)
              | (ucs4_t) (utf8_x1);
    } else if (utf8_x_ < 0xf0) {
        is.read ((char*)(&utf8[0]), 2*sizeof(unsigned char));
        unsigned char utf8_x1 = utf8[0] ^ 0x80;
        unsigned char utf8_x2 = utf8[1] ^ 0x80;
        if (!((utf8_x1 | utf8_x2) < 0x40
           && (utf8_x_ >= 0xe1 || utf8[0] >= 0xa0)))
            return 0;
        return ((ucs4_t) (utf8_x_ & 0x0f) << 12)
             | ((ucs4_t) (utf8_x1) << 6)
             |  (ucs4_t) (utf8_x2);
    } else if (utf8_x_ < 0xf8) {
        is.read ((char*)(&utf8[0]), 3*sizeof(unsigned char));
        unsigned char utf8_x1 = utf8[0] ^ 0x80;
        unsigned char utf8_x2 = utf8[1] ^ 0x80;
        unsigned char utf8_x3 = utf8[2] ^ 0x80;
        if (!((utf8_x1 | utf8_x2 | utf8_x3) < 0x40
           && (utf8_x_ >= 0xf1 || utf8[0] >= 0x90)))
            return 0;
        return ((ucs4_t) (utf8_x_ & 0x07) << 18)
             | ((ucs4_t) (utf8_x1) << 12)
             | ((ucs4_t) (utf8_x2) << 6)
             |  (ucs4_t) (utf8_x3);
    } else if (utf8_x_ < 0xfc) {
        is.read ((char*)(&utf8[0]), 4*sizeof(unsigned char));
        unsigned char utf8_x1 = utf8[0] ^ 0x80;
        unsigned char utf8_x2 = utf8[1] ^ 0x80;
        unsigned char utf8_x3 = utf8[2] ^ 0x80;
        unsigned char utf8_x4 = utf8[3] ^ 0x80;
        if (!((utf8_x1 | utf8_x2 | utf8_x3 | utf8_x4) < 0x40
           && (utf8_x_ >= 0xf9 || utf8[0] >= 0x88)))
            return 0;
        return ((ucs4_t) (utf8_x_ & 0x03) << 24)
             | ((ucs4_t) (utf8_x1) << 18)
             | ((ucs4_t) (utf8_x2) << 12)
             | ((ucs4_t) (utf8_x3) << 6)
             |  (ucs4_t) (utf8_x4);
    } else if (utf8_x_ < 0xfe) {
        is.read ((char*)(&utf8[0]), 5*sizeof(unsigned char));
        unsigned char utf8_x1 = utf8[0] ^ 0x80;
        unsigned char utf8_x2 = utf8[1] ^ 0x80;
        unsigned char utf8_x3 = utf8[2] ^ 0x80;
        unsigned char utf8_x4 = utf8[3] ^ 0x80;
        unsigned char utf8_x5 = utf8[4] ^ 0x80;
        if (!((utf8_x1 | utf8_x2 | utf8_x3 | utf8_x4 | utf8_x5) < 0x40
           && (utf8_x_ >= 0xfd || utf8[0] >= 0x84)))
            return 0;
        return ((ucs4_t) (utf8_x_ & 0x01) << 30)
             | ((ucs4_t) (utf8_x1) << 24)
             | ((ucs4_t) (utf8_x2) << 18)
             | ((ucs4_t) (utf8_x3) << 12)
             | ((ucs4_t) (utf8_x4) << 6)
             |  (ucs4_t) (utf8_x5);
    }
    return 0;
}

EAPI WideString
utf8_read_wstring (std::istream &is, ucs4_t delim, bool rm_delim)
{
    WideString str;
    ucs4_t wc;
    while ((wc = utf8_read_wchar (is)) > 0) {
        if (wc != delim)
            str.push_back (wc);
        else {
            if (!rm_delim)
                str.push_back (wc);
            break;
        }
    }
    return str;
}

EAPI std::ostream &
utf8_write_wchar (std::ostream &os, ucs4_t wc)
{
    unsigned char utf8[6];
    int count = 0;

    if ((count=utf8_wctomb (utf8, wc, 6)) > 0)
        os.write ((char*)utf8, count * sizeof (unsigned char));

    return os;
}

EAPI std::ostream &
utf8_write_wstring (std::ostream &os, const WideString & wstr)
{
    for (unsigned int i=0; i<wstr.size (); ++i)
        utf8_write_wchar (os, wstr [i]);

    return os;
}

EAPI WideString
utf8_mbstowcs (const String & str)
{
    WideString wstr;
    ucs4_t wc;
    unsigned int sn = 0;
    int un = 0;

    const unsigned char *s = (const unsigned char *) str.c_str ();

    while (sn < str.length () && *s != 0 &&
            (un=utf8_mbtowc (&wc, s, str.length () - sn)) > 0) {
        wstr.push_back (wc);
        s += un;
        sn += un;
    }
    return wstr;
}

EAPI WideString
utf8_mbstowcs (const char *str, int len)
{
    WideString wstr;

    if (str) {
        ucs4_t wc;
        unsigned int sn = 0;
        int un = 0;

        if (len < 0) len = strlen (str);

        while (sn < ((unsigned int)len) && *str != 0 && (un=utf8_mbtowc (&wc, (const unsigned char *)str, len - sn)) > 0) {
            wstr.push_back (wc);
            str += un;
            sn += un;

        }
    }
    return wstr;
}

EAPI String
utf8_wcstombs (const WideString & wstr)
{
    String str;
    char utf8 [6];
    int un = 0;

    for (unsigned int i = 0; i<wstr.size (); ++i) {
        un = utf8_wctomb ((unsigned char*)utf8, wstr [i], 6);
        if (un > 0)
            str.append (utf8, un);
    }
    return str;
}

EAPI String
utf8_wcstombs (const ucs4_t *wstr, int len)
{
    String str;
    char utf8 [6];
    int un = 0;

    if (wstr) {
        if (len < 0) {
            len = 0;
            while (wstr [len])
                ++len;
        }

        for (int i = 0; i < len; ++i) {
            un = utf8_wctomb ((unsigned char*)utf8, wstr [i], 6);
            if (un > 0)
                str.append (utf8, un);
        }
    }
    return str;
}

static String trim_blank (const String &str)
{
    String::size_type begin, len;

    begin = str.find_first_not_of (" \t\n\v");

    if (begin == String::npos)
        return String ();

    len = str.find_last_not_of (" \t\n\v") - begin + 1;

    return str.substr (begin, len);
}

EAPI String
scim_validate_locale (const String& locale)
{
    String good;

    String last = String (setlocale (LC_CTYPE, 0));

    if (setlocale (LC_CTYPE, locale.c_str ())) {
        good = locale;
    } else {
        std::vector<String> vec;
        if (scim_split_string_list (vec, locale, '.') == 2) {
            if (isupper (vec[1][0])) {
                for (String::iterator i=vec[1].begin (); i!=vec[1].end (); ++i)
                    *i = (char) tolower (*i);
            } else {
                for (String::iterator i=vec[1].begin (); i!=vec[1].end (); ++i)
                    *i = (char) toupper (*i);
            }
            if (setlocale (LC_CTYPE, (vec[0] + "." + vec[1]).c_str ())) {
                good = vec [0] + "." + vec[1];
            }
        }
    }

    setlocale (LC_CTYPE, last.c_str ());

    return good;
}

EAPI String
scim_get_locale_encoding (const String& locale)
{
    String last = String (setlocale (LC_CTYPE, 0));
    String encoding;

    if (setlocale (LC_CTYPE, locale.c_str ()))
        encoding = String (nl_langinfo (CODESET));
    else {
        std::vector<String> vec;
        if (scim_split_string_list (vec, locale, '.') == 2) {
            if (isupper (vec[1][0])) {
                for (String::iterator i=vec[1].begin (); i!=vec[1].end (); ++i)
                    *i = (char) tolower (*i);
            } else {
                for (String::iterator i=vec[1].begin (); i!=vec[1].end (); ++i)
                    *i = (char) toupper (*i);
            }
            if (setlocale (LC_CTYPE, (vec[0] + "." + vec[1]).c_str ()))
                encoding = String (nl_langinfo (CODESET));
        }

    }

    setlocale (LC_CTYPE, last.c_str ());

    return encoding;
}

EAPI int
scim_get_locale_maxlen (const String& locale)
{
    int maxlen;

    String last = String (setlocale (LC_CTYPE, 0));

    if (setlocale (LC_CTYPE, locale.c_str ()))
        maxlen = MB_CUR_MAX;
    else
        maxlen = 1;

    setlocale (LC_CTYPE, last.c_str ());
    return maxlen;
}

EAPI int
scim_split_string_list (std::vector<String>& vec, const String& str, char delim)
{
    int count = 0;

    String temp;
    String::const_iterator bg, ed;

    vec.clear ();

    if (str.empty ()) return 0;

    bg = str.begin ();
    ed = str.begin ();

    while (bg != str.end () && ed != str.end ()) {
        for (; ed != str.end (); ++ed) {
            if (*ed == delim)
                break;
        }
        temp.assign (bg, ed);
        temp = trim_blank (temp);
        vec.push_back (temp);
        ++count;

        if (ed != str.end ())
            bg = ++ ed;
    }
    return count;
}

EAPI String
scim_combine_string_list (const std::vector<String>& vec, char delim)
{
    String result;
    for (std::vector<String>::const_iterator i = vec.begin (); i!=vec.end (); ++i) {
        result += *i;
        if (i+1 != vec.end ())
            result += delim;
    }
    return result;
}

EAPI bool
scim_if_wchar_ucs4_equal ()
{
    if (sizeof (wchar_t) != sizeof (ucs4_t))
        return false;

    iconv_t cd;
    wchar_t wcbuf [2] = {0,0};
    ucs4_t  ucsbuf [2] = {0x4E00, 0x0001};
    size_t  wclen = 2 * sizeof (wchar_t);
    size_t  ucslen = 2 * sizeof (ucs4_t);

    char *wcp = (char *) wcbuf;
    ICONV_CONST char *ucsp = (ICONV_CONST char *) ucsbuf;

    if (scim_is_little_endian ())
        cd = iconv_open ("UCS-4LE", "wchar_t");
    else
        cd = iconv_open ("UCS-4BE", "wchar_t");

    if (cd == (iconv_t) -1)
        return false;

    iconv (cd, &ucsp, &ucslen, &wcp, &wclen);

    iconv_close (cd);

    if (wcbuf [0] == (wchar_t) ucsbuf [0] &&
        wcbuf [1] == (wchar_t) ucsbuf [1])
        return true;

    return false;
}

static struct {
    ucs4_t half;
    ucs4_t full;
    ucs4_t size;
} __half_full_table [] = {
    {0x0020, 0x3000, 1},
    {0x0021, 0xFF01, 0x5E},
    {0x00A2, 0xFFE0, 2},
    {0x00A5, 0xFFE5, 1},
    {0x00A6, 0xFFE4, 1},
    {0x00AC, 0xFFE2, 1},
    {0x00AF, 0xFFE3, 1},
    {0x20A9, 0xFFE6, 1},
    {0xFF61, 0x3002, 1},
    {0xFF62, 0x300C, 2},
    {0xFF64, 0x3001, 1},
    {0xFF65, 0x30FB, 1},
    {0xFF66, 0x30F2, 1},
    {0xFF67, 0x30A1, 1},
    {0xFF68, 0x30A3, 1},
    {0xFF69, 0x30A5, 1},
    {0xFF6A, 0x30A7, 1},
    {0xFF6B, 0x30A9, 1},
    {0xFF6C, 0x30E3, 1},
    {0xFF6D, 0x30E5, 1},
    {0xFF6E, 0x30E7, 1},
    {0xFF6F, 0x30C3, 1},
    {0xFF70, 0x30FC, 1},
    {0xFF71, 0x30A2, 1},
    {0xFF72, 0x30A4, 1},
    {0xFF73, 0x30A6, 1},
    {0xFF74, 0x30A8, 1},
    {0xFF75, 0x30AA, 2},
    {0xFF77, 0x30AD, 1},
    {0xFF78, 0x30AF, 1},
    {0xFF79, 0x30B1, 1},
    {0xFF7A, 0x30B3, 1},
    {0xFF7B, 0x30B5, 1},
    {0xFF7C, 0x30B7, 1},
    {0xFF7D, 0x30B9, 1},
    {0xFF7E, 0x30BB, 1},
    {0xFF7F, 0x30BD, 1},
    {0xFF80, 0x30BF, 1},
    {0xFF81, 0x30C1, 1},
    {0xFF82, 0x30C4, 1},
    {0xFF83, 0x30C6, 1},
    {0xFF84, 0x30C8, 1},
    {0xFF85, 0x30CA, 6},
    {0xFF8B, 0x30D2, 1},
    {0xFF8C, 0x30D5, 1},
    {0xFF8D, 0x30D8, 1},
    {0xFF8E, 0x30DB, 1},
    {0xFF8F, 0x30DE, 5},
    {0xFF94, 0x30E4, 1},
    {0xFF95, 0x30E6, 1},
    {0xFF96, 0x30E8, 6},
    {0xFF9C, 0x30EF, 1},
    {0xFF9D, 0x30F3, 1},
    {0xFFA0, 0x3164, 1},
    {0xFFA1, 0x3131, 30},
    {0xFFC2, 0x314F, 6},
    {0xFFCA, 0x3155, 6},
    {0xFFD2, 0x315B, 9},
    {0xFFE9, 0x2190, 4},
    {0xFFED, 0x25A0, 1},
    {0xFFEE, 0x25CB, 1},
    {0,0,0}
};


/**
 * convert a half width unicode char to full width char
 */
EAPI ucs4_t
scim_wchar_to_full_width (ucs4_t code)
{
    int i=0;
    while (__half_full_table [i].size) {
        if (code >= __half_full_table [i].half &&
            code <  __half_full_table [i].half +
                    __half_full_table [i].size)
            return __half_full_table [i].full +
                    (code - __half_full_table [i].half);
        ++ i;
    }
    return code;
}

/**
 * convert a full width unicode char to half width char
 */
EAPI ucs4_t
scim_wchar_to_half_width (ucs4_t code)
{
    int i=0;
    while (__half_full_table [i].size) {
        if (code >= __half_full_table [i].full &&
            code <  __half_full_table [i].full +
                    __half_full_table [i].size)
            return __half_full_table [i].half +
                    (code - __half_full_table [i].full);
        ++ i;
    }
    return code;
}

EAPI String
scim_get_home_dir ()
{
    const char * home_dir = 0;

    struct passwd *pw;

    setpwent ();
    pw = getpwuid (getuid ());
    endpwent ();

    if (pw) {
        home_dir = pw->pw_dir;
    }

    if (!home_dir) {
        home_dir = getenv ("HOME");
    }

    if (home_dir)
        return String (home_dir);
    else
        return String ("");
}

EAPI String
scim_get_user_name ()
{
    struct passwd *pw;
    const char *user_name;

    setpwent ();
    pw = getpwuid (getuid ());
    endpwent ();

    if (pw && pw->pw_name)
        return String (pw->pw_name);
    else if ((user_name = getenv ("USER")) != NULL)
        return String (user_name);

    char uid_str [10];

    snprintf (uid_str, 10, "%u", getuid ());

    return String (uid_str);
}

EAPI String
scim_get_user_data_dir ()
{
    String dir = scim_get_home_dir () + String ("/.scim");
    scim_make_dir (dir);
    return dir;
}

EAPI String
scim_get_current_locale ()
{
    char *locale = setlocale (LC_CTYPE, 0);

    if (locale) return String (locale);
    return String ();
}

EAPI String scim_get_current_language ()
{
    return scim_get_locale_language (scim_get_current_locale ());
}

EAPI bool
scim_is_little_endian ()
{
    short endian = 1;
    return (*((char *)&endian) != 0);
}

EAPI size_t
scim_load_file (const String &filename, char **bufptr)
{
    if (!filename.length ())
        return 0;

    struct stat statbuf;

    if (stat (filename.c_str (), &statbuf) < 0 ||
        !S_ISREG (statbuf.st_mode) ||
        !statbuf.st_size)
        return 0;

    if (!bufptr)
        return statbuf.st_size;

    FILE *fp = fopen (filename.c_str (), "r");

    if (fp == NULL) {
        *bufptr = 0;
        return 0;
    }

    try {
        *bufptr = new char [statbuf.st_size];
    } catch (...) {
        fclose (fp);
        throw;
    }

    if (! (*bufptr)) {
        fclose (fp);
        return 0;
    }

    size_t size = fread (*bufptr, 1, statbuf.st_size, fp);

    fclose (fp);

    if (!size) {
        delete [] *bufptr;
        *bufptr = 0;
    }

    return size;
}

EAPI bool
scim_make_dir (const String &dir)
{
    std::vector <String> paths;
    String path;

    scim_split_string_list (paths, dir, SCIM_PATH_DELIM);

    for (size_t i = 0; i < paths.size (); ++i) {
        path += SCIM_PATH_DELIM_STRING + paths [i];

        //Make the dir if it's not exist.
        if (access (path.c_str (), R_OK) != 0) {
            if (mkdir (path.c_str (), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0)
                return false;

            if (access (path.c_str (), R_OK) != 0)
                return false;
        }
    }
    return true;
}

struct __Language {
    const char *code;
    const char *normalized;
    const char *name;
    const char *untranslated;
    const char *locale_suffix;
};

static __Language __languages [] = {
    { "C",        NULL, N_("English Keyboard"), NULL, NULL},
    { "am_ET",    NULL, N_("Amharic"), "አማርኛ", NULL },
    { "ar",       "ar", N_("Arabic"), "عربي", NULL },
    { "ar_EG",    NULL, N_("Arabic (Egypt)"), "العربية (مصر)", NULL },
    { "ar_LB",    NULL, N_("Arabic (Lebanon)"), "العربية (لبنان)", NULL },
    { "ar_IL",    NULL, N_("Arabic (Israel)"), "العربية", NULL },
    { "as_IN",    NULL, N_("Assamese"), "অসমীয়া", NULL},
    { "az_AZ",    NULL, N_("Azerbaijani"), "Azərbaycan", NULL },
    { "be_BY",    NULL, N_("Belarusian"), "Беларуская мова", NULL },
    { "bg_BG",    NULL, N_("Bulgarian"), "Български", NULL },
    { "bn",    "bn_BD", N_("Bengali"), "বাংলা", NULL },
    { "bn_BD",    NULL, N_("Bengali"), "বাংলা", NULL },
    { "bn_IN",    NULL, N_("Bengali (India)"), "বাংলা", NULL },
    { "bo",       NULL, N_("Tibetan"), "པོད་སྐད་", NULL },
    { "bs_BA",    NULL, N_("Bosnian"), "Bosanski", NULL },
    { "ca_ES",    NULL, N_("Catalan"), "Català", "@euro" },
    { "cs_CZ",    NULL, N_("Czech"), "čeština", NULL },
    { "cy_GB",    NULL, N_("Welsh"), "Cymraeg", NULL },
    { "da_DK",    NULL, N_("Danish"), "Dansk", "@euro" },
    { "de_DE",    NULL, N_("German"), "Deutsch", "@euro" },
    { "dv_MV",    NULL, N_("Divehi"), "ދިވެހިބަސް", NULL },
    { "el_GR",    NULL, N_("Greek"), "ελληνικά", NULL },
    { "en"   ,    NULL, N_("English"), "English", NULL },
    { "en_AU",    NULL, N_("English (Australian)"), "English (Australian)", NULL },
    { "en_CA",    NULL, N_("English (Canadian)"), "English (Canadian)", NULL },
    { "en_GB",    NULL, N_("English (United Kingdom)"), "English (United Kingdom)", ".iso885915" },
    { "en_IE",    NULL, N_("English (Ireland)"), "English (Ireland)", NULL },
    { "en_US",    NULL, N_("English (United States)"), "English (United States)", ".iso885915" },
    { "eo",       NULL, N_("Esperanto"), "Esperanto", NULL },
    { "es",    "es_ES", N_("Spanish"), "Español", NULL },
    { "es_ES",    NULL, N_("Spanish"), "Español", "@euro" },
    { "es_MX",    NULL, N_("Spanish (Mexico)"), "Español (Mexico)", NULL },
    { "es_US",    NULL, N_("Spanish (United States)"), "Español (United States)", NULL },
    { "et_EE",    NULL, N_("Estonian"), "Eesti", ".iso885915" },
    { "eu_ES",    NULL, N_("Basque"), "Euskara", "@euro" },
    { "fa_IR",    NULL, N_("Persian"), "فارسی", NULL },
    { "fi_FI",    NULL, N_("Finnish"), "Suomi", "@euro" },
    { "fr_FR",    NULL, N_("French"), "Français", "@euro" },
    { "ga_IE",    NULL, N_("Irish"), "Gaeilge", "@euro" },
    { "gl_ES",    NULL, N_("Galician"), "Galego", "@euro" },
    { "gu_IN",    NULL, N_("Gujarati"), "ગુજરાતી", NULL },
    { "he_IL",    NULL, N_("Hebrew"), "עברית", NULL },
    { "hi_IN",    NULL, N_("Hindi"), "हिंदी", NULL },
    { "hr_HR",    NULL, N_("Croatian"), "Hrvatski", NULL },
    { "hu_HU",    NULL, N_("Hungarian"), "Magyar", NULL },
    { "hy_AM",    NULL, N_("Armenian"), "Հայերէն", NULL },
    { "ia"   ,    NULL, N_("Interlingua"), "Interlingua", NULL },
    { "id_ID",    NULL, N_("Indonesian"), "Bahasa Indonesia", NULL },
    { "is_IS",    NULL, N_("Icelandic"), "íslenska", NULL },
    { "it_IT",    NULL, N_("Italian"), "Italiano", "@euro" },
    { "iw_IL",    NULL, N_("Hebrew"), "עברית", NULL },
    { "ja_JP",    NULL, N_("Japanese"), "日本語", ".EUC-JP,.SJIS,.eucJP" },
    { "ka_GE",    NULL, N_("Georgian"), "ქართული", NULL },
    { "kk_KZ",    NULL, N_("Kazakh"), "Қазақ", NULL },
    { "km",       NULL, N_("Cambodian"), "ភាសាខ្មែរ", NULL },
    { "kn_IN",    NULL, N_("Kannada"), "﻿ಕನ್ನಡ", NULL },
    { "ko_KR",    NULL, N_("Korean"), "한국어", ".EUC-KR,.eucKR" },
    { "lo",       NULL, N_("Lao"), "لاوس", NULL },
    { "lo_LA",    NULL, N_("Laothian"), "ລາວ", NULL },
    { "lt_LT",    NULL, N_("Lithuanian"), "Lietuvių", NULL },
    { "lv_LV",    NULL, N_("Latvian"), "Latviešu", NULL },
    { "mk_MK",    NULL, N_("Macedonian"), "Македонски", NULL },
    { "ml_IN",    NULL, N_("Malayalam"), "മലയാളം", NULL },
    { "mn_MN",    NULL, N_("Mongolian"), "Монгол", NULL },
    { "mr_IN",    NULL, N_("Marathi"), "मराठी", NULL },
    { "ms_MY",    NULL, N_("Malay"), "Bahasa Melayu", NULL },
    { "my_MM",    NULL, N_("Burmese"), "မြန်မာဘာသာ", NULL },
    { "ne_NP",    NULL, N_("Nepali"), "नेपाली", NULL },
    { "nl_NL",    NULL, N_("Dutch"), "Nederlands", "@euro" },
    { "nn_NO",    NULL, N_("Norwegian (Nynorsk)"), "Norsk (Nynorsk)", NULL },
    { "nb_NO",    NULL, N_("Norwegian (Bokmal)"), "Norsk (Bokmål)", NULL },
    { "or_IN",    NULL, N_("Oriya"), "ଓଡ଼ିଆ", NULL },
    { "pa_IN",    NULL, N_("Punjabi"), "ਪੰਜਾਬੀ", NULL },
    { "pl_PL",    NULL, N_("Polish"), "Polski", NULL },
    { "pt",    "pt_PT", N_("Portuguese"), "Português", NULL },
    { "pt_BR",    NULL, N_("Portuguese (Brazil)"), "Português (Brasil)", NULL },
    { "pt_PT",    NULL, N_("Portuguese"), "Português", "@euro" },
    { "ro_RO",    NULL, N_("Romanian"), "Română", NULL },
    { "ru_RU",    NULL, N_("Russian"), "русский", ".koi8r" },
    { "sd",    "sd_IN", N_("Sindhi"), "ﺲﻧڌﻱ", NULL },
    { "sd_IN",    NULL, N_("Sindhi"), "सिन्धी", "@devanagari" },
    { "si_IN",    NULL, N_("Sinhala"), "සිංහල", NULL },
    { "si_LK",    NULL, N_("Sinhala"), "සිංහල", NULL },
    { "sk_SK",    NULL, N_("Slovak"), "Slovenský", NULL },
    { "sl_SI",    NULL, N_("Slovenian"), "Slovenščina", NULL },
    { "sq_AL",    NULL, N_("Albanian"), "Shqip", NULL },
    { "sr",    "sr_YU", N_("Serbian"), "Srpski", NULL },
    { "sr_CS",    NULL, N_("Serbian"), "Srpski", NULL },
    { "sr_YU",    NULL, N_("Serbian"), "Srpski", "@cyrillic" },
    { "sv",    "sv_SE", N_("Swedish"), "Svenska", NULL },
    { "sv_FI",    NULL, N_("Swedish (Finland)"), "Svenska (Finland)", "@euro" },
    { "sv_SE",    NULL, N_("Swedish"), "Svenska", ".iso885915" },
    { "ta_IN",    NULL, N_("Tamil"), "தமிழ்", NULL },
    { "te_IN",    NULL, N_("Telugu"), "తెలుగు", NULL },
    { "th_TH",    NULL, N_("Thai"), "ภาษาไทย", NULL },
    { "tr_TR",    NULL, N_("Turkish"), "Türkçe", NULL },
    { "ug",       NULL, N_("Uighur"), "ئۇيغۇر", NULL },
    { "uk_UA",    NULL, N_("Ukrainian"), "Українська", NULL },
    { "ur_PK",    NULL, N_("Urdu"), "اردو", NULL },
    { "uz_UZ",    NULL, N_("Uzbek"), "o'zbek tili", "@cyrillic" },
    { "vi_VN",    NULL, N_("Vietnamese"), "Tiếng Việt", ".tcvn" },
    { "wa_BE",    NULL, N_("Walloon"), "Walon", "@euro" },
    { "yi"   , "yi_US", N_("Yiddish"), "ייִדיש", NULL },
    { "yi_US",    NULL, N_("Yiddish"), "ייִדיש", NULL },
    { "zh",    "zh_CN", N_("Chinese"), "中文", NULL },
    { "zh_CN",    NULL, N_("Chinese (Simplified)"), "中文 (简体)", ".GB18030,.GBK,.GB2312,.eucCN" },
    { "zh_HK", NULL, N_("Chinese (Hongkong)"), "中文 (香港)", NULL },
    { "zh_SG", "zh_CN", N_("Chinese (Singapore)"), "中文 (新加坡)", ".GBK" },
    { "zh_TW",    NULL, N_("Chinese (Traditional_Taiwan)"), "中文 (台湾)", ".eucTW" },

    { "nl_BE",    NULL, N_("Dutch (Belgian)"), "Dutch (Belgian)", NULL },
    { "en_NZ",    NULL, N_("English (New Zealand)"), "English (New Zealand)", NULL },
    { "en_ZA",    NULL, N_("English (South Africa)"), "English (South Africa)", NULL },
    { "en_JM",    NULL, N_("English (Jamaica)"), "English (Jamaica)", NULL },
    { "en_BZ",    NULL, N_("English (Belize)"), "English (Belize)", NULL },
    { "en_TT",    NULL, N_("English (Trinidad)"), "English (Trinidad)", NULL },
    { "en_ZW",    NULL, N_("English (Zimbabwe)"), "English (Zimbabwe)", NULL },
    { "en_PH",    NULL, N_("English (Philippines)"), "English (Philippines)", NULL },
    { "fr_BE",    NULL, N_("French (Belgian)"), "Français (Belgian)", NULL },
    { "fr_CA",    NULL, N_("French (Canadian)"), "Français (Canada)", NULL },
    { "fr_CH",    NULL, N_("French (Swiss)"), "Français (Suisse)", NULL },
    { "fr_LU",    NULL, N_("French (Luxembourg)"), "Français (Luxembourg)", NULL },
    { "fr_MC",    NULL, N_("French (Monaco)"), "Français (Monaco)", NULL },
    { "de_CH",    NULL, N_("German (Swiss)"), "Deutsch (Schweiz)", NULL },
    { "de_AT",    NULL, N_("German (Austrian)"), "Deutsch (Österreich)", NULL },
    { "de_LU",    NULL, N_("German (Luxembourg)"), "Deutsch (Luxembourg)", NULL },
    { "de_LI",    NULL, N_("German (Liechtenstein)"), "Deutsch (Liechtenstein)", NULL },
    { "it_CH",    NULL, N_("Italian (Swiss)"), "italiano (Svizzera)", NULL },
    { "es_GT",    NULL, N_("Spanish (Guatemala)"), "español (Guatemala)", NULL },
    { "es_CR",    NULL, N_("Spanish (Costa Rica)"), "español (Costa Rica)", NULL },
    { "es_PA",    NULL, N_("Spanish (Panama)"), "español (Panamá)", NULL },
    { "es_DO",    NULL, N_("Spanish (Dominican Republic)"), "español (República Dominicana)", NULL },
    { "es_VE",    NULL, N_("Spanish (Venezuela)"), "español (Venezuela)", NULL },
    { "es_CO",    NULL, N_("Spanish (Colombia)"), "español (Colombia)", NULL },
    { "es_PE",    NULL, N_("Spanish (Peru)"), "español (Perú)", NULL },
    { "es_AR",    NULL, N_("Spanish (Argentina)"), "español (Argentina)", NULL },
    { "es_EC",    NULL, N_("Spanish (Ecuador)"), "español (Ecuador)", NULL },
    { "es_CL",    NULL, N_("Spanish (Chile)"), "español (Chile)", NULL },
    { "es_UY",    NULL, N_("Spanish (Uruguay)"), "español (Uruguay)", NULL },
    { "es_PY",    NULL, N_("Spanish (Paraguay)"), "español (Paraguay)", NULL },
    { "es_BO",    NULL, N_("Spanish (Bolivia)"), "español (Bolivia)", NULL },
    { "es_SV",    NULL, N_("Spanish (El Salvador)"), "español (El Salvador)", NULL },
    { "es_HN",    NULL, N_("Spanish (Honduras)"), "español (Honduras)", NULL },
    { "es_NI",    NULL, N_("Spanish (Nicaragua)"), "español (Nicaragua)", NULL },
    { "es_PR",    NULL, N_("Spanish (Puerto Rico)"), "español (Puerto Rico)", NULL },
    { "af_AF",    NULL, N_("Afrikaans"), "Afrikaans", NULL },
    { "ms_BN",    NULL, N_("Malay (Brunei Darussalam)"), "Bahasa Melayu (Brunei)", NULL },
    { "no_NO",    NULL, N_("Norwegian"), "Norsk", NULL },
    { "sr_RS",    NULL, N_("Serbian (Latin)"), "Srpski (latinica)", NULL },
    { "sr_RS",    NULL, N_("Serbian (Cyrillic)"), "Srpski (ćirilica)", NULL },
    { "zh_MO",    NULL, N_("Chinese (Macau)"), "Chinese (Macau)", NULL },
    { "ar_SA",    NULL, N_("Arabic (Saudi Arabia)"), "العربية (المملكة العربية السعودية)", NULL },
    { "ar_IQ",    NULL, N_("Arabic (Iraq)"), "العربية (العراق)", NULL },
    { "ar_LY",    NULL, N_("Arabic (Libya)"), "العربية (ليبيا)", NULL },
    { "ar_DZ",    NULL, N_("Arabic (Algeria)"), "العربية (الجزائر)", NULL },
    { "ar_MA",    NULL, N_("Arabic (Morocco)"), "العربية (المغرب)", NULL },
    { "ar_TN",    NULL, N_("Arabic (Tunisia)"), "العربية (تونس)", NULL },
    { "ar_OM",    NULL, N_("Arabic (Oman)"), "العربية (عُمان)", NULL },
    { "ar_YE",    NULL, N_("Arabic (Yemen)"), "العربية (اليمن)", NULL },
    { "ar_SY",    NULL, N_("Arabic (Syria)"), "العربية (سوريا)", NULL },
    { "ar_JO",    NULL, N_("Arabic (Jordan)"), "العربية (الأردن)", NULL },
    { "ar_KW",    NULL, N_("Arabic (Kuwait)"), "العربية (الكويت)", NULL },
    { "ar_AE",    NULL, N_("Arabic (UAE)"), "العربية (الإمارات العربية المتحدة)", NULL },
    { "ar_BH",    NULL, N_("Arabic (Bahrain)"), "العربية (البحرين)", NULL },
    { "ar_QA",    NULL, N_("Arabic (Qatar)"), "العربية (قطر)", NULL },
    { "az_IR",    NULL, N_("Azerbaijani"), "Azərbaycan", NULL },
    { "ha_BJ",    NULL, N_("Hausa"), "Hausa", NULL },
    { "cy_UK",    NULL, N_("Welsh"), "Cymraeg", NULL },
    { "xh_ZA",    NULL, N_("Xhosa"), "isiXhosa", NULL },
    { "yo_NG",    NULL, N_("Yoruba"), "Èdè Yorùbá", NULL },
    { "zu_ZA",    NULL, N_("Zulu"), "isiZulu", NULL },
    { "hg_IN",    NULL, N_("Hinglish"), "Hinglish", NULL },
    { "su_ID",    NULL, N_("Sundanese"), "Basa Sunda", NULL },
    { "tl_PH",    NULL, N_("Tagalog"), "Tagalog", NULL },
    { "af_ZA",    NULL, N_("Afrikaans"), "Afrikaans", NULL },
    { "jv_ID",    NULL, N_("Javanese"), "Basa Jawa", NULL },
    { "km_KH",    NULL, N_("Khmer"), "ខ្មែរ", NULL },
    { "fil",    NULL, N_("Filipino"), "Filipino", NULL },
    { "my",    NULL, N_("Myanmar"), "ماينمار", NULL },
    { "or",    NULL, N_("Odia"), "ଓଡ଼ିଆ", NULL },
    { "es_LA",    NULL, N_("Spanish (Latin America)"), "Español (América Latina)", NULL },

    { "", "", "", NULL, NULL }
};

class __LanguageLess
{
public:
    bool operator () (const __Language &lhs, const __Language &rhs) const {
        return strcmp (lhs.code, rhs.code) < 0;
    }

    bool operator () (const __Language &lhs, const String &rhs) const {
        return strcmp (lhs.code, rhs.c_str ()) < 0;
    }

    bool operator () (const String &lhs, const __Language &rhs) const {
        return strcmp (lhs.c_str (), rhs.code) < 0;
    }
};

static bool language_sorted = false;

static __Language *
__find_language (const String &lang)
{
    if (!language_sorted) {
        language_sorted = true;
        for (int loop = 0; ((unsigned int)loop) < sizeof(__languages) / sizeof(__Language) - 1; loop++) {
            for (int innerLoop = loop + 1; ((unsigned int)innerLoop) < sizeof(__languages) / sizeof(__Language) - 1; innerLoop++) {
                if (strcmp (__languages[loop].code, __languages[innerLoop].code) > 0) {
                    __Language temp = __languages[innerLoop];
                    __languages[innerLoop] = __languages[loop];
                    __languages[loop] = temp;
                }
            }
        }
    }

    static __Language *langs_begin = __languages;
    static __Language *langs_end   = __languages + sizeof (__languages) / sizeof (__Language) - 1;

    String nlang = lang;
    bool contry_code = false;

    // Normalize the language name.
    for (String::iterator it = nlang.begin (); it != nlang.end (); ++it) {
        if (*it == '-' || *it == '_') {
            *it = '_';
            contry_code = true;
        } else if (contry_code) {
            *it = toupper (*it);
        } else {
            *it = tolower (*it);
        }
    }

    __Language *result = std::lower_bound (langs_begin, langs_end, nlang, __LanguageLess ());

    if (result != langs_end) {
        if (strncmp (result->code, nlang.c_str (), strlen (result->code)) == 0 ||
            (strncmp (result->code, nlang.c_str (), nlang.length ()) == 0 &&
             strncmp (result->code, (result+1)->code, nlang.length ()) != 0))
            return result;
    }

    return NULL;
}

EAPI String
scim_get_language_name (const String &lang)
{
    return String (_(scim_get_language_name_english (lang).c_str ()));
}

EAPI String
scim_get_language_name_english (const String &lang)
{
    __Language *result = __find_language (lang);

    if (result)
        return String (result->name);

    return String ("Other");
}

EAPI String
scim_get_language_name_untranslated (const String &lang)
{
    __Language *result = __find_language (lang);

    if (result) {
        if (result->untranslated)
            return String (result->untranslated);
        else
            return String (_(result->name));
    }

    return String (_("Other"));
}

EAPI String
scim_get_language_locales (const String &lang)
{
    __Language *result = __find_language (lang);

    std::vector<String> locales;

    if (result) {
        String good;

        if (strlen (result->code) < 5 && result->normalized)
            result = __find_language (result->normalized);

        if (result)
            good = scim_validate_locale (String (result->code) + ".UTF-8");

        if (good.length ()) locales.push_back (good);

        if (result && result->locale_suffix) {
            std::vector<String> suffixes;

            scim_split_string_list (suffixes, result->locale_suffix, ',');
            for (size_t i = 0; i < suffixes.size (); ++ i) {
                good = scim_validate_locale (String (result->code) + suffixes [i]);
                if (good.length ()) locales.push_back (good);
            }
        }

        if (result)
            good = scim_validate_locale (result->code);

        if (good.length ()) locales.push_back (good);
    }

    return scim_combine_string_list (locales, ',');
}

EAPI String
scim_get_locale_language (const String &locale)
{
    if (locale.length () == 0) return String ();

    String str = locale.substr (0, locale.find ('.'));
    return scim_validate_language (str.substr (0, str.find ('@')));
}

EAPI String
scim_validate_language (const String &lang)
{
    __Language *result = __find_language (lang);

    if (result)
        return String (result->code);

    // Add prefix ~ to let other become the last item when sorting.
    return String ("~other");
}

EAPI String
scim_get_normalized_language (const String &lang)
{
    __Language *result = __find_language (lang);

    if (result) {
        if (result->normalized) return String (result->normalized);
        else return String (result->code);
    }

    // Add prefix ~ to let other become the last item when sorting.
    //return String ("~other");
    return String ("en");
}

#ifndef SCIM_LAUNCHER
 #define SCIM_LAUNCHER  (SCIM_LIBEXECDIR "/scim-launcher")
#endif

EAPI int  scim_launch (bool          daemon,
                  const String &config,
                  const String &imengines,
                  const String &frontend,
                  char  * const argv [])
{
    if (!config.length () || !imengines.length () || !frontend.length ())
        return -1;

    int   new_argc = 0;
    char *new_argv [40];

    new_argv [new_argc ++] = strdup (SCIM_LAUNCHER);

    if (daemon)
        new_argv [new_argc ++] = strdup ("-d");

    new_argv [new_argc ++] = strdup ("-c");
    new_argv [new_argc ++] = strdup (config.c_str ());
    new_argv [new_argc ++] = strdup ("-e");
    new_argv [new_argc ++] = strdup (imengines.c_str ());
    new_argv [new_argc ++] = strdup ("-f");
    new_argv [new_argc ++] = strdup (frontend.c_str ());

    if (argv) {
        for (int i = 0; argv [i] && new_argc < 39 ; ++i, ++new_argc)
            new_argv [new_argc] = strdup (argv [i]);
    }

    new_argv [new_argc] = 0;

    pid_t child_pid;

    child_pid = fork ();

    ISF_SAVE_LOG ("ppid: %d, fork result : %d, user %s\n", getppid (), child_pid, scim_get_user_name ().c_str ());

    // Error fork.
    if (child_pid < 0) return -1;

    // In child process, start scim-launcher.
    if (child_pid == 0) {
        return execv (SCIM_LAUNCHER, new_argv);
    }

    // In parent process, wait the child exit.

    for (int i = 0; i < new_argc; ++i)
        if (new_argv [i]) free (new_argv [i]);

    int status;
    pid_t ret_pid;

    ret_pid = waitpid (child_pid, &status, 0);

    if (ret_pid == child_pid && WIFEXITED(status))
        return WEXITSTATUS(status);

    return -1;
}

#ifndef SCIM_PANEL_PROGRAM
  #define SCIM_PANEL_PROGRAM  (SCIM_BINDIR "/scim-panel-gtk")
#endif

EAPI int scim_launch_panel (bool          daemon,
                       const String &config,
                       const String &display,
                       char * const  argv [])
{
    if (!config.length ())
        return -1;

    String panel_program = scim_global_config_read (SCIM_GLOBAL_CONFIG_DEFAULT_PANEL_PROGRAM, String (SCIM_PANEL_PROGRAM));

    if (!panel_program.length ())
        panel_program = String (SCIM_PANEL_PROGRAM);

    if (panel_program [0] != SCIM_PATH_DELIM) {
        panel_program = String (SCIM_BINDIR) +
                        String (SCIM_PATH_DELIM_STRING) +
                        panel_program;
    }

    //if the file is not exist or is not executable, fallback to default
    if (access (panel_program.c_str (), X_OK) != 0)
            panel_program = String (SCIM_PANEL_PROGRAM);

    int   new_argc = 0;
    char *new_argv [80];

    new_argv [new_argc ++] = strdup (panel_program.c_str ());

    if (display.length() > 0) {
        new_argv [new_argc ++] = strdup ("--display");
        new_argv [new_argc ++] = strdup (display.c_str ());
    }

    new_argv [new_argc ++] = strdup ("-c");
    new_argv [new_argc ++] = strdup (config.c_str ());

    if (daemon)
        new_argv [new_argc ++] = strdup ("-d");

    if (argv) {
        for (int i = 0; argv [i] && new_argc < 40; ++i, ++new_argc)
            new_argv [new_argc] = strdup (argv [i]);
    }

    new_argv [new_argc] = 0;

    pid_t child_pid;

    child_pid = fork ();

    ISF_SAVE_LOG ("ppid : %d fork result : %d\n", getppid (), child_pid);

    // Error fork.
    if (child_pid < 0) return -1;

    // In child process, start scim-launcher.
    if (child_pid == 0) {
        return execv (panel_program.c_str (), new_argv);
    }

    // In parent process, wait the child exit.

    for (int i = 0; i < new_argc; ++i)
        if (new_argv [i]) free (new_argv [i]);

    int status;
    pid_t ret_pid;

    ret_pid = waitpid (child_pid, &status, 0);

    if (ret_pid == child_pid && WIFEXITED(status))
        return WEXITSTATUS(status);

    return -1;
}

EAPI void
scim_usleep (unsigned int usec)
{
    if (usec == 0) return;

#if HAVE_NANOSLEEP
    struct timespec req, rem;

    req.tv_sec = usec / 1000000;
    req.tv_nsec = (usec % 1000000) * 1000;

    while (nanosleep (&req, &rem) == -1 && errno == EINTR && (rem.tv_sec != 0 || rem.tv_nsec != 0))
        req = rem;
#elif HAVE_USLEEP
    unsigned int sec = usec / 1000000;
    usec %= 1000000;

    for (unsigned int i = 0; i < sec; ++i)
        sleep (1);

    usleep (usec);
#else
    unsigned int sec = usec / 1000000;

    sleep (sec ? sec : 1);
#endif
}

EAPI void scim_daemon ()
{
#if HAVE_DAEMON
    ISF_SAVE_LOG ("ppid:%d  calling daemon()\n", getppid ());

    if (daemon (0, 0) == -1)
        std::cerr << "Error to make SCIM into a daemon!\n";

    ISF_SAVE_LOG ("ppid:%d  daemon() called\n", getppid ());

    return;
#else
    pid_t id;

    id = fork ();
    if (id == -1) {
        std::cerr << "Error to make SCIM into a daemon!\n";
        return;
    } else if (id > 0) {
        _exit (0);
    }

    ISF_SAVE_LOG ("ppid:%d fork result : %d\n", getppid (), id);

    id = fork ();
    if (id == -1) {
        std::cerr << "Error to make SCIM into a daemon!\n";
        return;
    } else if (id > 0) {
        _exit (0);
    }

    ISF_SAVE_LOG ("ppid:%d fork result : %d\n", getppid (), id);

    return;
#endif
}

EAPI void isf_save_log (const char *fmt, ...)
{
    char buf[1024] = {0};
    va_list ap;

    va_start (ap, fmt);
    vsnprintf (buf, sizeof (buf), fmt, ap);
    va_end (ap);

    const int MAX_LOG_FILE_SIZE = 10 * 1024; /* 10KB */

    static bool size_exceeded = false;
    static struct stat st;
    if (!size_exceeded) {
        String strLogFile = scim_get_user_data_dir () + String (SCIM_PATH_DELIM_STRING) + String ("isf.log");
        int ret = stat(strLogFile.c_str(), &st);
        if (ret == 0 || (ret == -1 && errno == ENOENT)) {
            if (st.st_size < MAX_LOG_FILE_SIZE) {
                std::ofstream isf_log_file (strLogFile.c_str (), std::ios::app);
                isf_log_file << buf;
                isf_log_file.flush ();
            } else {
                size_exceeded = true;
            }
        }
    }

    LOGD ("%s", buf);
}

static struct timeval _t0 = {0, 0};
static struct timeval _t1;

static clock_t _p_t0;
static clock_t _p_t1;

void ISF_PROF_DEBUG_TIME_BEGIN ()
{
    struct tms tms;

    _p_t0 = times (&tms);
}

/* Measure elapsed time */
void ISF_PROF_DEBUG_TIME_END (char const* format, char const* func, int line)
{
    float etime = 0.0;
    struct tms tms;
    static long clktck = 0;

    if (clktck == 0)
        if ((clktck = sysconf (_SC_CLK_TCK)) < 0)
            return;

    _p_t1 = times (&tms);

    etime = (_p_t1 - _p_t0) / (double) clktck;

    printf (format, func, line);
    printf (mzc_red "[T:%ld][E:%f]" mzc_normal " ", _p_t1, etime);

    return;
}

/* Measure elapsed time */
void ISF_PROF_DEBUG_TIME (char const* func, int line, char const* str)
{
    float etime = 0.0;

    if (_t0.tv_sec == 0 && _t0.tv_usec == 0) {
        gettimeofday (&_t0, NULL);
        return;
    }

    gettimeofday (&_t1, NULL);

    if (_t0.tv_usec != 0) {
        etime = ((_t1.tv_sec * 1000000 + _t1.tv_usec) - (_t0.tv_sec * 1000000 + _t0.tv_usec))/1000000.0;
    }

    printf (mzc_red "[%s:%04d] [T:%ld][E:%f] %s " mzc_normal " \n", func, line, (_t1.tv_sec * 1000000 + _t1.tv_usec), etime, str);

    _t0 = _t1;

    return;
}

EAPI void gettime (clock_t clock_start, const char* str)
{
#ifdef ISF_PROF
    struct  tms tiks_buf;
    double  clock_tiks = (double)sysconf (_SC_CLK_TCK);
    clock_t times_tiks = times (&tiks_buf);
    double  times_secs = (double)(times_tiks - clock_start) / clock_tiks;
    double  utime      = (double)tiks_buf.tms_utime / clock_tiks;
    double  stime      = (double)tiks_buf.tms_stime / clock_tiks;
    printf (mzc_red "%s Times:   %.3f \t User:   %.3f \t System:   %.3f " mzc_normal "\n", str, (double)times_secs, (double)utime , (double)stime);
#endif
}

} // namespace scim

/*
vi:ts=4:ai:nowrap:expandtab
*/
