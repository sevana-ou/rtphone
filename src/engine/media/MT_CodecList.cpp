/* Copyright(C) 2007-2023 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <algorithm>
#include <list>

#include "../engine_config.h"
#include "MT_CodecList.h"
#include "MT_AudioCodec.h"

#if !defined(TARGET_ANDROID) && !defined(TARGET_OPENWRT) && !defined(TARGET_RPI)
# if defined(USE_AMR_CODEC)
#  include "MT_AmrCodec.h"
# endif

# if defined(USE_EVS_CODEC)
#  include "MT_EvsCodec.h"
# endif
#endif

#include "helper/HL_String.h"

using namespace MT;

using strx = strx;


bool CodecList::Settings::contains(int ptype) const
{
    if (ptype >= 0 && ptype < 96)
        return true;

    if (mAmrNbOctetPayloadType.contains(ptype))
        return true;
    if (mAmrNbPayloadType.contains(ptype))
        return true;
    if (mAmrWbOctetPayloadType.contains(ptype))
        return true;
    if (mAmrWbPayloadType.contains(ptype))
        return true;
    for (const auto& s: mOpusSpec)
        if (s.mPayloadType == ptype)
            return true;
    for (const auto& s: mEvsSpec)
        if (s.mPayloadType == ptype)
            return true;

    if (mIsac16KPayloadType == ptype || mIsac32KPayloadType == ptype)
        return true;

    if (mIlbc20PayloadType == ptype || mIlbc30PayloadType == ptype)
        return true;

    if (mGsmEfrPayloadType == ptype || mGsmFrPayloadType == ptype || mGsmHrPayloadType == ptype)
        return true;

    return false;
}

std::string CodecList::Settings::toString() const
{
    std::ostringstream oss;
    // oss << "wrap IuUP:    " << mWrapIuUP << std::endl
    //     << "skip decode:  " << mSkipDecode << std::endl;

    if (!mAmrWbPayloadType.empty())
    {
        oss << "AMR WB ptype: ";
        for (int ptype: mAmrWbPayloadType)
            oss << ptype << " ";
    }
    if (!mAmrWbOctetPayloadType.empty())
    {
        oss << "AMR WB octet ptype: ";
        for (int ptype: mAmrWbOctetPayloadType)
            oss << ptype << " ";
    }

    if (!mAmrNbPayloadType.empty())
    {
        oss << "AMR ptype: ";
        for (int ptype: mAmrNbPayloadType)
            oss << ptype << " ";
    }

    if (!mAmrNbOctetPayloadType.empty())
    {
        oss << "AMR octet ptype: ";
        for (int ptype: mAmrNbOctetPayloadType)
            oss << ptype << " ";
    }
    if (mIsac16KPayloadType != -1)
        oss << "ISAC 16Khz ptype: " << mIsac16KPayloadType << " ";

    if (mIsac32KPayloadType != -1)
        oss << "ISAC 32Khz ptype: " << mIsac32KPayloadType << " ";

    if (mIlbc20PayloadType != -1)
        oss << "iLBC 20ms ptype: " << mIlbc20PayloadType << " ";

    if (mIlbc30PayloadType != -1)
        oss << "iLBC 30ms ptype: " << mIlbc30PayloadType << " ";

    if (mGsmFrPayloadType != -1)
        oss << "GSM FR ptype: " << mGsmFrPayloadType << ", GSM FR plength: " << mGsmFrPayloadLength << " ";

    if (mGsmHrPayloadType != -1)
        oss << "GSM HR ptype: " << mGsmHrPayloadType << " ";

    if (mGsmEfrPayloadType != -1)
        oss << "GSM EFR ptype: " << mGsmEfrPayloadType << " ";

    for (auto& spec: mEvsSpec)
    {
        oss << "EVS ptype: " << spec.mPayloadType << ", bw: " << spec.mBandwidth << ", enc: " << (spec.mEncodingType == EvsSpec::Encoding_MIME ? "mime" : "g192") << " ";
    }

    for (auto& spec: mOpusSpec)
    {
        oss << "OPUS ptype: " << spec.mPayloadType << ", rate: " << spec.mRate << ", channels: " << spec.mChannels << std::endl;
    }

    return oss.str();
}

void CodecList::Settings::clear()
{
    // Remove all dynamic payload type assignments
    mEvsSpec.clear();
    mOpusSpec.clear();
    mAmrNbOctetPayloadType.clear();
    mAmrNbPayloadType.clear();
    mAmrWbOctetPayloadType.clear();
    mAmrWbPayloadType.clear();
    mIsac16KPayloadType = -1;
    mIsac32KPayloadType = -1;
    mIlbc20PayloadType = -1;
    mIlbc30PayloadType = -1;
    mGsmEfrPayloadType = -1;
    mGsmFrPayloadType = -1;
    mGsmHrPayloadType = -1;
}

bool CodecList::Settings::EvsSpec::isValid() const
{
    return mPayloadType >= 96 && mPayloadType <= 127;
}

CodecList::Settings::EvsSpec CodecList::Settings::EvsSpec::parse(const std::string& spec)
{
    EvsSpec result;

    auto parts = strx::split(spec, "-/");
    if (parts.size() == 3)
    {
        // Payload type number
        result.mPayloadType = strx::toInt(strx::trim(parts.front()).c_str(), -1);

        // Encoding
        std::string& encoding_type = parts[1];
        if (encoding_type == "mime")
            result.mEncodingType = Encoding_MIME;
        else
            if (encoding_type == "g192")
                result.mEncodingType = Encoding_G192;
            else
                throw std::logic_error("Bad EVS codec encoding type");

        // Bandwidth
        std::string& bandwidth = parts.back();
        if (bandwidth == "nb" || bandwidth == "NB")
            result.mBandwidth = Bandwidth_NB;
        else
            if (bandwidth == "wb" || bandwidth == "WB")
                result.mBandwidth = Bandwidth_WB;
            else
                if (bandwidth == "swb" || bandwidth == "SWB")
                    result.mBandwidth = Bandwidth_SWB;
                else
                    if (bandwidth == "fb" || bandwidth == "FB")
                        result.mBandwidth = Bandwidth_FB;
    }

    return result;
}

bool CodecList::Settings::OpusSpec::isValid() const
{
    return (mPayloadType >= 96 && mPayloadType <= 127 &&
            mRate > 0 &&
            mChannels > 0);
}

CodecList::Settings::OpusSpec CodecList::Settings::OpusSpec::parse(const std::string &spec)
{
    OpusSpec result;

    auto parts = strx::split(spec, "-/");
    if (parts.size() == 3)
    {
        result.mPayloadType = strx::toInt(strx::trim(parts.front()).c_str(), -1);
        result.mRate = strx::toInt(strx::trim(parts[1]).c_str(), -1);
        result.mChannels = strx::toInt(strx::trim(parts.back()).c_str(), -1);
    }
    return result;
}

#if defined(USE_RESIP_INTEGRATION)
static int findOctetMode(const char* line)
{
    const char* param_name = "octet-align=";
    auto p = strstr(line, param_name);
    if (!p)
        return 0;

    p += strlen(param_name);
    char int_buf[8] = {0};
    int int_buf_offset = 0;
    while (*p && isdigit(*p) && int_buf_offset < sizeof(int_buf))
        int_buf[int_buf_offset++] = *p++;
    return atoi(int_buf);
}

CodecList::Settings CodecList::Settings::parseSdp(const std::list<resip::Codec>& codeclist)
{
    CodecList::Settings r{DefaultSettings};

    for (auto& c: codeclist)
    {
        std::string codec_name = strx::uppercase(c.getName().c_str());
        int samplerate = c.getRate();
        int ptype = c.payloadType();

        auto enc_params = c.encodingParameters(); // This must channels number for Opus codec
        auto params = c.parameters();

        // Dynamic payload type codecs only - ISAC / iLBC / Speex / etc.
        if (codec_name == "OPUS")
        {
            // Check the parameters
            int channels = strx::toInt(enc_params.c_str(), 1);
            r.mOpusSpec.push_back({ptype, samplerate, channels});
        }
        else
            if (codec_name == "AMR-WB")
            {
                int octet_mode = findOctetMode(params.c_str());
                if (octet_mode != -1)
                {
                    if (octet_mode == 0)
                        r.mAmrWbPayloadType.insert(ptype);
                    else
                        if (octet_mode == 1)
                            r.mAmrWbOctetPayloadType.insert(ptype);
                }
                // std::cout << "AMR-WB parameters: " << params.c_str() << ", found octet-mode: " << octet_mode << std::endl;
            }
            else
                if (codec_name == "AMR")
                {
                    int octet_mode = findOctetMode(params.c_str());
                    if (octet_mode != -1)
                    {
                        if (octet_mode == 0)
                            r.mAmrWbPayloadType.insert(ptype);
                        else
                            if (octet_mode == 1)
                                r.mAmrWbOctetPayloadType.insert(ptype);
                    }
                    // std::cout << "AMR parameters: " << params.c_str() << ", found octet-mode: " << octet_mode << std::endl;
                }
    }
    return r;
}
#endif


bool CodecList::Settings::operator == (const Settings& rhs) const
{
    if (std::tie(mWrapIuUP, mSkipDecode, mIsac16KPayloadType, mIsac32KPayloadType, mIlbc20PayloadType, mIlbc30PayloadType, mGsmFrPayloadType, mGsmFrPayloadLength, mGsmEfrPayloadType, mGsmHrPayloadType) !=
        std::tie(rhs.mWrapIuUP, rhs.mSkipDecode, rhs.mIsac16KPayloadType, rhs.mIsac32KPayloadType, rhs.mIlbc20PayloadType, rhs.mIlbc30PayloadType, rhs.mGsmFrPayloadType, rhs.mGsmFrPayloadLength, rhs.mGsmEfrPayloadType, rhs.mGsmHrPayloadType))
        return false;

    if (mAmrNbOctetPayloadType != rhs.mAmrNbOctetPayloadType)
        return false;

    if (mAmrNbPayloadType != rhs.mAmrNbPayloadType)
        return false;

    if (mAmrWbOctetPayloadType != rhs.mAmrWbOctetPayloadType)
        return false;

    if (mAmrWbPayloadType != rhs.mAmrWbPayloadType)
        return false;

    // ToDo: compare EVS and Opus specs
    if (mEvsSpec.size() != rhs.mEvsSpec.size())
        return false;

    for (size_t i = 0; i < mEvsSpec.size(); i++)
        if (mEvsSpec[i] != rhs.mEvsSpec[i])
            return false;

    if (mOpusSpec.size() != rhs.mOpusSpec.size())
        return false;

    for (size_t i = 0; i < mOpusSpec.size(); i++)
        if (mOpusSpec[i] != rhs.mOpusSpec[i])
            return false;

    return true;
}


// ----------------------------------------
CodecList::Settings CodecList::Settings::DefaultSettings;

CodecList::CodecList(const Settings& settings)
    :mSettings(settings)
{
    init(mSettings);
}

void CodecList::init(const Settings& settings)
{
    mFactoryList.clear();
    mSettings = settings;
#if defined(USE_OPUS_CODEC)
    for (auto spec: settings.mOpusSpec)
    {
        mFactoryList.push_back(std::make_shared<OpusCodec::OpusFactory>(spec.mRate, spec.mChannels, spec.mPayloadType));
    }
#endif


#if !defined(TARGET_ANDROID) && !defined(TARGET_OPENWRT) && !defined(TARGET_RPI)
#if defined(USE_AMR_CODEC)
    for (int pt: mSettings.mAmrWbPayloadType)
        mFactoryList.push_back(std::make_shared<AmrWbCodec::CodecFactory>(AmrCodecConfig{mSettings.mWrapIuUP, false, pt}));
    for (int pt: mSettings.mAmrWbOctetPayloadType)
        mFactoryList.push_back(std::make_shared<AmrWbCodec::CodecFactory>(AmrCodecConfig{mSettings.mWrapIuUP, true, pt}));

    for (int pt: mSettings.mAmrNbPayloadType)
        mFactoryList.push_back(std::make_shared<AmrNbCodec::CodecFactory>(AmrCodecConfig{mSettings.mWrapIuUP, false, pt}));
    for (int pt: mSettings.mAmrNbOctetPayloadType)
        mFactoryList.push_back(std::make_shared<AmrNbCodec::CodecFactory>(AmrCodecConfig{mSettings.mWrapIuUP, true, pt}));

    if (mSettings.mGsmEfrPayloadType != -1)
        mFactoryList.push_back(std::make_shared<GsmEfrCodec::GsmEfrFactory>(mSettings.mWrapIuUP, mSettings.mGsmEfrPayloadType));
#endif
#endif
    mFactoryList.push_back(std::make_shared<G711Codec::AlawFactory>());
    mFactoryList.push_back(std::make_shared<G711Codec::UlawFactory>());

    if (mSettings.mGsmFrPayloadType != -1)
        mFactoryList.push_back(std::make_shared<GsmCodec::GsmFactory>(mSettings.mGsmFrPayloadLength == 32 ? GsmCodec::Type::Bytes_32 : GsmCodec::Type::Bytes_33, mSettings.mGsmFrPayloadType));
    mFactoryList.push_back(std::make_shared<G722Codec::G722Factory>());
    mFactoryList.push_back(std::make_shared<G729Codec::G729Factory>());
#ifndef TARGET_ANDROID
    if (mSettings.mGsmHrPayloadType != -1)
        mFactoryList.push_back(std::make_shared<GsmHrCodec::GsmHrFactory>(mSettings.mGsmHrPayloadType));
#endif

#if !defined(TARGET_ANDROID) && defined(USE_EVS_CODEC)
    for (auto& spec: settings.mEvsSpec)
    {
        EVSCodec::StreamParameters evs_params;
        evs_params.mime = spec.mEncodingType == Settings::EvsSpec::Encoding_MIME;
        evs_params.bw = (int)spec.mBandwidth;
        evs_params.ptime = 20;
        evs_params.ptype = spec.mPayloadType;

        mFactoryList.push_back(std::make_shared<EVSCodec::EVSFactory>(evs_params));
    }
#endif
}

CodecList::~CodecList()
{
    mFactoryList.clear();
}

int CodecList::count() const
{
    return static_cast<int>(mFactoryList.size());
}

Codec::Factory& CodecList::codecAt(int index) const
{
    return *mFactoryList[static_cast<size_t>(index)];
}

int CodecList::findCodec(const std::string &name) const
{
    for (int i=0; i<count(); i++)
    {
        if (codecAt(i).name() == name)
            return i;
    }
    return -1;
}

void CodecList::fillCodecMap(CodecMap& cm)
{
    cm.clear();
    for (auto& factory: mFactoryList)
    {
        // Create codec here. Although they are not needed right now - they can be needed to find codec's info.
        PCodec c = factory->create();
        cm.insert({factory->payloadType(), c});
    }
}

CodecListPriority::CodecListPriority()
{

}

CodecListPriority::~CodecListPriority()
{

}

bool CodecListPriority::isNegativePriority(const CodecListPriority::Item& item)
{
    return item.mPriority < 0;
}

bool CodecListPriority::compare(const Item& item1, const Item& item2)
{
    return item1.mPriority < item2.mPriority;
}

void CodecListPriority::setupFrom(PVariantMap vmap)
{
    CodecList::Settings settings;
    CodecList cl(settings);
    //mPriorityList.resize(cl.count());
    bool emptyVmap = vmap ? vmap->empty() : true;

    if (emptyVmap)
    {
        for (int i=0; i<cl.count(); i++)
        {
            Item item;
            item.mCodecIndex = i;
            item.mPriority = i;
            mPriorityList.push_back(item);
        }
    }
    else
    {
        for (int i=0; i<cl.count(); i++)
        {
            Item item;
            item.mCodecIndex = i;
            item.mPriority = vmap->exists(i) ? vmap->at(i).asInt() : 1000; // Non listed codecs will get lower priority
            mPriorityList.push_back(item);
        }

        // Remove -1 records
        mPriorityList.erase(std::remove_if(mPriorityList.begin(), mPriorityList.end(), isNegativePriority), mPriorityList.end());

        // Sort by priority
        std::sort(mPriorityList.begin(), mPriorityList.end(), compare);
    }
}

int CodecListPriority::count(const CodecList & /*cl*/) const
{
    return static_cast<int>(mPriorityList.size());
}

Codec::Factory& CodecListPriority::codecAt(const CodecList& cl, int index) const
{
    return cl.codecAt(mPriorityList[static_cast<size_t>(index)].mCodecIndex);
}
