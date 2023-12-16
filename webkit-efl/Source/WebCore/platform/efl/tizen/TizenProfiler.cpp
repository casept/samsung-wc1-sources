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

#include "config.h"
#include "TizenProfiler.h"

#if ENABLE(TIZEN_PROFILE)

#include <wtf/HashMap.h>

namespace WebCore {

static unsigned int USEC(unsigned int sec, unsigned int usec)
{
    return sec * 1000000 + usec;
}

Vector<ProfileItem*> TizenProfiler::s_vectorItems;

// Environment variable TIZEN_PROFILE can have one of 4 values : 'T'(Timeline), 'F'(Function), 'A'(All), 'L'(Log)
char* TizenProfiler::s_logType = getenv("TIZEN_PROFILE");

#if DEPTH_SUPPORT
// Environment variable TIZEN_PROFILE_NESTED_LEVEL can have allowedNestedLevel in profile data : '0'(not allow), '1'(allow 1 nested level)
int TizenProfiler::s_allowedNestedLevel = getenv("TIZEN_PROFILE_NESTED_LEVEL") ? atoi(getenv("TIZEN_PROFILE_NESTED_LEVEL")) : 32767;

int TizenProfiler::s_depthLevel = 0;
#endif

#if PROFILE_ONLY_IN_SPECIFIED_FUNCTIONS
// Initial value 1 let TizenProfiler profiles functions regardless of TizenProfileEnabler.
int TizenProfileEnabler::s_profileEnabled = getenv("TIZEN_PROFILE_SPECFIC_FUNCTION") ? 0 : 1;
#endif

void TizenProfiler::clearItems()
{
    for (size_t i = 0; i < s_vectorItems.size(); ++i)
        delete s_vectorItems[i];

    s_vectorItems.clear();
}

void TizenProfiler::print(FILE* fp)
{
    if (!s_logType)
        return;
    if (s_logType[0] == 'T' || s_logType[0] == 'A') // If env var TIZEN_PROFILE is Timeline or All
        TizenProfiler::printByTimeline(fp);
    if (s_logType[0] == 'F' || s_logType[0] == 'A') // If env var TIZEN_PROFILE is Function or All
        TizenProfiler::printByFunctions(fp);
}

void TizenProfiler::printByTimeline(FILE* fp)
{
    unsigned int previousTime = 0;
    for (size_t i = 0; i < s_vectorItems.size(); ++i) {
        ProfileItem* item = s_vectorItems[i];
        unsigned int currentTime = USEC(item->m_time.tv_sec, item->m_time.tv_usec);
        if (!item->m_startItem)
            fprintf(fp, " %80s : %u : %u\n",
                    item->m_functionName,
                    currentTime,
                    currentTime - previousTime
                    );
        else
            fprintf(fp, " %80s : %u : %u : %u\n",
                    item->m_startItem->m_functionName,
                    currentTime,
                    currentTime - previousTime,
                    USEC(item->m_time.tv_sec - item->m_startItem->m_time.tv_sec,
                         item->m_time.tv_usec - item->m_startItem->m_time.tv_usec)
                    );
        previousTime = currentTime;
    }
}

class FunctionProfileItem {
public:
    unsigned int count;
    unsigned int time;
};

void TizenProfiler::printByFunctions(FILE* fp)
{
    HashMap<const char*, FunctionProfileItem*> m_countFunctionMap;
    for (size_t i = 0; i < s_vectorItems.size(); ++i) {
        ProfileItem* item = s_vectorItems[i];
        if (item->m_startItem) {
            FunctionProfileItem* data = m_countFunctionMap.get(item->m_startItem->m_functionName);
            if (!data) {
                data = new FunctionProfileItem;
                data->count = 0;
                data->time = 0;
                m_countFunctionMap.set(item->m_startItem->m_functionName, data);
            }
            data->count++;
            data->time += USEC(item->m_time.tv_sec - item->m_startItem->m_time.tv_sec,
                               item->m_time.tv_usec - item->m_startItem->m_time.tv_usec);
        }
    }

    int profileFunctionCount = 0;
    int profileFunctionTotalTime = 0;
    HashMap<const char*, FunctionProfileItem*>::iterator end = m_countFunctionMap.end();
    for (HashMap<const char*, FunctionProfileItem*>::iterator it = m_countFunctionMap.begin(); it != end; ++it) {
        fprintf(fp, "%s\t%u\t%u\n", it->key, it->value->count, it->value->time);
        profileFunctionCount += it->value->count;
        profileFunctionTotalTime += it->value->time;
    }
    fprintf(fp, "TotalTime\t%u\t%u\n", profileFunctionCount, profileFunctionTotalTime);
}

static void __attribute__((destructor)) PrintProfileData()
{
    TizenProfiler::print(stdout);
}

}
#endif // ENABLE(TIZEN_PROFILE)