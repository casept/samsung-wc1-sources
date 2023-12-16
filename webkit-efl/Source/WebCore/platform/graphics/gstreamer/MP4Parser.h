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


#ifndef MP4Parser_h
#define MP4Parser_h

#include <wtf/RefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

template < char a, char b, char c, char d>
struct BoxTypeAsIntT {
    enum { Val = a << 24 | b << 16 | c << 8 | d };
};

// According to ISO 14496-12, it's 4 bytes for Box size, and 4 bytes for Box type. We need this minimal amount of data to start parsing.
const unsigned MP4BoxHeaderSize = 8;

class Box : public WTF::RefCounted<Box> {
protected:
    unsigned m_size;
    unsigned long long m_largeSize;
public:
    Box(const unsigned char* data, unsigned long long length);
    Box();

    virtual ~Box() { }

    virtual bool parseBox();
    unsigned long long size() const { return m_largeSize; }
    unsigned type() const { return m_type; }

protected:
    const unsigned char* m_data;
    unsigned long long m_leftLength;
    unsigned m_type;
    template<typename T> bool read(T* outputValue, unsigned length);
    template<typename T> bool read1(T* outputValue);
    template<typename T> bool read2(T* outputValue);
    template<typename T> bool read3(T* outputValue);
    template<typename T> bool read4(T* outputValue);
    template<typename T> bool read8(T* outputValue);
    bool getChildBoxInfo(unsigned long long& size, unsigned &type);
    virtual bool parseNextBox(Box* nextBox);
    virtual Box* getChild(unsigned , unsigned long long) { return 0; }
    virtual unsigned getExpectedType() const { return 0; }
    virtual bool parseChildrenBox();
};

class FtypBox : public Box {
private:
    unsigned m_brand;
    unsigned m_version;
    Vector<unsigned> m_compatibleBrands;
public:
    FtypBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    { }

    virtual bool parseBox();
};

class FullBox : public Box {
protected:
    unsigned char m_version;
    unsigned m_flags;
public:
    FullBox(const unsigned char* data, unsigned length)
        : Box(data, length)
        , m_version(0)
        , m_flags(0)
    { }

    virtual bool parseBox();
};

class TrefBox : public Box {
public:
    TrefBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    { }

    virtual bool parseBox();
};

struct ElstEntry {
    unsigned long long m_segmentDuration;
    long long m_mediaTime;
    int m_mediaRateInteger;
    int m_mediaRateFraction;
};

class ElstBox : public FullBox {
private:
    unsigned m_entryCount;
    Vector<ElstEntry> m_entries;
public:
    ElstBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_entryCount(0)
    {
    }
    virtual bool parseBox();
};

class EdtsBox : public Box {
private:
    RefPtr<ElstBox> m_elst;
public:
    EdtsBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    { }

    virtual bool parseBox();
protected:
    virtual Box* getChild(unsigned type, unsigned long long size);
};

class MdhdBox : public FullBox {
    unsigned long long m_creationTime;
    unsigned long long m_modificationTime;
    unsigned m_timescale;
    unsigned long long m_duration;
    unsigned char m_language[3];
    unsigned m_preDefined;

public:
    MdhdBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_creationTime(0)
        , m_modificationTime(0)
        , m_timescale(0)
        , m_duration(0)
        , m_language { }
        , m_preDefined(0)
    {
    }
    virtual bool parseBox();
    unsigned long long duration() const { return m_duration; }
    unsigned timeScale() const { return m_timescale; }
};

class HdlrBox : public FullBox {
public:
    unsigned m_preDefined;
    unsigned m_handlerType;
    String m_name;

    HdlrBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_preDefined(0)
        , m_handlerType(0)
    {
    }
    virtual bool parseBox();
    unsigned handlerType() { return m_handlerType; }
};

class VmhdBox : public FullBox {
private:
    unsigned m_graphicsMode;
    unsigned m_opcolor[3];
public:
    VmhdBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_graphicsMode(0)
        , m_opcolor { }
    {
    }
    virtual bool parseBox();
};

class SmhdBox : public FullBox {
private:
    unsigned m_balance;
public:
    SmhdBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_balance(0)
    {
    }
    virtual bool parseBox();
};

class HmhdBox : public FullBox {
private:
    unsigned m_maxPDUSize;
    unsigned m_avgPDUSize;
    unsigned m_maxBitRate;
    unsigned m_avgBitRate;
public:
    HmhdBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_maxPDUSize(0)
        , m_avgPDUSize(0)
        , m_maxBitRate(0)
        , m_avgBitRate(0)
    {
    }
    virtual bool parseBox();
};

class NmhdBox : public FullBox {
public:
    NmhdBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
    { }

    virtual bool parseBox();
};

class DinfBox : public Box {
public:
    DinfBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    { }

    virtual bool parseBox();
};

struct SampleCountDelta {
    unsigned m_sampleCount;
    unsigned m_sampleDelta;
};

class SttsBox : public FullBox {
private:
    unsigned m_entryCount;
    Vector<SampleCountDelta> m_entries;
public:
    SttsBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_entryCount(0)
    {
    }
    virtual bool parseBox();
};

class CttsBox : public FullBox {
private:
    unsigned m_entryCount;
    Vector<SampleCountDelta> m_entries;
public:
    CttsBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_entryCount(0)
    {
    }
    virtual bool parseBox();
};

class StszBox : public FullBox {
private:
    unsigned m_sampleSize;
    unsigned m_sampleCount;
    std::vector<unsigned> m_entrySize;
public:
    StszBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_sampleSize(0)
        , m_sampleCount(0)
    {
    }
    virtual bool parseBox();
};

class SampleEntry : public Box {
private:
    unsigned short m_dataReferenceIndex;
public:
    SampleEntry(const unsigned char* data, unsigned length)
        : Box(data, length)
        , m_dataReferenceIndex(0)
    {
    }
    virtual bool parseBox(const unsigned char* data, unsigned length);
};

class HintSampleEntry : public SampleEntry {
private:
    std::vector<unsigned char> m_data;
public:
    HintSampleEntry()
        : SampleEntry(0, 0)
    {
    }
    bool parseBox(const unsigned char* data, unsigned length);

};

class VisualSampleEntry : public SampleEntry {
private:
    unsigned m_preDefined[5];
    unsigned short m_width;
    unsigned short m_height;
    unsigned m_horizResolution;
    unsigned m_vertResolution;
    unsigned short m_frameCount;
    char m_compressorname[33];
    unsigned short m_depth;
public:
    VisualSampleEntry()
        : SampleEntry(0, 0)
        , m_preDefined { }
        , m_width(0)
        , m_height(0)
        , m_horizResolution(0)
        , m_vertResolution(0)
        , m_frameCount(0)
        , m_compressorname { }
        , m_depth(0)
    {
    }
    virtual bool parseBox(const unsigned char* data, unsigned length);
};

class AudioSampleEntry : public SampleEntry {
private:
    unsigned short m_channelCount;
    unsigned short m_sampleSize;
    unsigned short m_preDefined;
    unsigned m_sampleRate;
public:
    AudioSampleEntry()
        : SampleEntry(0, 0)
        , m_channelCount(0)
        , m_sampleSize(0)
        , m_preDefined(0)
        , m_sampleRate(0)
    {
    }
    virtual bool parseBox(const unsigned char* data, unsigned length);
};

class FrmaBox : public Box {
private:
    unsigned m_dataFormat;
public:
    FrmaBox(const unsigned char* data, unsigned length)
        : Box(data, length)
        , m_dataFormat(0)
    {
    }
    virtual bool parseBox();
};

class ImifBox : public FullBox {
public:
    ImifBox(const unsigned char* data, unsigned length):FullBox(data, length) { }
protected:
    virtual unsigned getExpectedType() const { return BoxTypeAsIntT<'i', 'm', 'i', 'f'>::Val; }
};

class SchmBox : public FullBox {
private:
    unsigned m_schemeType;
    unsigned m_schemeVersion;
    std::vector<unsigned char> m_schemeUri;
public:
    SchmBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_schemeType(0)
        , m_schemeVersion(0)
    {
    }
    virtual bool parseBox();
};

class TencBox : public FullBox {
private:
    unsigned m_defaultIsEncrypted;
    unsigned char m_defaultIVSize;
    unsigned char m_defaultKID[16];
public:
    TencBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_defaultIsEncrypted(0)
        , m_defaultIVSize(0)
        , m_defaultKID { }
    {
    }
    virtual bool parseBox();
protected:
    virtual unsigned getExpectedType() const { return BoxTypeAsIntT<'t', 'e', 'n', 'c'>::Val; }
};

class SchiBox : public Box {
private:
    RefPtr<TencBox> m_tenc;
    Vector<RefPtr<Box>> m_schemeSpecData;
public:
    SchiBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    {
    }
    virtual bool parseBox();
protected:
    virtual Box* getChild(unsigned type, unsigned long long size);
    virtual unsigned getExpectedType() const { return BoxTypeAsIntT<'s', 'c', 'h', 'i'>::Val; }
};

class SinfBox : public Box {
private:
    RefPtr<FrmaBox> m_frma;
    RefPtr<ImifBox> m_imif;
    RefPtr<SchmBox> m_schm;
    RefPtr<SchiBox> m_schi;
public:
    SinfBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    {
    }
    virtual bool parseBox();
protected:
    virtual Box* getChild(unsigned type, unsigned long long size);
    virtual unsigned getExpectedType() const { return BoxTypeAsIntT<'s', 'i', 'n', 'f'>::Val; }
};

class StsdBox : public FullBox {
private:
    unsigned m_entryCount;
    unsigned m_trackType;
    Vector<RefPtr<SampleEntry>> m_sampleEntries;
public:
    StsdBox(unsigned trackType, const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_entryCount(0)
        , m_trackType(trackType)
    {
    }
    virtual bool parseBox();
    bool parseNextSampleEntry(SampleEntry*);
};

class StblBox : public Box {
private:
    RefPtr<StsdBox> m_stsd;
    RefPtr<SttsBox> m_stts;
    RefPtr<CttsBox> m_ctts;
    RefPtr<StszBox> m_stsz;
    unsigned m_trackType;
public:
    StblBox(unsigned trackType, const unsigned char* data, unsigned length)
        : Box(data, length)
        , m_trackType(trackType)
    {
    }
    virtual bool parseBox();
};

class MdiaBox;
class MinfBox : public Box {
private:
    RefPtr<FullBox> m_mhd;
    RefPtr<DinfBox> m_dinf;
    RefPtr<StblBox> m_stbl;
    MdiaBox* m_parent;
public:
    MinfBox(MdiaBox * parent, const unsigned char* data, unsigned length)
        : Box(data, length)
        , m_parent(parent)
    {
    }
    virtual bool parseBox();
protected:
    virtual Box* getChild(unsigned type, unsigned long long size);
};

class MdiaBox : public Box {
public:
    RefPtr<MdhdBox> m_mdhd;
    RefPtr<HdlrBox> m_hdlr;
    RefPtr<MinfBox> m_minf;
public:
    MdiaBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    {
    }
    unsigned mediaType();
    unsigned long long duration();
    virtual bool parseBox();
    virtual Box* getChild(unsigned type, unsigned long long size);
};

class MvhdBox : public FullBox {
public:
    unsigned long long m_creationTime;
    unsigned long long m_modificationTime;
    unsigned m_timescale;
    unsigned long long m_duration;
    int m_rate;
    short m_volume;
    int m_matrix[9];
    unsigned m_preDefined[6];
    unsigned m_nextTrackID;

    MvhdBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_creationTime(0)
        , m_modificationTime(0)
        , m_timescale(0)
        , m_duration(0)
        , m_rate(0)
        , m_volume(0)
        , m_matrix { }
        , m_preDefined { }
        , m_nextTrackID(0)
    {
    }
    unsigned long long duration() const { return m_duration; }
    unsigned long long timescale() const { return m_timescale; }
    virtual bool parseBox();
};

class TkhdBox : public FullBox {
private:
    unsigned long long m_creationTime;
    unsigned long long m_modificationTime;
    unsigned m_trackID;
    unsigned long long m_duration;
    short m_layer;
    short m_alternateGroup;
    short m_volume;
    int m_matrix[9];
    unsigned m_width;
    unsigned m_height;
public:
    TkhdBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_creationTime(0)
        , m_modificationTime(0)
        , m_trackID(0)
        , m_duration(0)
        , m_layer(0)
        , m_alternateGroup(0)
        , m_volume(0)
        , m_matrix { }
        , m_width(0)
        , m_height(0)
    {
    }
    virtual bool parseBox();
};

class TrakBox : public Box {
public:
    RefPtr<TkhdBox> m_tkhd;
    RefPtr<TrefBox> m_tref;
    RefPtr<EdtsBox> m_edts;
    RefPtr<MdiaBox> m_mdia;

    TrakBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    {
    }
    unsigned long long duration();
    virtual bool parseBox();
protected:
    virtual Box* getChild(unsigned type, unsigned long long size);
};

class MehdBox : public FullBox {
private:
    unsigned long long m_fragmentDuration;
public:
    MehdBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_fragmentDuration(0)
    {
    }
    virtual bool parseBox();
    unsigned long long fragmentDuration() const { return m_fragmentDuration; }
};

struct TrexBox : public FullBox {
public:
    unsigned m_trackID;
    unsigned m_sampleDescriptionIndex;
    unsigned m_defaultSampleDuration;
    unsigned m_defaultSampleSize;
    unsigned m_defaultSampleFlags;
    TrexBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_trackID(0)
        , m_sampleDescriptionIndex(0)
        , m_defaultSampleDuration(0)
        , m_defaultSampleSize(0)
        , m_defaultSampleFlags(0)
    {
    }
    virtual bool parseBox();
};

struct MvexBox : public Box {
public:
    RefPtr<MehdBox> m_mehd;
    Vector<RefPtr<TrexBox>> m_trex;

    MvexBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    {
    }
    virtual bool parseBox();
protected:
    virtual Box* getChild(const unsigned type, const unsigned long long size);
};

class IpmcBox : public Box {
public:
    IpmcBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    {
    }
    virtual bool parseBox();
};

class PsshBox : public FullBox {
private:
    unsigned char m_systemId[16];
    unsigned m_rawDataSize;
    Vector<char> m_rawData;
public:
    PsshBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_systemId { }
        , m_rawDataSize(0)
    {
    }
    virtual bool parseBox();
};

class UdtaBox : public Box {
public:
    UdtaBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    { }

    virtual bool parseBox();
};

class MoovBox : public Box {
public:
    RefPtr<MvhdBox> m_mvhd;
    RefPtr<MvexBox> m_mvex;
    RefPtr<IpmcBox> m_ipmc;
    RefPtr<UdtaBox> m_udta;
    Vector<RefPtr<TrakBox>> m_trak;
    Vector<RefPtr<PsshBox>> m_pssh;
    Vector<unsigned char> m_rawData;

    MoovBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    {
    }
    double duration();
    virtual bool parseBox();
protected:
    virtual Box* getChild(unsigned type, unsigned long long size);
};

class TfhdBox : public FullBox {
public:
    unsigned m_trackID;
    unsigned long long m_baseDataOffset;
    unsigned m_sampleDescriptionIndex;
    unsigned m_defaultSampleDuration;
    unsigned m_defaultSampleSize;
    unsigned m_defaultSampleFlags;

    TfhdBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_trackID(0)
        , m_baseDataOffset(0)
        , m_sampleDescriptionIndex(0)
        , m_defaultSampleDuration(0)
        , m_defaultSampleSize(0)
        , m_defaultSampleFlags(0)
    {
    }
    virtual bool parseBox();
    bool baseDataOffsetPresent() const         { return m_flags & 0x000001; }
    bool sampleDescriptionIndexPresent() const { return m_flags & 0x000002; }
    bool defaultDurationPesent() const         { return m_flags & 0x000008; }
    bool defaultSampleSizePresent() const      { return m_flags & 0x000010; }
    bool defaultSampleFlagsPresent() const     { return m_flags & 0x000020; }
    bool durationIsEmpty() const               { return m_flags & 0x010000; }
    bool defaultBaseIsMoof() const             { return m_flags & 0x020000; }
};

struct TrunSampleEntry {
    unsigned m_sampleDuration;
    unsigned m_sampleSize;
    unsigned m_sampleFlags;
    unsigned m_sampleCompositionTimeOffset;
    // FIXME: unsigned/signed of composition_time_offset should be treated according to spec
    TrunSampleEntry()
        : m_sampleDuration(0)
        , m_sampleSize(0)
        , m_sampleFlags(0)
        , m_sampleCompositionTimeOffset(0)
    {
    }
};

class TrunBox : public FullBox {
public:
    unsigned m_sampleCount;
    unsigned m_dataOffset;
    unsigned m_firstSampleFlags;
    TrunSampleEntry* m_samples;

    TrunBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_sampleCount(0)
        , m_dataOffset(0)
        , m_firstSampleFlags(0)
        , m_samples(0)
        , m_populated(false)
    {
    }
    virtual ~TrunBox()
    {
        delete [] m_samples;
    }
    virtual bool parseBox();
    bool populated() const { return m_populated; }
    void populateDefaultsForSamples(const MoovBox*, const TfhdBox*);

    inline bool dataOffsetPresent() const                  { return m_flags & 0x000001; }
    inline bool firstSampleFlagsPresent() const            { return m_flags & 0x000004; }
    inline bool sampleDurationPresent() const              { return m_flags & 0x000100; }
    inline bool sampleSizePresent() const                  { return m_flags & 0x000200; }
    inline bool sampleFlagsPresent() const                 { return m_flags & 0x000400; }
    inline bool sampleCompositionTimeOffsetPresent() const { return m_flags & 0x000800; }

private:
    bool m_populated;
};

class SencBox : public FullBox {
private:
    unsigned m_sampleCount;
public:
    SencBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_sampleCount(0)
    {
    }
    virtual bool parseBox();
};

class TfdtBox : public FullBox {
public:
    unsigned long long m_baseMediaDecodeTime;
public:
    TfdtBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_baseMediaDecodeTime(0)
    {
    }
    virtual unsigned getExpectedType() const { return BoxTypeAsIntT<'t', 'f', 'd', 't'>::Val;}
    virtual bool parseBox();
};

class SaioBox : public FullBox {
private:
    unsigned m_aux_info_type;
    unsigned m_aux_info_parameter;
    unsigned m_entry_count;
    Vector<unsigned long long> m_offset_table;
public:
    SaioBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_aux_info_type(0)
        , m_aux_info_parameter(0)
        , m_entry_count(0)
    {
    }
    virtual bool parseBox();
};

struct SaizBox : public FullBox {
public:
    unsigned m_aux_info_type;
    unsigned m_aux_info_parameter;
    unsigned char m_default_sample_info_size;
    unsigned m_sample_count;
    Vector<unsigned char> m_sample_info_size;

    unsigned m_totalSize;
public:
    SaizBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_aux_info_type(0)
        , m_aux_info_parameter(0)
        , m_default_sample_info_size(0)
        , m_sample_count(0)
        , m_totalSize(0)
    {
    }
    virtual bool parseBox();
};

class TrafBox : public Box {
public:
    RefPtr<TfhdBox> m_tfhd;
    Vector<RefPtr<TrunBox>> m_trun;
    RefPtr<SencBox> m_senc;
    RefPtr<TfdtBox> m_tfdt;
    Vector<RefPtr<SaioBox>> m_saio;
    Vector<RefPtr<SaizBox>> m_saiz;

    TrafBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    {
    }
    virtual bool parseBox();
};

class MfhdBox : public FullBox {
private:
    unsigned m_sequenceNumber;
public:
    MfhdBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_sequenceNumber(0)
    {
    }
    virtual bool parseBox();
};

class MoofBox : public Box {
public:
    RefPtr<MfhdBox> m_mfhd;
    Vector<RefPtr<TrafBox>> m_traf;
    Vector<RefPtr<PsshBox>> m_pssh;
    Vector<unsigned char> m_rawData;

    MoofBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    {
    }
    virtual bool parseBox();
};

class MdatBox : public Box {
private:
    const unsigned char* m_dataBuffer;
public:
    MdatBox(const unsigned char* data, unsigned length)
        : Box(data, length)
        , m_dataBuffer(data)
    {
    }
    virtual ~MdatBox()
    {
    }
    virtual bool parseBox();
    const unsigned char* getInternalData() const { return m_dataBuffer; }
};

class MfroBox : public FullBox {
private:
    unsigned m_localSize;
public:
    MfroBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_localSize(0)
    {
    }
    virtual bool parseBox();
};

class MfraBox : public Box {
private:
    RefPtr<MfroBox> m_mfro;
public:
    MfraBox(const unsigned char* data, unsigned length)
        : Box(data, length)
    {
    }
    virtual bool parseBox();
};

class PitmBox : public FullBox {
private:
    unsigned short m_itemID;
public:
    PitmBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
        , m_itemID(0)
    {
    }
    virtual bool parseBox();
protected:
    virtual unsigned getExpectedType() const { return BoxTypeAsIntT<'p', 'i', 't', 'm'>::Val; }
};

class MetaBox : public FullBox {
private:
    RefPtr<HdlrBox> m_hdlr;
    RefPtr<PitmBox> m_pitm;
    RefPtr<DinfBox> m_dinf;
public:
    MetaBox(const unsigned char* data, unsigned length)
        : FullBox(data, length)
    {
    }
    virtual bool parseBox();
protected:
    virtual Box* getChild(unsigned type, unsigned long long size);
    virtual unsigned getExpectedType() const { return BoxTypeAsIntT<'m', 'e', 't', 'a'>::Val; }
};

class MP4Parser {
public:

    MP4Parser(const unsigned char* data, unsigned length)
        : m_data(data)
        , m_leftLength(length)
        , m_readLength(0)
    {
    }

    PassRefPtr<Box> getNextBox();

    unsigned long long getReadLength() const { return m_readLength; } // how many bytes was readed for last box

private:
    bool readSizeAndType(unsigned long long& size, unsigned &type);

    const unsigned char* m_data;
    unsigned long long m_leftLength;
    unsigned long long m_readLength;

};



}

#endif
