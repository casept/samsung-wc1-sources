/*
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "AboutDataTizen.h"

#if ENABLE(TIZEN_WEBKIT2_ABOUT_MEMORY)
#include "AboutCredits.html.cpp"
#include "AboutTemplate.html.cpp"
#include "BlobData.h"
#include "BlobRegistry.h"
#include "BlobRegistryImpl.h"
#include "JSDOMWindow.h"
#include "MemoryCache.h"
#include "MemoryStatistics.h"
#include "WKAPICast.h"
#include "WebContext.h"
#include "WebKitVersion.h"
#include "WebMemorySampler.h"
#include "PluginModuleInfo.h"
#include "ewk_context.h"
#include "ewk_context_private.h"
#include <WebCore/ApplicationCache.h>
#include <WebCore/ApplicationCacheResource.h>
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/ApplicationCacheGroup.h>
#include <WebCore/PluginData.h>
#include <heap/Heap.h>
#include <malloc.h>
//#include <runtime/JSGlobalData.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#if ENABLE(TIZEN_NATIVE_MEMORY_SNAPSHOT)
#include "InspectorMemoryAgent.h"
namespace WebCore {
extern HashSet<Page*>* allPages;
}
#endif
using namespace WebCore;
using namespace WTF;

const int kiloByte = 1024;
const int megaByte = 1024 * 1024;
const unsigned meminfoIndex = 10;

enum UNIT { BYTE, KILOBYTE, MEGABYTE, EA };

namespace WebKit {

class WebContext;

typedef HashMap<String, RefPtr<BlobStorageData> > BlobMap;

PrintFormat printFormat = PRINT_FORMAT_HTML;

typedef struct ApplicationMemoryStats {
    size_t totalProgramSize;
    size_t residentSetSize;
    size_t proportionalSetSize;
    size_t peak;
    size_t sharedSize;
    size_t textSize;
    size_t librarySize;
    size_t dataStackSize;
    size_t dirtyPageSize;
    size_t UMPSize;
    size_t graphics3DSize;
    size_t privateCleanSize;
    size_t privateDirtySize;
} ApplicationMemoryStats;

static const unsigned int maxBuffer = 128;
static const unsigned int maxProcessPath = 35;

static String getToken(FILE* file)
{
    ASSERT(file);
    if (!file)
        return String();

    char buffer[maxBuffer] = {0, };
    unsigned int index = 0;
    while (index < maxBuffer) {
        int ch = fgetc(file);
        if (ch == EOF || (isASCIISpace(ch) && index)) // Break on non-initial ASCII space.
            break;
        if (!isASCIISpace(ch)) {
            buffer[index] = ch;
            index++;
        }
    }

    return String(buffer);
}

static unsigned getPeakRSS(int pid)
{
    char processPath[maxProcessPath];
    sprintf(processPath, "/proc/%d/status", pid);

    FILE* fStatus = fopen(processPath, "r");
    if (!fStatus) {
        fprintf(stderr, "cannot open %s\n", processPath);
        return 0;
    }

    unsigned size = 0;
    while (!feof(fStatus)) {
        String strToken = getToken(fStatus);
        if (strToken.find("VmHWM") != notFound) {
            size = getToken(fStatus).toUInt();
            break;
        }
    }

    fclose(fStatus);
    return size;
}

void sampleDeviceMemoryMalloc(ApplicationMemoryStats& applicationStats, int pid)
{
    const char* strUMPZone = "/dev/ump";
    const char* strGraphicsZone = "/dev/mali";
    const char* strPrivateClean = "Private_Clean";
    const char* strPrivateDirty = "Private_Dirty";
    const char* strDeviceDriver = "/dev/dri/card0";
    const char* strProportionalSetSize = "Pss";

    char processPath[maxProcessPath];
    sprintf(processPath, "/proc/%d/smaps", pid);
    FILE* fMAPS = fopen(processPath, "r");
    if (fMAPS) {
        int UMPSize = 0;
        int graphics3DSize = 0;
        int privateCleanSize = 0;
        int privateDirtySize = 0;
        int deviceDriverSize = 0;
        int proportionalSetSize = 0;
        while (!feof(fMAPS)) {
            String strToken = getToken(fMAPS);
            if (strToken.find(strUMPZone) != notFound) {
                getToken(fMAPS);                // Skip "Size:"
                strToken = getToken(fMAPS);     // Actual size
                UMPSize = UMPSize + strToken.toInt();
            }
            if (strToken.find(strGraphicsZone) != notFound) {
                getToken(fMAPS);                // Skip "Size:"
                strToken = getToken(fMAPS);     // Actual size
                graphics3DSize = graphics3DSize + strToken.toInt();
            }
            if (strToken.find(strPrivateClean) != notFound) {
                strToken = getToken(fMAPS);     // Actual size
                privateCleanSize = privateCleanSize + strToken.toInt();
            }
            if (strToken.find(strPrivateDirty) != notFound) {
                strToken = getToken(fMAPS);     // Actual size
                privateDirtySize = privateDirtySize + strToken.toInt();
            }
            if (strToken.find(strDeviceDriver) != notFound) {
                getToken(fMAPS);                // Skip "Size:"
                strToken = getToken(fMAPS);     // Actual size
                deviceDriverSize = deviceDriverSize + strToken.toInt();
            }
            if (strToken.find(strProportionalSetSize) != notFound) {
                strToken = getToken(fMAPS);     // Actual size
                proportionalSetSize += strToken.toInt();
            }
        }
        applicationStats.UMPSize = UMPSize;
        applicationStats.graphics3DSize = graphics3DSize;
        applicationStats.privateCleanSize = privateCleanSize;
        applicationStats.privateDirtySize = privateDirtySize;
        applicationStats.residentSetSize -= deviceDriverSize;
        applicationStats.proportionalSetSize = proportionalSetSize - deviceDriverSize;
        applicationStats.peak = getPeakRSS(pid) - deviceDriverSize;

        fclose(fMAPS);
    }
}

ApplicationMemoryStats sampleApplicationMalloc(int pid)
{
    ApplicationMemoryStats applicationStats;

    char processPath[maxProcessPath];
    sprintf(processPath, "/proc/%d/statm", getpid());
    FILE* fMemoryStatus = fopen(processPath, "r");
    if (fMemoryStatus) {
        int result = fscanf(fMemoryStatus, "%d %d %d %d %d %d %d",
                &applicationStats.totalProgramSize,
                &applicationStats.residentSetSize,
                &applicationStats.sharedSize,
                &applicationStats.textSize,
                &applicationStats.librarySize,
                &applicationStats.dataStackSize,
                &applicationStats.dirtyPageSize);
        UNUSED_PARAM(result);
        fclose(fMemoryStatus);
    }

    // Get Additional Memory Info: UMP, 3D Graphics, Device Driver, Private Clean/Dirty, RSS, PSS Bytes
    sampleDeviceMemoryMalloc(applicationStats, pid);

    return applicationStats;
}

static String header(const String& description)
{
    if (printFormat == PRINT_FORMAT_SIMPLE)
        return description + "\n";
    else if (printFormat == PRINT_FORMAT_JSON)
        return  "";
    else
        return String("<div class='box'><div class='box-title'>")
               + description + String("</div><table class='fixed-table'><col width=75%><col width=25%>");
}

static String footer()
{
    if (printFormat == PRINT_FORMAT_SIMPLE)
        return "";
    else if (printFormat == PRINT_FORMAT_JSON)
        return "";
    else
        return "</table></div><br>";
}

template<class T> static String numberToRow(const String& description, T number, UNIT type)
{
    if (printFormat == PRINT_FORMAT_SIMPLE)
        return description + ":" + String::number(number) + "\n";
    else if (printFormat == PRINT_FORMAT_JSON)
        return String("\"") + description + "\":" + String::number(number) + ",";

    String unit;
    switch (type) {
    case BYTE:
        unit = "B";
        break;
    case KILOBYTE:
        unit = "KB";
        number /= kiloByte;
        break;
    case MEGABYTE:
        unit = "MB";
        number /= megaByte;
        break;
    case EA:
        break;
    }
    return String("<tr><td>") + description + "</td><td>" + String::number(number) + " " + unit + "</td></tr>";
}

String numberToRow(const String& description, bool truth)
{
    if (printFormat == PRINT_FORMAT_SIMPLE)
        return description + ":" + (truth?"true":"false") + "\n";
    else if (printFormat == PRINT_FORMAT_JSON)
        return String("\"") + description + "\":" + (truth?"true":"false") + ",";
    else
        return String("<tr><td>") + description + "</td><td>" + (truth?"true":"false") + "</td></tr>";
}

static String cacheTypeStatisticToRow(const String& description, const MemoryCache::TypeStatistic& statistic)
{
    if (printFormat == PRINT_FORMAT_SIMPLE)
        return description + ":"
           + String::number(statistic.count) + " "
           + String::number(statistic.size / kiloByte) + " "
           + String::number(statistic.liveSize / kiloByte) + " "
           + String::number(statistic.decodedSize / kiloByte) + "\n";
    else if (printFormat == PRINT_FORMAT_JSON)  //If dump format is 'JSON', only print the size of the cache
        return String("\"") +  description + "\":"
           + String::number(statistic.size) +  "," ;
    else
        return String("<tr><td>") + description + "</td>"
            + "<td>" + String::number(statistic.count) + "</td>"
            + "<td>" + String::number(statistic.size / kiloByte) + "</td>"
            + "<td>" + String::number(statistic.liveSize / kiloByte) + "</td>"
            + "<td>" + String::number(statistic.decodedSize / kiloByte) + "</td>"
            + "</tr>";
}

static String appcacheStatisticToRow(const String& flag, const String& description, const int cacheSize)
{
    return String("<tr><td>") + flag + "</td>"
        + "<td style=\"word-break:break-all;\">" + description + "</td>"
        + "<td>" + String::number(cacheSize) + "B</td>"
        + "</tr>";
}

static String helpTextToRow(const String& feature, const String& description)
{
    return String("<tr><td>") + feature + "</td>" + "<td style=\"word-break:break-all;\">" + description + "</td></tr>";

}

static String creditToRow(const String& credit)
{
    return "<div class='box'><span class='box-title'>" + credit + "</span>"
        "<span class='license'><a href=# onclick=\"license('" + credit + "');\">license</a> </span>"
        "<table class='fixed-table'><col width=75%><col width=25%>"
        "<tr><td><div id='" + credit + "_license' style='display:none;'>"
        + getLicense(credit) + "</div></td></tr></table></div><br>";
}

static void dumpJSCTypeCountSetToTableHTML(StringBuilder& tableHTML, JSC::TypeCountSet* typeCountSet)
{
    if (!typeCountSet)
        return;

    for (JSC::TypeCountSet::const_iterator iter = typeCountSet->begin(); iter != typeCountSet->end(); ++iter)
        tableHTML.append(numberToRow(iter->key, iter->value, EA));
}

static void getLowMemoryNotifyInfo(size_t value[])
{
    const char* TIZEN_MEMORY_INFO_PATH = "/sys/class/lowmemnotify/meminfo";
    unsigned index = 0;
    bool foundKeyName = false;
    FILE* fSystemMemoryInfo = fopen(TIZEN_MEMORY_INFO_PATH, "r");

    if(fSystemMemoryInfo) {
        while (!feof(fSystemMemoryInfo)) {
            String strToken = getToken(fSystemMemoryInfo);
            if (strToken.find(':') != notFound) {
                String keyName = strToken.left(strToken.length() - 1);
                foundKeyName = true;
            } else if (foundKeyName) {
                value[index++] = strToken.toInt();
                if (index == meminfoIndex)
                    break;
                foundKeyName = false;
            }
        }
        fclose(fSystemMemoryInfo);
    }
}

String memoryPage(PrintFormat format)
{
    printFormat = format;
    StringBuilder page;
    if (printFormat == PRINT_FORMAT_HTML)
        page.append(writeHeader("Memory"));

    // generate system memory infomation
    page.append(header("System Memory Usage"));

    // TIZEN System Memory Info
    size_t systemMemory[meminfoIndex];
    getLowMemoryNotifyInfo(systemMemory);

    page.append(numberToRow("Total RAM size", systemMemory[0] * megaByte, MEGABYTE));
    page.append(numberToRow("Used (Mem+Reclaimable) RAM size", systemMemory[1] * megaByte, MEGABYTE));
    page.append(numberToRow("Used (Mem+Swap) RAM size", systemMemory[2] * megaByte, MEGABYTE));
    page.append(numberToRow("Used (Mem) RAM size", systemMemory[3] * megaByte, MEGABYTE));
    page.append(numberToRow("Used (Swap) RAM size", systemMemory[4] * megaByte, MEGABYTE));
    page.append(numberToRow("Free RAM size", systemMemory[6] * megaByte, MEGABYTE));
    page.append(numberToRow("Free CMA size", systemMemory[7] * megaByte, MEGABYTE));
    page.append(numberToRow("Available (Free+Reclaimable) RAM size", systemMemory[8] * megaByte, MEGABYTE));
    page.append(numberToRow("Reserved Page RAM size", systemMemory[9] * megaByte, MEGABYTE));

    page.append(footer());

    // UI Process memps information
    ApplicationMemoryStats applicationStats = sampleApplicationMalloc(getppid());

    page.append(header("UI Process"));

    page.append(numberToRow("UI Process Private CODE", applicationStats.privateCleanSize * kiloByte, KILOBYTE));
    page.append(numberToRow("UI Process Private DATA", applicationStats.privateDirtySize * kiloByte, KILOBYTE));
    page.append(numberToRow("UI Process PEAK", applicationStats.peak * kiloByte, KILOBYTE));
    page.append(numberToRow("UI Process RSS", applicationStats.residentSetSize * kiloByte, KILOBYTE));
    page.append(numberToRow("UI Process PSS", applicationStats.proportionalSetSize * kiloByte, KILOBYTE));
    page.append(numberToRow("UI Process 3D memory", applicationStats.graphics3DSize * kiloByte, KILOBYTE));
    page.append(numberToRow("UI Process UMP memory", applicationStats.UMPSize * kiloByte, KILOBYTE));

    page.append(footer());

    // Web Process memps information
    applicationStats = sampleApplicationMalloc(getpid());

    page.append(header("Web Process"));

    page.append(numberToRow("Web Process Private CODE", applicationStats.privateCleanSize* kiloByte, KILOBYTE));
    page.append(numberToRow("Web Process Private DATA", applicationStats.privateDirtySize* kiloByte, KILOBYTE));
    page.append(numberToRow("Web Process PEAK", applicationStats.peak * kiloByte, KILOBYTE));
    page.append(numberToRow("Web Process RSS", applicationStats.residentSetSize * kiloByte, KILOBYTE));
    page.append(numberToRow("Web Process PSS", applicationStats.proportionalSetSize * kiloByte, KILOBYTE));
    page.append(numberToRow("Web Process 3D memory", applicationStats.graphics3DSize * kiloByte, KILOBYTE));
    page.append(numberToRow("Web Process UMP memory", applicationStats.UMPSize * kiloByte, KILOBYTE));

    page.append(footer());

    // Memory Cache
    MemoryCache* cacheInc = memoryCache();
    MemoryCache::Statistics cacheStat = cacheInc->getStatistics();

    MemoryCache::TypeStatistic total;
    total.count = cacheStat.images.count + cacheStat.cssStyleSheets.count
            + cacheStat.scripts.count + cacheStat.xslStyleSheets.count + cacheStat.fonts.count;
    total.size = cacheInc->liveSize() + cacheInc->deadSize();
    total.liveSize = cacheInc->liveSize();
    total.decodedSize = cacheStat.images.decodedSize
            + cacheStat.cssStyleSheets.decodedSize + cacheStat.scripts.decodedSize
            + cacheStat.xslStyleSheets.decodedSize + cacheStat.fonts.decodedSize;

    // JS engine memory usage.
    JSC::GlobalMemoryStatistics jscMemoryStat = JSC::globalMemoryStatistics();
    JSC::Heap& mainHeap = JSDOMWindow::commonVM()->heap;
    OwnPtr<JSC::TypeCountSet> objectTypeCounts = mainHeap.objectTypeCounts();
    OwnPtr<JSC::TypeCountSet> protectedObjectTypeCounts = mainHeap.protectedObjectTypeCounts();

    // Malloc info.
    struct mallinfo mallocInfo = mallinfo();

    page.append(header("Web Process Memory Details"));

    page.append(numberToRow("Cache used memory", total.size, KILOBYTE));
    page.append(numberToRow("JSC used memory", jscMemoryStat.stackBytes + jscMemoryStat.JITBytes + mainHeap.capacity(), KILOBYTE));
    page.append(numberToRow("Malloc used memory", mallocInfo.usmblks + mallocInfo.uordblks, KILOBYTE));
    page.append(numberToRow("Total(cache+JSC+malloc) used memory", total.size + mallocInfo.usmblks + mallocInfo.uordblks + jscMemoryStat.stackBytes + jscMemoryStat.JITBytes + mainHeap.capacity(), KILOBYTE));

    page.append(footer());

    // generate cache information
    if(printFormat == PRINT_FORMAT_HTML) {
        page.append(String("<div class=\"box\"><div class=\"box-title\">Cache Information<br><div style='font-size:11px;color:#A8A8A8'>Size, Living, and Decoded are expressed in KB.</div><br></div><table class='fixed-table'><col width=75%><col width=25%>"));
        page.append(String("<tr> <th align=left>Item</th> <th align=left>Count</th> <th align=left>Size</th> <th align=left>Living</th> <th align=left>Decoded</th></tr>"));
    } else
        page.append(header("Cache Information"));

    if (printFormat == PRINT_FORMAT_HTML) {
        page.append(cacheTypeStatisticToRow("Total", total));
        page.append(cacheTypeStatisticToRow("Images", cacheStat.images));
        page.append(cacheTypeStatisticToRow("CSS Style Sheets", cacheStat.cssStyleSheets));
        page.append(cacheTypeStatisticToRow("Scripts", cacheStat.scripts));
    #if ENABLE(XSLT)
        page.append(cacheTypeStatisticToRow("XSL Style Sheets", cacheStat.xslStyleSheets));
    #endif
        page.append(cacheTypeStatisticToRow("Fonts", cacheStat.fonts));
   } else {
        page.append(cacheTypeStatisticToRow("Images Cache", cacheStat.images));
        page.append(cacheTypeStatisticToRow("CSS Style Sheets Cache", cacheStat.cssStyleSheets));
        page.append(cacheTypeStatisticToRow("Scripts Cache", cacheStat.scripts));
        page.append(cacheTypeStatisticToRow("Fonts Cache", cacheStat.fonts));
   }

    page.append(footer());
   // [Native memory information]
#if ENABLE(TIZEN_NATIVE_MEMORY_SNAPSHOT)  //Using InspectorMemoryAgent's new interface for about:memory
    if (allPages) {
        MemoryInformation info[10];
        int info_count;
        HashSet<Page*>::iterator end = allPages->end();
        for (HashSet<Page*>::iterator it = allPages->begin(); it != end; ++it) {
            page.append(header((*it)->mainFrame()->loader()->documentLoader()->url().string()));
            for (int i = 0; i < 10; ++i)
                info[i].value = 0;
            info_count = WebCore::InspectorMemoryAgent::getMemoryInformationForPage(info,*it);
            for(int i=0;i<info_count && i<10;i++)
                page.append(numberToRow(info[i].name, info[i].value, BYTE));
    page.append(footer());
       }
    }
#endif


    if (printFormat == PRINT_FORMAT_HTML) {
    // JS Engine Memory Usage
        page.append("<div class='box'><div class='box-title'>JS engine memory usage<br><div style='font-size:11px;color:#A8A8A8'>Stack, JIT, Heap are expressed in KB.</div><br></div><table class='fixed-table'><col width=75%><col width=25%>");

        page.append(numberToRow("Stack size", jscMemoryStat.stackBytes, KILOBYTE));
        page.append(numberToRow("JIT memory usage", jscMemoryStat.JITBytes, KILOBYTE));
        page.append(numberToRow("Main heap capacity", mainHeap.capacity(), KILOBYTE));
        page.append(numberToRow("Main heap size", mainHeap.size(), KILOBYTE));
        page.append(numberToRow("Object count", mainHeap.objectCount(), EA));
        page.append(numberToRow("Global object count", mainHeap.globalObjectCount(), EA));
        page.append(numberToRow("Protected object count", mainHeap.protectedObjectCount(), EA));
        page.append(numberToRow("Protected global object count", mainHeap.protectedGlobalObjectCount(), EA));

        page.append(footer());

        // JS object type counts
        page.append(header("JS object type counts"));
        dumpJSCTypeCountSetToTableHTML(page, objectTypeCounts.get());
        page.append(footer());

        // JS protected object type counts
        page.append(header("JS protected object type counts"));
        dumpJSCTypeCountSetToTableHTML(page, protectedObjectTypeCounts.get());
        page.append(footer());

        // Malloc Information
        page.append(header("Malloc Information"));

        page.append(numberToRow("Total space in use", mallocInfo.usmblks + mallocInfo.uordblks, KILOBYTE));
        page.append(numberToRow("Total space in free blocks", mallocInfo.fsmblks + mallocInfo.fordblks, KILOBYTE));
        page.append(numberToRow("Size of the arena", mallocInfo.arena, KILOBYTE));
        page.append(numberToRow("Number of big blocks in use", mallocInfo.ordblks, EA));
        page.append(numberToRow("Number of small blocks in use", mallocInfo.smblks, EA));
        page.append(numberToRow("Number of header blocks in use", mallocInfo.hblks, EA));
        page.append(numberToRow("Space in header block headers", mallocInfo.hblkhd, EA));
        page.append(numberToRow("Space in small blocks in use", mallocInfo.usmblks, EA));
        page.append(numberToRow("Memory in free small blocks", mallocInfo.fsmblks, EA));
        page.append(numberToRow("Space in big blocks in use", mallocInfo.uordblks, KILOBYTE));
        page.append(numberToRow("Memory in free big blocks", mallocInfo.fordblks, KILOBYTE));
        page.append(footer());

#if ENABLE(GLOBAL_FASTMALLOC_NEW)
        // Fast Malloc Information
        page.append(header("Fast Malloc Information"));

        FastMallocStatistics fastMallocStatistics = WTF::fastMallocStatistics();
        size_t fastMallocBytesInUse = fastMallocStatistics.committedVMBytes - fastMallocStatistics.freeListBytes;
        size_t fastMallocBytesCommitted = fastMallocStatistics.committedVMBytes;
        totalBytesInUse += fastMallocBytesInUse;
        totalBytesCommitted += fastMallocBytesCommitted;

        page.append(numberToRow("Fast Malloc In Use", fastMallocBytesInUse, KILOBYTE));
        page.append(numberToRow("Fast Malloc Committed Memory", fastMallocBytesCommitted, KILOBYTE));

        page.append(footer());
#endif
    }

    if(printFormat == PRINT_FORMAT_HTML)
        page.append("</body></html>");
    return page.toString();
}

static String appCachePage()
{
    printFormat = PRINT_FORMAT_HTML;

    StringBuilder page;
    page.append(writeHeader("Application Cache"));

    Vector<KURL> urls;
    cacheStorage().manifestURLs(&urls);

    if (!urls.size()) {
        page.append(header("Application cache is empty"));
        page.append(footer());
        return page.toString();
    }

    for(size_t index = 0; index < urls.size(); index++)
    {
        // generate application cache infomation
        page.append(String("<div class='box'><div class='box-title'>")
               + urls[index].string() + String("</div><table class='fixed-table'><col width=15%><col width=70%><col width=15%>"));

        ApplicationCacheGroup* cacheGroup = cacheStorage().cacheGroupForURL(urls[index]);
        ApplicationCache* cache = cacheGroup->newestCache();

        if (!cache || !cache->isComplete()) {
            page.append("cache is incomplete");
            page.append(footer());
        }

        ApplicationCache::ResourceMap::const_iterator end = cache->end();
        for (ApplicationCache::ResourceMap::const_iterator it = cache->begin(); it != end; ++it) {

            RefPtr<ApplicationCacheResource> resource = it->value;
            unsigned type = resource->type();

            StringBuilder flags;
            if (type & ApplicationCacheResource::Master)
                flags.append("Master");
            if (type & ApplicationCacheResource::Manifest)
                flags.append("Manifest");
            if (type & ApplicationCacheResource::Explicit)
                flags.append("Explicit");
            if (type & ApplicationCacheResource::Foreign)
                flags.append("Foreign");
            if (type & ApplicationCacheResource::Fallback)
                flags.append("Fallback");

            page.append(appcacheStatisticToRow(flags.toString(), it->key, resource->estimatedSizeInStorage()));
        }
        page.append(footer());
    }

    return page.toString();
}

static String blobPage()
{
    StringBuilder page;
    page.append(writeHeader("Blob internals"));
    page.append("<div class='box'><table class='fixed-table' style=word-break:break-all; width=100%>");

    BlobMap map = static_cast<BlobRegistryImpl&>(blobRegistry()).getBlobData();

    if (map.isEmpty()) {
        page.append(header("There is no Blob data"));
        page.append(footer());
        return page.toString();
    }

    BlobMap::const_iterator it = map.begin();
    const BlobMap::const_iterator itend = map.end();
    for (; it != itend; ++it) {
        // append blob object url information
        page.append("<tr><td>" + it->key + "<ul>");

        // append blob data information
        RefPtr<BlobStorageData> blobData = it->value;
        if (!blobData->contentType().isEmpty())
            page.append("<li>Content Type:" + blobData->contentType() + "</li>");
        if (!blobData->contentDisposition().isEmpty())
            page.append("<li>Content Disposition:" + blobData->contentDisposition() + "</li>");

        BlobDataItemList::const_iterator it = blobData->items().begin();
        const BlobDataItemList::const_iterator itend = blobData->items().end();
        for (; it != itend; ++it) {
            const BlobDataItem& blobItem = *it;

            switch (blobItem.type) {
            case BlobDataItem::Data:
                page.append("<li>Type: data</li>");
                break;
            case BlobDataItem::File:
                page.append("<li>Type: file</li>");
                if (!blobItem.path.isEmpty())
                    page.append("<li>Path: " + blobItem.path + "</li>");
                break;
            case BlobDataItem::Blob:
                page.append("<li>Type: blob</li>");
                if (!blobItem.url.isEmpty())
                    page.append("<li>URL: " + blobItem.url.string() + "</li>");
                break;
            default:
                break;
            }
            page.append("<li>Length: " + String::number(blobItem.length) + "</li>");
        }
        page.append("</ul></td></tr>");
    }

    page.append(footer());

    return page.toString();
}

static String cachePage()
{
    printFormat = PRINT_FORMAT_HTML;

    StringBuilder page;
    page.append(writeHeader("Memory Cache"));
    page.append("<div class='box'><table class='fixed-table' style=word-break:break-all; width=100%>");

    String cachedResourceLists = memoryCache()->dumpCachedResourceLists();
    if (cachedResourceLists.isEmpty()) {
        page.append(header("Memory cache is empty"));
        page.append(footer());
        return page.toString();
    }

    Vector<String> cacheInfo;
    cachedResourceLists.split('\n', cacheInfo);

    Vector<String>::iterator iter = cacheInfo.begin();
    for(; iter != cacheInfo.end(); ++iter)
        page.append("<tr><td>" + *iter + "</td></tr>");

    page.append(footer());

    return page.toString();
}

static String creditsPage()
{
    printFormat = PRINT_FORMAT_HTML;

    StringBuilder page;
    page.append(writeHeader("Credits"));
    page.append(creditToRow("WebKit"));
    page.append(footer());

    return page.toString();
}

static String helpPage()
{
    printFormat = PRINT_FORMAT_HTML;

    StringBuilder page;
    page.append(writeHeader("List of About Features"));
    page.append("<div class='box'><table class='fixed-table' style=word-break:break-all;><col width=35%><col width=65%>");

    page.append(helpTextToRow("about:appcache", "Application cache information"));
    page.append(helpTextToRow("about:cache", "Memory cache information"));
    page.append(helpTextToRow("about:credits", "License information"));
    page.append(helpTextToRow("about:help", "List of About features"));
    page.append(helpTextToRow("about:memory", "Memory Statistics"));
    page.append(helpTextToRow("about:plugins", "Plugin information"));
    page.append(helpTextToRow("about:version", "Version information"));

    page.append(footer());

    return page.toString();
}

static String pluginsPage()
{
    printFormat = PRINT_FORMAT_HTML;

    StringBuilder page;
    page.append(writeHeader("Plugin"));
    page.append("<div class='box'><table class='fixed-table' style=word-break:break-all; width=100%>");

    // FIXME: Ewk_Context should be obtained from Ewk_View. This will be fixed after refactoring 'about:' implementation itself.
    //Ewk_Context* context = ewk_context_default_get();
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(ewk_context_default_get());
    Vector<PluginModuleInfo> plugins = (toImpl(ewkContext->wkContext())->pluginInfoStore()).plugins();

    if (plugins.isEmpty()) {
        page.append(header("There is no plugin"));
        page.append(footer());
        return page.toString();
    }

    Vector<PluginModuleInfo>::const_iterator it = plugins.begin();
    const Vector<PluginModuleInfo>::const_iterator itend = plugins.end();
    for (; it != itend; ++it) {
        page.append("<tr><td>");

        PluginInfo info = (*it).info;
        if (!info.name.isEmpty())
            page.append(info.name);

        page.append("<ul>");

        if (!info.file.isEmpty())
            page.append("<li>File: " + info.file + "</li>");

        if (!info.desc.isEmpty())
            page.append("<li>Description: " + info.desc + "</li>");

        page.append("</ul></td></td>");
    }

    page.append(footer());

    return page.toString();
}

static String versionPage()
{
    printFormat = PRINT_FORMAT_HTML;

    StringBuilder page;
    page.append(writeHeader("Version"));
    page.append("<div class='box'><table class='fixed-table' style=word-break:break-all; width=100%>");

    // TODO : More information(OS version, JavaScript Core version) will be added. 
    page.append(helpTextToRow("WebKit", String::format("%d.%d", WEBKIT_MAJOR_VERSION, WEBKIT_MINOR_VERSION)));

#if ENABLE(TIZEN_NATIVE_MEMORY_SNAPSHOT)
     HashSet<Page*>::iterator begin = allPages->begin();
     HashSet<Page*>::iterator end = allPages->end();
     KURL unused;
     if (begin != end)
        page.append(helpTextToRow("User Agent", header((*begin)->mainFrame()->loader()->userAgent(unused))));
#endif

    page.append(footer());

    return page.toString();
}

String aboutData(String aboutWhat)
{
    if (aboutWhat.endsWith("about:appcache", false))
        return appCachePage();
    if (aboutWhat.endsWith("about:blob", false))
        return blobPage();
    if (aboutWhat.endsWith("about:cache", false))
        return cachePage();
    if (aboutWhat.endsWith("about:credits", false))
        return creditsPage();
    if (aboutWhat.endsWith("about:help", false))
        return helpPage();
    if (aboutWhat.endsWith("about:memory", false))
        return memoryPage();
    if (aboutWhat.endsWith("about:plugins", false))
        return pluginsPage();
    if (aboutWhat.endsWith("about:version", false))
        return versionPage();

    return String();
}

} // namespace WebKit
#endif // end of #if ENABLE(TIZEN_WEBKIT2_ABOUT_MEMORY)
