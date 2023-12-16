/*
   Copyright (C) 2011 Samsung Electronics

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

#ifndef TizenProfiler_h
#define TizenProfiler_h
#include <stdio.h>
#include <sys/time.h>
#include <wtf/Vector.h>

#if ENABLE(TIZEN_PROFILE)

using namespace WTF;

namespace WebCore {

#define TIZEN_PROFILE \
    WebCore::TizenProfiler profileObject(__PRETTY_FUNCTION__, __LINE__);

#define TIZEN_LOG \
    WebCore::TizenProfiler loggerObject(__PRETTY_FUNCTION__, __LINE__);

#define LOG_SUPPORT 1
#define DEPTH_SUPPORT 1
#define PROFILE_ONLY_IN_SPECIFIED_FUNCTIONS 0

class ProfileItem {
public:
    const char* m_functionName;
    struct timeval m_time;
    ProfileItem* m_startItem;
};

#if PROFILE_ONLY_IN_SPECIFIED_FUNCTIONS
class TizenProfiler;
class TizenProfileEnabler {
    friend class TizenProfiler;
public:
    TizenProfileEnabler() { s_profileEnabled++; }
    ~TizenProfileEnabler() { s_profileEnabled--; }

private:
    static int s_profileEnabled;
};
#endif

class TizenProfiler {
public:
    TizenProfiler(const char* functionName, int lineNumber = 0);
    ~TizenProfiler();

    static void clearItems();
    static void print(FILE* fp);
    static void printByTimeline(FILE* fp);
    static void printByFunctions(FILE* fp);
private:
    static Vector<ProfileItem*> s_vectorItems;
    static char* s_logType;

#if DEPTH_SUPPORT
    static int s_allowedNestedLevel;
    static int s_depthLevel;
#endif

    ProfileItem* m_item;

};

inline bool isSkippedFunction(const char* functionName)
{
    return strstr(functionName, "::connection()")
        || strstr(functionName, "::mainFrame()")
        || strstr(functionName, "::process()");
}

inline TizenProfiler::TizenProfiler(const char* functionName, int lineNumber)
{
    if (!s_logType) {
        m_item = 0;
        return;
    }

#if DEPTH_SUPPORT
    if (s_allowedNestedLevel < s_depthLevel) {
        m_item = 0;
        return;
    }
#endif

#if PROFILE_ONLY_IN_SPECIFIED_FUNCTIONS
    if (!TizenProfileEnabler::s_profileEnabled) {
        m_item = 0;
        return;
    }
#endif

    m_item = new ProfileItem();
    m_item->m_functionName = functionName;
    gettimeofday(&m_item->m_time, 0);
    m_item->m_startItem = 0;

    s_vectorItems.append(m_item);

#if LOG_SUPPORT
    if (s_logType[0] == 'L' && !isSkippedFunction(functionName)) {
        int level = 2 * s_depthLevel + 1;
        fprintf(stderr, " [ENTER] %*s%s [ %d]\n", level , ">", functionName, lineNumber);
    }
#endif

#if DEPTH_SUPPORT
    s_depthLevel++;
#endif
}

inline TizenProfiler::~TizenProfiler()
{
    if (!m_item)
        return;

    ProfileItem* item = new ProfileItem();
    gettimeofday(&item->m_time, 0);
    item->m_startItem = m_item;

    s_vectorItems.append(item);

#if DEPTH_SUPPORT
    s_depthLevel--;
#endif

#if LOG_SUPPORT
    if (s_logType[0] == 'L' && !isSkippedFunction(m_item->m_functionName)) {
        int level = 2 * s_depthLevel + 1;
        fprintf(stderr, " [LEAVE] %*s%s\n", level, " ", m_item->m_functionName);
    }
#endif
}

}

#endif // ENABLE(TIZEN_PROFILE)
#endif // TizenProfiler_h
