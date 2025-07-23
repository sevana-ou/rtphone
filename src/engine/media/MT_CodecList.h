/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_CODEC_LIST_H
#define __MT_CODEC_LIST_H

#include "../engine_config.h"

#if defined(USE_RESIP_INTEGRATION)
# include "resiprocate/resip/stack/SdpContents.hxx"
#endif

#include "MT_Codec.h"
#include <vector>
#include <set>
#include <list>
#include "../helper/HL_VariantMap.h"

#define ALL_CODECS_STRING "OPUS,ISAC,ILBC,PCMU,PCMA,G722,GSM"

namespace MT
{
class CodecList
{
public:
    struct Settings
    {
        bool mWrapIuUP              = false;
        bool mSkipDecode            = false;

        // AMR payload types
        std::set<int64_t> mAmrWbPayloadType       = { };
        std::set<int64_t> mAmrNbPayloadType       = { };
        std::set<int64_t> mAmrWbOctetPayloadType  = { };
        std::set<int64_t> mAmrNbOctetPayloadType  = { };

        bool isAmrWb(int ptype) const { return mAmrWbOctetPayloadType.count(ptype) > 0 || mAmrWbPayloadType.count(ptype) > 0; }
        bool isAmrNb(int ptype) const { return mAmrNbOctetPayloadType.count(ptype) > 0 || mAmrNbPayloadType.count(ptype) > 0; }

        int mIsac16KPayloadType     = -1;
        int mIsac32KPayloadType     = -1;
        int mIlbc20PayloadType      = -1;
        int mIlbc30PayloadType      = -1;
        int mGsmFrPayloadType       = -1; // GSM is codec with fixed payload type. But sometimes it has to be overwritten.
        int mGsmFrPayloadLength     = 33; // Expected GSM payload length
        int mGsmHrPayloadType       = -1;
        int mGsmEfrPayloadType      = -1;

        struct EvsSpec
        {
            int mPayloadType = 0;
            enum Bandwidth
            {
                Bandwidth_NB = 0,
                Bandwidth_WB,
                Bandwidth_SWB,
                Bandwidth_FB
            };

            Bandwidth mBandwidth = Bandwidth_FB;

            enum Encoding
            {
                Encoding_MIME,
                Encoding_G192
            };

            Encoding mEncodingType = Encoding_MIME;
            bool isValid() const;
            static EvsSpec parse(const std::string& spec);

            bool operator == (const EvsSpec& rhs) const { return std::tie(mPayloadType, mBandwidth, mEncodingType) == std::tie(rhs.mPayloadType, rhs.mBandwidth, rhs.mEncodingType);}
            bool operator != (const EvsSpec& rhs) const { return ! (operator ==) (rhs);}

        };

        std::vector<EvsSpec> mEvsSpec;

        struct OpusSpec
        {
            int mPayloadType    = -1;
            int mRate           = -1;
            int mChannels       = -1;

            OpusSpec(int ptype = -1, int rate = -1, int channels = -1)
                :mPayloadType(ptype), mRate(rate), mChannels(channels)
            {}

            bool isValid() const;
            bool operator == (const OpusSpec& rhs) const { return std::tie(mPayloadType, mRate, mChannels) == std::tie(rhs.mPayloadType, rhs.mRate, rhs.mChannels);}
            bool operator != (const OpusSpec& rhs) const { return ! (operator ==) (rhs);}

            static OpusSpec parse(const std::string& spec);
        };
        std::vector<OpusSpec> mOpusSpec;

        // Payload type
        bool contains(int ptype) const;

        // Textual representation - used in logging
        std::string toString() const;
        void clear();

        static Settings DefaultSettings;

        #if defined(USE_RESIP_INTEGRATION)
        static Settings parseSdp(const std::list<resip::Codec>& codeclist);
        #endif

        bool operator == (const Settings& rhs) const;
    };

    CodecList(const Settings& settings);
    ~CodecList();
    void setSettings(const Settings& settings) {
        init(settings);
    }

    int count() const;
    Codec::Factory& codecAt(int index) const;
    int findCodec(const std::string& name) const;
    void fillCodecMap(CodecMap& cm);
    PCodec createCodecByPayloadType(int payloadType);
    void clear();

protected:
    typedef std::vector<std::shared_ptr<Codec::Factory>> FactoryList;
    FactoryList mFactoryList;
    Settings mSettings;

    void init(const Settings& settings);
};

class CodecListPriority
{
public:
    CodecListPriority();
    ~CodecListPriority();

    void setupFrom(PVariantMap vmap);
    int  count(const CodecList& cl) const;
    Codec::Factory& codecAt(const CodecList& cl, int index) const;

protected:
    struct Item
    {
        int mCodecIndex;
        int mPriority;
    };
    std::vector<Item> mPriorityList;

    static bool isNegativePriority(const CodecListPriority::Item& item);
    static bool compare(const Item& item1, const Item& item2);
};
}
#endif
