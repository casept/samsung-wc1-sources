/*
 * Copyright (C) 2014 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is based upon:
 * - ISO/IEC 14496-12: Information technology — Coding of audio-visual objects — Part 12: ISO base media file format
 * - ISO/IEC FDIS 23001-7:2011: MPEG systems technologies — Part 7: Common encryption in ISO base media file format files
 */


#include "config.h"

#include "MP4Parser.h"

namespace WebCore {

const inline std::string BoxTypeAsString(unsigned boxtype)
{
    char buf[5];
    buf[0] = (boxtype >> 24) & 0xff;
    buf[1] = (boxtype >> 16) & 0xff;
    buf[2] = (boxtype >> 8) & 0xff;
    buf[3] = (boxtype) & 0xff;
    buf[4] = 0;
    return std::string(buf);
}

template<typename T>
bool readT(T& outputValue, unsigned length, const unsigned char* data)
{
    if (sizeof(T) < length)
        return false;

    outputValue = 0;
    for (size_t i = 0; i < length; i++) {
        outputValue <<= 8;
        outputValue += data[0];
        data++;
    }
    return true;
}

Box::Box(const unsigned char* data, unsigned long long length)
    : m_size(0)
    , m_largeSize(0)
    , m_data(data)
    , m_leftLength(length)
    , m_type(0)
{
}

Box::Box()
    : m_size(0)
    , m_largeSize(0)
    , m_data(0)
    , m_leftLength(0)
    , m_type(0)
{
}

bool Box::getChildBoxInfo(unsigned long long& size, unsigned &type)
{
    size = 0;
    type = 0;
    if (!read4(&size) || !read4((unsigned*)&type))
        return false;

    if (size == 1) {
        size = 0;
        if (!read8(&size))
            return false;
        m_data -= 8;
        m_leftLength += 8;
    }

    m_data -= 8;
    m_leftLength += 8;
    return true;
}

bool Box::parseNextBox(Box* nextBox)
{
    if (!nextBox)
        return false;

    if (nextBox->parseBox()) {
        m_data += nextBox->size();
        m_leftLength -= nextBox->size();
        return true;
    }
    return false;
}

bool Box::parseChildrenBox()
{
    unsigned type;
    unsigned long long size;
    while (m_leftLength > 0 && getChildBoxInfo(size, type)) {
        Box* nextBox = getChild(type, size);
        if (!nextBox && type == BoxTypeAsIntT<'u', 'u', 'i', 'd'>::Val) {
            // Note: special case, as UUID boxes can appear anywhere, as they are described by other extensions
            // Even if we don't expect them in appropriate boxes (so we enter this if), their existence is not error.
            m_data += size;
            m_leftLength -= size;
            continue;
        }
        if (!nextBox) {
            TIZEN_LOGE("MP4Parser: NOT SUPPORTED BOX IN %s CONTAINER: %s", BoxTypeAsString(m_type).data(), BoxTypeAsString(type).data());
            return false;
        }
        if (!parseNextBox(nextBox)) {
            TIZEN_LOGE("MP4Parser: TYPE FAILED %s IN BOX TYPE %s\n", BoxTypeAsString(type).data(), BoxTypeAsString(m_type).data());
            return false;
        }
    }
    return true;
}

bool Box::parseBox()
{
    if (!read4(&m_size))
        return false;

    if (!read4(&m_type))
        return false;

    if (m_size == 1) {
        if (!read8(&m_largeSize))
            return false;
        m_leftLength = m_largeSize - 16;
    } else {
        m_largeSize = m_size;
        m_leftLength = m_largeSize - 8;
    }
    return true;
}

template<typename T> bool Box::read(T* outputValue, unsigned length)
{
    if (length > m_leftLength || !outputValue)
        return false;

    if (readT(*outputValue, length, m_data)) {
        m_data += length;
        m_leftLength -= length;
        return true;
    }

    return false;
}

template<typename T> bool Box::read1(T* outputValue)
{
    return read(outputValue, 1);
}

template<typename T> bool Box::read2(T* outputValue)
{
    return read(outputValue, 2);
}

template<typename T> bool Box::read3(T* outputValue)
{
    return read(outputValue, 3);
}

template<typename T> bool Box::read4(T* outputValue)
{
    return read(outputValue, 4);
}

template<typename T> bool Box::read8(T* outputValue)
{
    return read(outputValue, 8);
}

bool FtypBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'f', 't', 'y', 'p'>::Val)
        return false;
    read4(&m_brand);
    read4(&m_version);

    unsigned compatibleBrand;
    while (m_leftLength > 0) {
        compatibleBrand = 0;
        if (!read4(&compatibleBrand))
            return false;
        m_compatibleBrands.append(compatibleBrand);
    }
    return true;
}

bool FullBox::parseBox()
{
    Box::parseBox();
    if (!read1(&m_version))
        return false;
    return read3(&m_flags);
}

bool TrefBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'t', 'r', 'e', 'f'>::Val)
        return false;
    return true;
}

bool ElstBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'e', 'l', 's', 't'>::Val)
        return false;
    read4(&m_entryCount);
    ElstEntry entry = { 0, 0, 0, 0 };
    if (m_version) {
        for (unsigned i = 0; i < m_entryCount; i++) {
            read8(&entry.m_segmentDuration);
            read8(&entry.m_mediaTime);
            read2(&entry.m_mediaRateInteger);
            read2(&entry.m_mediaRateFraction);
            m_entries.append(entry);
        }
    } else {
        for (unsigned i = 0; i < m_entryCount; i++) {
            read4(&entry.m_segmentDuration);
            read4(&entry.m_mediaTime);
            read2(&entry.m_mediaRateInteger);
            read2(&entry.m_mediaRateFraction);
            m_entries.append(entry);
        }
    }
    return true;
}

Box* EdtsBox::getChild(unsigned type, unsigned long long size)
{
    switch (type) {
    case BoxTypeAsIntT<'e', 'l', 's', 't'>::Val:
        return (m_elst = adoptRef(new ElstBox(m_data, size))).get();
        break;
    default:
        return 0;
    }
}

bool EdtsBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'e', 'd', 't', 's'>::Val)
        return false;

    return Box::parseChildrenBox();
}

bool MdhdBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'m', 'd', 'h', 'd'>::Val)
        return false;

    if (m_version) {
        read8(&m_creationTime);
        read8(&m_modificationTime);
        read4(&m_timescale);
        read8(&m_duration);
    } else {
        read4(&m_creationTime);
        read4(&m_modificationTime);
        read4(&m_timescale);
        read4(&m_duration);
    }

    unsigned tmp;
    read1(&tmp);
    m_language[2] = tmp & 0x1F;
    tmp >>= 5;
    m_language[1] = tmp & 0x1F;
    tmp >>= 5;
    m_language[0] = tmp & 0x1F;

    read2(&m_preDefined);

    return true;
}

bool HdlrBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'h', 'd', 'l', 'r'>::Val)
        return false;

    read4(&m_preDefined);
    read4(&m_handlerType);
    unsigned reserved;
    read4(&reserved);
    read4(&reserved);
    read4(&reserved);

    char name;
    while (read1(&name) && name)
        m_name.append(name);

    return true;
}

bool VmhdBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'v', 'm', 'h', 'd'>::Val)
        return false;
    read2(&m_graphicsMode);
    read2(&m_opcolor[0]);
    read2(&m_opcolor[1]);
    return read2(&m_opcolor[2]);
}

bool SmhdBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'s', 'm', 'h', 'd'>::Val)
        return false;
    unsigned reserved;
    read2(&m_balance);
    return read2(&reserved);
}

bool HmhdBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'h', 'm', 'h', 'd'>::Val)
        return false;

    read2(&m_maxPDUSize);
    read2(&m_avgPDUSize);
    read4(&m_maxBitRate);
    read4(&m_avgBitRate);
    unsigned reserved;
    return read4(&reserved);
}

bool NmhdBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'n', 'm', 'h', 'd'>::Val)
        return false;

    return true;
}

bool DinfBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'d', 'i', 'n', 'f'>::Val)
        return false;
    // todo parse url, urn, dref boxes
    return true;
}

bool SttsBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'s', 't', 't', 's'>::Val)
        return false;

    if (!read4(&m_entryCount))
        return false;

    for (unsigned i = 0; i < m_entryCount; i++) {
        SampleCountDelta tmp = {0, 0};
        read4(&(tmp.m_sampleCount));
        if (!read4(&(tmp.m_sampleDelta)))
            return false;

        m_entries.append(tmp);
    }

    return true;
}

bool CttsBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'c', 't', 't', 's'>::Val)
        return false;

    if (!read4(&m_entryCount))
        return false;

    for (unsigned i = 0; i < m_entryCount; i++) {
        SampleCountDelta tmp = {0, 0};
        read4(&(tmp.m_sampleCount));
        if (!read4(&(tmp.m_sampleDelta)))
            return false;

        m_entries.append(tmp);
    }

    return true;
}

bool StszBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'s', 't', 's', 'z'>::Val)
        return false;

    if (!read4(&m_sampleSize))
        return false;

    if (!read4(&m_sampleCount))
        return false;

    if (m_sampleSize)
        return true;

    m_entrySize.resize(m_sampleSize);

    for (unsigned i = 0; i < m_sampleCount; i++) {
        if (!read4(&m_entrySize[i]))
            return false;
    }

    return true;
}

bool SampleEntry::parseBox(const unsigned char* data, unsigned length)
{
    Box::m_data = (unsigned char*) data;
    Box::m_leftLength = length;
    Box::parseBox();
    unsigned reserved;
    read4(&reserved);
    read2(&reserved);
    return read2(&m_dataReferenceIndex);

}

bool HintSampleEntry::parseBox(const unsigned char* data, unsigned length)
{
    if (!SampleEntry::parseBox(data, length))
        return false;

    if (!m_leftLength)
        return true;

    m_data.resize(m_leftLength);
    for (unsigned i = 0; i < m_leftLength; i++) {
        if (!read1(&m_data[i]))
            return false;
    }
    return true;
}

bool VisualSampleEntry::parseBox(const unsigned char* data, unsigned length)
{
    if (!SampleEntry::parseBox(data, length))
        return false;

    unsigned reserved;
    read2(&m_preDefined[0]);
    read2(&reserved);
    for (int i = 1; i < 4; i++)
        read4(&m_preDefined[i]);

    read2(&m_width);
    read2(&m_height);
    read4(&m_horizResolution);
    read4(&m_vertResolution);
    read4(&reserved);
    read2(&m_frameCount);
    m_compressorname[32] = 0;

    for (int i = 0; i < 32; i++)
        read1(&m_compressorname[i]);

    read2(&m_depth);
    return read2(&m_preDefined[4]);
}

bool AudioSampleEntry::parseBox(const unsigned char* data, unsigned length)
{
    if (!SampleEntry::parseBox(data, length))
        return false;

    unsigned reserved;
    read4(&reserved);
    read4(&reserved);
    read2(&m_channelCount);
    read2(&m_sampleSize);
    read2(&m_preDefined);
    read2(&reserved);
    return read4(&m_sampleRate);
}

bool FrmaBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'f', 'r', 'm', 'a'>::Val)
        return false;

    return read4(&m_dataFormat);
}

bool SchmBox::parseBox()
{
    read4(&m_schemeType);
    bool retVal = read4(&m_schemeVersion);

    if (m_flags & 0x000001) {
        m_schemeUri.resize(m_leftLength);
        unsigned i = 0;
        while (m_leftLength) {
            retVal = read1(&m_schemeUri[i]);
            i++;
        }
    }
    return retVal;
}

bool TencBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != getExpectedType())
        return false;
    read3(&m_defaultIsEncrypted);
    bool retVal = read1(&m_defaultIVSize);

    for (int i = 0; i < 16; i++)
        retVal = read1(&m_defaultKID[i]);
    return retVal;
}

Box* SchiBox::getChild(unsigned type, unsigned long long size)
{
    Box* newBox = 0;

    if (type == BoxTypeAsIntT<'t', 'e', 'n', 'c'>::Val)
        newBox = (m_tenc = adoptRef(new TencBox(m_data, size))).get();
    else {
        newBox = new Box(m_data, size);
        m_schemeSpecData.append(newBox);
    }
    return newBox;
}

bool SchiBox::parseBox()
{
    Box::parseBox();
    if (m_type != getExpectedType())
        return false;

    return Box::parseChildrenBox();
}

Box* SinfBox::getChild(unsigned type, unsigned long long size)
{
    switch (type) {
    case BoxTypeAsIntT<'f', 'r', 'm', 'a'>::Val:
        return (m_frma = adoptRef(new FrmaBox(m_data, size))).get();
    case BoxTypeAsIntT<'i', 'm', 'i', 'f'>::Val:
        return (m_imif = adoptRef(new ImifBox(m_data, size))).get();
    case BoxTypeAsIntT<'s', 'c', 'h', 'm'>::Val:
        return (m_schm = adoptRef(new SchmBox(m_data, size))).get();
    case BoxTypeAsIntT<'s', 'c', 'h', 'i'>::Val:
        return (m_schi = adoptRef(new SchiBox(m_data, size))).get();
    default:
        return 0;
    }
}

bool SinfBox::parseBox()
{
    Box::parseBox();
    if (m_type != getExpectedType())
        return false;

    return Box::parseChildrenBox();
}

bool StsdBox::parseNextSampleEntry(SampleEntry* entry)
{
    if (entry->parseBox(m_data, m_leftLength)) {
        m_data += entry->size();
        m_leftLength -= entry->size();
        return true;
    }
    return false;
}

bool StsdBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'s', 't', 's', 'd'>::Val)
        return false;

    if (!read4(&m_entryCount))
        return false;

    switch (m_trackType) {
    case BoxTypeAsIntT<'s', 'o', 'u', 'n'>::Val:
        for (size_t i = 0 ; i < m_entryCount; i++)
            m_sampleEntries.append(adoptRef(new AudioSampleEntry));
        break;
    case BoxTypeAsIntT<'v', 'i', 'd', 'e'>::Val:
        for (size_t i = 0 ; i < m_entryCount; i++)
            m_sampleEntries.append(adoptRef(new VisualSampleEntry));
        break;
    case BoxTypeAsIntT<'h', 'i', 'n', 't'>::Val:
        for (size_t i = 0 ; i < m_entryCount; i++)
            m_sampleEntries.append(adoptRef(new HintSampleEntry));
        break;
    default:
        return false;
    }
    for (unsigned i = 0; i< m_entryCount; i++) {
        if (!parseNextSampleEntry( m_sampleEntries[i].get() ))
            return false;
    }

    return true;
}

bool StblBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'s', 't', 'b', 'l'>::Val)
        return false;

    unsigned type;
    unsigned long long size;
    while (m_leftLength > 0 && Box::getChildBoxInfo(size, type)) {
        Box* nextBox = 0;
        switch (type) {
        case BoxTypeAsIntT<'s', 't', 's', 'd'>::Val:
            nextBox = (m_stsd = adoptRef(new StsdBox(m_trackType, m_data, size))).get();
            break;
        case BoxTypeAsIntT<'s', 't', 't', 's'>::Val:
            nextBox = (m_stts = adoptRef(new SttsBox(m_data, size))).get();
            break;
        case BoxTypeAsIntT<'c', 't', 't', 's'>::Val:
            nextBox = (m_ctts = adoptRef(new CttsBox(m_data, size))).get();
            break;
        case BoxTypeAsIntT<'s', 't', 's', 'z'>::Val:
            nextBox = (m_stsz = adoptRef(new StszBox(m_data, size))).get();
            break;
        case BoxTypeAsIntT<'s', 't', 's', 'c'>::Val:
        case BoxTypeAsIntT<'s', 't', 'z', '2'>::Val:
        case BoxTypeAsIntT<'s', 't', 'c', 'o'>::Val:
        case BoxTypeAsIntT<'c', 'o', '6', '4'>::Val:
        case BoxTypeAsIntT<'s', 't', 's', 'h'>::Val:
        case BoxTypeAsIntT<'s', 't', 's', 's'>::Val:
        case BoxTypeAsIntT<'p', 'a', 'd', 'b'>::Val:
        case BoxTypeAsIntT<'s', 'd', 't', 'p'>::Val:
        case BoxTypeAsIntT<'s', 'b', 'g', 'p'>::Val:
        case BoxTypeAsIntT<'s', 'g', 'p', 'd'>::Val:
        case BoxTypeAsIntT<'s', 'u', 'b', 's'>::Val:
        case BoxTypeAsIntT<'u', 'u', 'i', 'd'>::Val:
            m_data += size;
            m_leftLength -= size;
            break;
        default:
            TIZEN_LOGE("MP4Parser: NOT SUPPORTED BOX IN STBL CONTAINER: %s", BoxTypeAsString(type).data());
            return false;
        }
        if (nextBox && !Box::parseNextBox(nextBox)) {
            TIZEN_LOGE("MP4Parser: TYPE FAILED %X\n", type);
            return false;
        }
    }
    return true;
}

Box* MinfBox::getChild(unsigned type, unsigned long long size)
{
    switch (type) {
    case BoxTypeAsIntT<'v', 'm', 'h', 'd'>::Val:
        return (m_mhd = adoptRef(new VmhdBox(m_data, size))).get();
    case BoxTypeAsIntT<'s', 'm', 'h', 'd'>::Val:
        return (m_mhd = adoptRef(new SmhdBox(m_data, size))).get();
    case BoxTypeAsIntT<'h', 'm', 'h', 'd'>::Val:
        return (m_mhd = adoptRef(new HmhdBox(m_data, size))).get();
    case BoxTypeAsIntT<'n', 'm', 'h', 'd'>::Val:
        return (m_mhd = adoptRef(new NmhdBox(m_data, size))).get();
    case BoxTypeAsIntT<'d', 'i', 'n', 'f'>::Val:
        return (m_dinf = adoptRef(new DinfBox(m_data, size))).get();
    case BoxTypeAsIntT<'s', 't', 'b', 'l'>::Val:
        return (m_stbl = adoptRef(new StblBox(m_parent->mediaType(), m_data, size))).get();
    default:
        return 0;
    }
}

bool MinfBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'m', 'i', 'n', 'f'>::Val)
        return false;

    return Box::parseChildrenBox();
}

unsigned MdiaBox::mediaType()
{
    if (m_hdlr)
        return m_hdlr->handlerType();
    return 0;
}

unsigned long long MdiaBox::duration()
{
    if (m_mdhd)
        return m_mdhd->duration();

    return 0;
}

Box* MdiaBox::getChild(unsigned type, unsigned long long size)
{
    switch (type) {
    case BoxTypeAsIntT<'m', 'd', 'h', 'd'>::Val:
        return (m_mdhd = adoptRef(new MdhdBox(m_data, size))).get();
    case BoxTypeAsIntT<'h', 'd', 'l', 'r'>::Val:
        return (m_hdlr = adoptRef(new HdlrBox(m_data, size))).get();
    case BoxTypeAsIntT<'m', 'i', 'n', 'f'>::Val:
        return (m_minf = adoptRef(new MinfBox(this, m_data, size))).get();
    default:
        return 0;
    }
}

bool MdiaBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'m', 'd', 'i', 'a'>::Val)
        return false;

    return Box::parseChildrenBox();
}

bool MvhdBox::parseBox()
{
    FullBox::parseBox();

    if (m_type != BoxTypeAsIntT<'m', 'v', 'h', 'd'>::Val)
        return false;

    if (m_version) {
        read8(&m_creationTime);
        read8(&m_modificationTime);
        read4(&m_timescale);
        read8(&m_duration);
    } else {
        read4(&m_creationTime);
        read4(&m_modificationTime);
        read4(&m_timescale);
        read4(&m_duration);
    }

    read4(&m_rate);
    read2(&m_volume);
    int reserved; // just ommit reserved data
    read2(&reserved); // const bit(16) reserved =0;
    read4(&reserved); // const unsigned(32)[2] reserved = 0;
    read4(&reserved);

    for (int i = 0; i < 9; i++)
        read4(&m_matrix[i]);

    for (int i = 0; i < 6; i++)
        read4(&m_preDefined[i]);

    return read4(&m_nextTrackID);
}

bool TkhdBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'t', 'k', 'h', 'd'>::Val)
        return false;

    unsigned reserved;
    if (m_version) {
        read8(&m_creationTime);
        read8(&m_modificationTime);
        read4(&m_trackID);
        read4(&reserved);
        read8(&m_duration);
    } else {
        read4(&m_creationTime);
        read4(&m_modificationTime);
        read4(&m_trackID);
        read4(&reserved);
        read4(&m_duration);
    }

    read4(&reserved);
    read4(&reserved);

    read2(&m_layer);
    read2(&m_alternateGroup);
    read2(&m_volume);
    read2(&reserved);

    for (int i = 0; i < 9; i++)
        read4(&m_matrix[i]);

    read4(&m_width);
    return read4(&m_height);
}

unsigned long long TrakBox::duration()
{
    if (m_mdia)
        return m_mdia->duration();

    return 0;
}

Box* TrakBox::getChild(unsigned type, unsigned long long size)
{
    switch (type) {
    case BoxTypeAsIntT<'t', 'k', 'h', 'd'>::Val:
        return (m_tkhd = adoptRef(new TkhdBox(m_data, size))).get();
        break;
    case BoxTypeAsIntT<'t', 'r', 'e', 'f'>::Val:
        return (m_tref = adoptRef(new TrefBox(m_data, size))).get();
        break;
    case BoxTypeAsIntT<'e', 'd', 't', 's'>::Val:
        return (m_edts = adoptRef(new EdtsBox(m_data, size))).get();
        break;
    case BoxTypeAsIntT<'m', 'd', 'i', 'a'>::Val:
        return (m_mdia = adoptRef(new MdiaBox(m_data, size))).get();
        break;
    default:
        return 0;
    }
}

bool TrakBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'t', 'r', 'a', 'k'>::Val)
        return false;

    return Box::parseChildrenBox();
}

bool MehdBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'m', 'e', 'h', 'd'>::Val)
        return false;
    if (m_version)
        return read8(&m_fragmentDuration);
    return read4(&m_fragmentDuration);
}

bool TrexBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'t', 'r', 'e', 'x'>::Val)
        return false;
    bool retVal = read4(&m_trackID);
    retVal &= read4(&m_sampleDescriptionIndex);
    retVal &= read4(&m_defaultSampleDuration);
    retVal &= read4(&m_defaultSampleSize);
    retVal &= read4(&m_defaultSampleFlags);
    return retVal;
}

bool MvexBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'m', 'v', 'e', 'x'>::Val)
        return false;
    return Box::parseChildrenBox();
}

Box* MvexBox::getChild(const unsigned type, const unsigned long long size)
{
    switch (type) {
    case BoxTypeAsIntT<'m', 'e', 'h', 'd'>::Val:
        return (m_mehd = adoptRef(new MehdBox(m_data, size))).get();
    case BoxTypeAsIntT<'t', 'r', 'e', 'x'>::Val:
        m_trex.append(adoptRef(new TrexBox(m_data, m_leftLength)));
        return m_trex.last().get();
    default: // we don't care for other boxes yet
        return 0;
    }
}

bool IpmcBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'i', 'p', 'm', 'c'>::Val)
        return false;
    return true;
}

bool PsshBox::parseBox()
{
    FullBox::parseBox();
    if (m_type != BoxTypeAsIntT<'p', 's', 's', 'h'>::Val)
        return false;

    for (int i = 0; i < 16; i++)
        read1(&m_systemId[i]);

    if (!read4(&m_rawDataSize))
        return false;

    char tmp;
    for (unsigned i = 0; i < m_rawDataSize; i++) {
        if (!read1(&tmp))
            return false;
        m_rawData.append(tmp);
    }

    return true;
}

bool UdtaBox::parseBox()
{
    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'u', 'd', 't', 'a'>::Val)
        return false;
    return true;
}

Box* MoovBox::getChild(unsigned type, unsigned long long size)
{
    switch (type) {
    case BoxTypeAsIntT<'m', 'v', 'h', 'd'>::Val:
        return (m_mvhd = adoptRef(new MvhdBox(m_data, size))).get();
    case BoxTypeAsIntT<'m', 'v', 'e', 'x'>::Val:
        return (m_mvex = adoptRef(new MvexBox(m_data, size))).get();
    case BoxTypeAsIntT<'i', 'p', 'm', 'c'>::Val:
        return (m_ipmc = adoptRef(new IpmcBox(m_data, size))).get();
    case BoxTypeAsIntT<'u', 'd', 't', 'a'>::Val:
        return (m_udta = adoptRef(new UdtaBox(m_data, size))).get();
    case BoxTypeAsIntT<'t', 'r', 'a', 'k'>::Val:
        m_trak.append(adoptRef(new TrakBox(m_data, m_leftLength)));
        return m_trak.last().get();
    case BoxTypeAsIntT<'p', 's', 's', 'h'>::Val:
        m_pssh.append(adoptRef(new PsshBox(m_data, m_leftLength)));
        return m_pssh.last().get();
    default:
        return 0;
    }
}

double MoovBox::duration()
{
    if (m_mvhd && m_mvhd->duration())
        return m_mvhd->duration();

    return std::numeric_limits<double>::infinity();
}

bool MoovBox::parseBox()
{
    m_rawData.clear();
    m_rawData.reserveCapacity(m_leftLength);
    m_rawData.append(m_data, m_leftLength);

    Box::parseBox();
    if (m_type != BoxTypeAsIntT<'m', 'o', 'o', 'v'>::Val)
        return false;

    return Box::parseChildrenBox();
}

bool TfhdBox::parseBox()
{
    if (!FullBox::parseBox() || m_type != BoxTypeAsIntT<'t', 'f', 'h', 'd'>::Val)
        return false;
    bool retVal = read4(&m_trackID);

    if (baseDataOffsetPresent())
        retVal &= read8(&m_baseDataOffset);
    if (sampleDescriptionIndexPresent())
        retVal &= read4(&m_sampleDescriptionIndex);
    if (defaultDurationPesent())
        retVal &= read4(&m_defaultSampleDuration);
    if (defaultSampleSizePresent())
        retVal &= read4(&m_defaultSampleSize);
    if (defaultSampleFlagsPresent())
        retVal &= read4(&m_defaultSampleFlags);

    return retVal;
}

bool TrunBox::parseBox()
{
    m_populated = false;
    if (!FullBox::parseBox() || m_type != BoxTypeAsIntT<'t', 'r', 'u', 'n'>::Val)
        return false;

    if (!read4(&m_sampleCount))
        return false;

    if (dataOffsetPresent())
        read4(&m_dataOffset);

    if (firstSampleFlagsPresent())
        read4(&m_firstSampleFlags);

    m_samples = new TrunSampleEntry[m_sampleCount];

    bool retVal = true;
    for (unsigned i = 0 ; i < m_sampleCount; i++) {
        if (sampleDurationPresent())
            retVal &= read4(&(m_samples[i].m_sampleDuration));
        if (sampleSizePresent())
            retVal &= read4(&(m_samples[i].m_sampleSize));
        if (sampleFlagsPresent())
            retVal &= read4(&(m_samples[i].m_sampleFlags));
        if (sampleCompositionTimeOffsetPresent())
            retVal &= read4(&(m_samples[i].m_sampleCompositionTimeOffset));
    }
    return retVal;
}

void TrunBox::populateDefaultsForSamples(const MoovBox* moov, const TfhdBox* tfhd)
{
    if (!moov || !tfhd)
        return;

    // FIXME: need to handle right track in multitrack scenario
    TrexBox* trex = (moov->m_mvex) ? moov->m_mvex->m_trex[0].get() : 0;

    // if there is no info for sample, it has to be in 'tfhd' (most likely), or 'trex' (less likely as it's non mandatory field)
    for (unsigned i = 0 ; i < m_sampleCount; i++) {
        if (!sampleDurationPresent()) {
            if (tfhd->defaultDurationPesent())
                m_samples[i].m_sampleDuration = tfhd->m_defaultSampleDuration;
            else if (trex)
                m_samples[i].m_sampleDuration = trex->m_defaultSampleDuration;
        }
        if (!sampleSizePresent()) {
            if (tfhd->defaultSampleSizePresent())
                m_samples[i].m_sampleSize = tfhd->m_defaultSampleSize;
            else if (trex)
                m_samples[i].m_sampleSize = trex->m_defaultSampleSize;
        }
        if (!sampleFlagsPresent()) {
            if (tfhd->defaultSampleFlagsPresent())
                m_samples[i].m_sampleFlags = tfhd->m_defaultSampleFlags;
            else if (trex)
                m_samples[i].m_sampleFlags = trex->m_defaultSampleFlags;
        }
    }
    if (m_sampleCount > 0 && firstSampleFlagsPresent())
        m_samples[0].m_sampleFlags = m_firstSampleFlags;

    m_populated = true;
}

bool SencBox::parseBox()
{
    if (!FullBox::parseBox() || m_type != BoxTypeAsIntT<'s', 'e', 'n', 'c'>::Val)
        return false;
    return read4(&m_sampleCount);
    // TODO: missing impl;
}

bool SaioBox::parseBox()
{
    if (!FullBox::parseBox() || m_type != BoxTypeAsIntT<'s', 'a', 'i', 'o'>::Val)
        return false;

    if (m_flags & 1) {
        read4(&m_aux_info_type);
        read4(&m_aux_info_parameter);
    }
    read4(&m_entry_count);

    unsigned long long temporaryOffset;
    for (size_t i = 0; i < m_entry_count; i++) {
        if (!m_version)
            read4(&temporaryOffset);
        else
            read8(&temporaryOffset);
        m_offset_table.append(temporaryOffset);
    }
    return true;
}

bool SaizBox::parseBox()
{
    if (!FullBox::parseBox() || m_type != BoxTypeAsIntT<'s', 'a', 'i', 'z'>::Val)
        return false;

    if (m_flags & 1) {
        read4(&m_aux_info_type);
        read4(&m_aux_info_parameter);
    }

    read1(&m_default_sample_info_size);
    read4(&m_sample_count);

    unsigned char tempSampleInfoSize;
    for (size_t i = 0; i < m_sample_count; i++) {
        tempSampleInfoSize = m_default_sample_info_size ? m_default_sample_info_size : read1(&tempSampleInfoSize);
        m_sample_info_size.append(tempSampleInfoSize);
        m_totalSize += tempSampleInfoSize;
    }
    return true;
}

bool TfdtBox::parseBox()
{
    if (!FullBox::parseBox() || m_type != getExpectedType())
        return false;
    if (m_version)
        return read8(&m_baseMediaDecodeTime);
    return read4(&m_baseMediaDecodeTime);
}

bool TrafBox::parseBox()
{
    if (!Box::parseBox() || m_type != BoxTypeAsIntT<'t', 'r', 'a', 'f'>::Val)
        return false;

    unsigned type;
    unsigned long long size;
    while (m_leftLength > 0 && Box::getChildBoxInfo(size, type)) {
        Box* nextBox = 0;
        switch (type) {
        case BoxTypeAsIntT<'t', 'f', 'h', 'd'>::Val:
            nextBox = (m_tfhd = adoptRef(new TfhdBox(m_data, size))).get();
            break;
        case BoxTypeAsIntT<'t', 'r', 'u', 'n'>::Val:
            m_trun.append(adoptRef(new TrunBox(m_data, size)));
            nextBox = m_trun.last().get();
            break;
        case BoxTypeAsIntT<'s', 'a', 'i', 'o'>::Val:
            m_saio.append(adoptRef(new SaioBox(m_data, size)));
            nextBox = m_saio.last().get();
            break;
        case BoxTypeAsIntT<'s', 'a', 'i', 'z'>::Val:
            m_saiz.append(adoptRef(new SaizBox(m_data, size)));
            nextBox = m_saiz.last().get();
            break;
        case BoxTypeAsIntT<'s', 'e', 'n', 'c'>::Val:
            nextBox = (m_senc = adoptRef(new SencBox(m_data, size))).get();
            break;
        case BoxTypeAsIntT<'t', 'f', 'd', 't'>::Val:
            nextBox = (m_tfdt = adoptRef(new TfdtBox(m_data, size))).get();
            break;
        case BoxTypeAsIntT<'s', 'd', 't', 'p'>::Val:
        case BoxTypeAsIntT<'s', 'b', 'g', 'p'>::Val:
        case BoxTypeAsIntT<'s', 'u', 'b', 's'>::Val:
            m_data += size;
            m_leftLength -= size;
            break;
        default:
            TIZEN_LOGE("MP4Parser: NOT SUPPORTED BOX IN TRAF CONTAINER: %X", type);
            return false;
        }
        if (nextBox && !Box::parseNextBox(nextBox)) {
            TIZEN_LOGE("MP4Parser: TYPE FAILED IN TRAF %X\n", type);
            return false;
        }
    }
    return true;
}

bool MfhdBox::parseBox()
{
    if (!FullBox::parseBox() || m_type != BoxTypeAsIntT<'m', 'f', 'h', 'd'>::Val)
        return false;
    return read4(&m_sequenceNumber);
}

bool MoofBox::parseBox()
{
    m_rawData.clear();
    m_rawData.reserveCapacity(m_leftLength);
    m_rawData.append(m_data, m_leftLength);

    if (!Box::parseBox() || m_type != BoxTypeAsIntT<'m', 'o', 'o', 'f'>::Val)
        return false;

    unsigned type;
    unsigned long long size;
    while (m_leftLength > 0 && Box::getChildBoxInfo(size, type)) {
        Box* nextBox = 0;
        switch (type) {
        case BoxTypeAsIntT<'m', 'f', 'h', 'd'>::Val:
            nextBox = (m_mfhd = adoptRef(new MfhdBox(m_data, size))).get();
            break;
        case BoxTypeAsIntT<'t', 'r', 'a', 'f'>::Val:
            m_traf.append(adoptRef(new TrafBox(m_data, size)));
            nextBox = m_traf.last().get();
            break;
        case BoxTypeAsIntT<'p', 's', 's', 'h'>::Val:
            m_pssh.append(adoptRef(new PsshBox(m_data, m_leftLength)));
            nextBox = m_pssh.last().get();
            break;
        case BoxTypeAsIntT<'u', 'u', 'i', 'd'>::Val:
            m_data += size;
            m_leftLength -= size;
            break;
        default:
            TIZEN_LOGE("MP4Parser: NOT SUPPORTED BOX IN MOOF CONTAINER: %X", type);
            return false;
        }
        if (!Box::parseNextBox(nextBox)) {
            TIZEN_LOGE("MP4Parser: TYPE FAILED IN MOOF %X\n", type);
            return false;
        }
    }
    return true;
}

bool MdatBox::parseBox()
{
    if (!Box::parseBox() || m_type != BoxTypeAsIntT<'m', 'd', 'a', 't'>::Val)
        return false;

    return true;
}

bool MfroBox::parseBox()
{
    if (!FullBox::parseBox() || m_type != BoxTypeAsIntT<'m', 'f', 'r', 'o'>::Val)
        return false;

    return read4(&m_localSize);
}

bool MfraBox::parseBox()
{
    if (!Box::parseBox() || m_type != BoxTypeAsIntT<'m', 'f', 'r', 'a'>::Val)
        return false;

    unsigned type;
    unsigned long long size;
    while (m_leftLength > 0 && Box::getChildBoxInfo(size, type)) {
        Box* nextBox = 0;
        switch (type) {
        case BoxTypeAsIntT<'m', 'f', 'r', 'o'>::Val:
            nextBox = (m_mfro = adoptRef(new MfroBox(m_data, size))).get();
            break;
        case BoxTypeAsIntT<'t', 'f', 'r', 'a'>::Val:
            m_data += size;
            m_leftLength  -= size;
            break;
        default:
            TIZEN_LOGE("MP4Parser: NOT SUPPORTED BOX IN FILE: %X", type);
            return false;
        }
        if (nextBox && !Box::parseNextBox(nextBox)) {
            TIZEN_LOGE("MP4Parser: TYPE FAILED in mfra%X\n", type);
            return false;
        }
    }
    return true;
}

bool PitmBox::parseBox()
{
    if (!FullBox::parseBox() || m_type != getExpectedType())
        return false;
    return read2(&m_itemID);
}

Box* MetaBox::getChild(unsigned type, unsigned long long size)
{
    switch (type) {
    case BoxTypeAsIntT<'h', 'd', 'l', 'r'>::Val:
        return (m_hdlr = adoptRef(new HdlrBox(m_data, size))).get();
    case BoxTypeAsIntT<'p', 'i', 't', 'm'>::Val:
        return (m_pitm = adoptRef(new PitmBox(m_data, size))).get();
    case BoxTypeAsIntT<'d', 'i', 'n', 'f'>::Val:
        return (m_dinf = adoptRef(new DinfBox(m_data, size))).get();
    case BoxTypeAsIntT<'i', 'l', 'o', 'c'>::Val:
    case BoxTypeAsIntT<'i', 'p', 'r', 'o'>::Val:
    default:
        return 0;
    }
}

bool MetaBox::parseBox()
{
    if (!Box::parseBox() || m_type != getExpectedType())
        return false;
    return Box::parseChildrenBox();
}


bool MP4Parser::readSizeAndType(unsigned long long& size, unsigned &type) {
    size = 0;
    type = 0;

    if (m_leftLength < MP4BoxHeaderSize)
        return false;

    if (!readT(size, 4, m_data) || !readT(type, 4, m_data + 4))
        return false;

    if (size == 1) {
        size = 0;
        if (m_leftLength < 16 || !readT(size, 8, m_data + 8))
            return false;
    }

    return true;
}

PassRefPtr<Box> MP4Parser::getNextBox()
{
    RefPtr<Box> result;

    if (!m_leftLength)
        return result;

    if (m_leftLength < MP4BoxHeaderSize) { // there is only part of header, unknown type and size.
        TIZEN_LOGI("MP4Parser: file lenght to small. Corrupted file?");
        result = adoptRef(new Box());
        m_readLength = m_leftLength;
        m_leftLength = 0;
        return result;
    }

    unsigned type;
    unsigned long long size;

    if (readSizeAndType(size, type)) {

        switch (type) {
        case BoxTypeAsIntT<'f', 't', 'y', 'p'>::Val:
            result = adoptRef(new FtypBox(m_data, size));
            break;
        case BoxTypeAsIntT<'m', 'o', 'o', 'v'>::Val:
            result = adoptRef(new MoovBox(m_data, size));
            break;
        case BoxTypeAsIntT<'m', 'd', 'a', 't'>::Val:
            result = adoptRef(new MdatBox(m_data, size));
            break;
        case BoxTypeAsIntT<'m', 'o', 'o', 'f'>::Val:
            result = adoptRef(new MoofBox(m_data, size));
            break;
        case BoxTypeAsIntT<'m', 'f', 'r', 'a'>::Val:
            result = adoptRef(new MfraBox(m_data, size));
            break;
        default:
            result = adoptRef(new Box(m_data, size));
            break;
        }

        if (size > m_leftLength) {
            TIZEN_LOGI("Next box '%s' size (%lld) is longer then data received (%lld)", BoxTypeAsString(type).data(), size, m_leftLength);
            result->Box::parseBox(); // there is not enough data to parse all box, only base Box
            m_readLength = m_leftLength;
            m_leftLength = 0;
            return result;
        }

        if (result->parseBox()) {
            m_data += result->size();
            m_leftLength -= m_readLength = result->size();
        } else {
            m_leftLength = 0;
            m_readLength = result->size();
            TIZEN_LOGE("MP4Parser: TYPE FAILED %s %lld\n", BoxTypeAsString(type).data(), result->size());
        }

    }

    return result;
}


}

