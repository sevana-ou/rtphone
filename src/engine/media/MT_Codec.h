/* Copyright(C) 2007-2026 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_CODEC_H
#define __MT_CODEC_H

#include <map>
#include <span>

#include "resiprocate/resip/stack/SdpContents.hxx"
#include "../helper/HL_Types.h"
#include "../audio/Audio_Interface.h"

namespace MT
{
class Codec;
typedef std::shared_ptr<Codec> PCodec;

class CodecMap: public std::map<int, PCodec>
{};

class Codec
{
public:

    class Factory
    {
    public:
        virtual ~Factory() {}
        virtual const char* name() = 0;
        virtual int samplerate() = 0;
        virtual int payloadType() = 0;
        virtual PCodec create() = 0;

        virtual int channels();
        typedef std::map<int, PCodec > CodecMap;
        virtual void create(CodecMap& codecs);
        virtual void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction);
        // Returns payload type from chosen codec if success. -1 is returned for negative result.
        virtual int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction);
        resip::Codec resipCodec();
    };
    virtual ~Codec() {}

    struct Info
    {
        std::string mName;
        int mSamplerate = 0;    // Hz
        int mChannels = 0;
        int mPcmLength = 0;     // In bytes
        int mFrameTime = 0;     // In milliseconds
        int mRtpLength = 0;     // In bytes
        float mTimestampUnit = 0.0f;
    };
    // Returns information about this codec instance
    virtual Info info() = 0;

    // Helper functions to return information - they are based on info() method
    int pcmLength()         { return info().mPcmLength;  }
    int rtpLength()         { return info().mRtpLength;  }
    int channels()          { return info().mChannels;   }
    int samplerate()        { return info().mSamplerate; }
    int frameTime()         { return info().mFrameTime;  }
    std::string name()      { return info().mName;       }
    float timestampUnit()   { return info().mTimestampUnit == 0.0f ? 1.0f / info().mSamplerate : info().mTimestampUnit; }

    Audio::Format getAudioFormat() {
        return Audio::Format(this->info().mSamplerate, this->info().mChannels);
    }

    // Returns size of encoded data (RTP) in bytes
    struct EncodeResult
    {
        size_t mEncoded = 0; // Number of encoded bytes
    };
    virtual EncodeResult encode(std::span<const uint8_t> input, std::span<uint8_t> output) = 0;

    // Returns size of decoded data (PCM signed short) in bytes
    struct DecodeResult
    {
        size_t  mDecoded = 0;    // Number of decoded bytes
        bool    mIsCng = false;    // Should this packet to be used as CNG ? (used for AMR codecs)
    };
    virtual DecodeResult decode(std::span<const uint8_t> input, std::span<uint8_t> output) = 0;

    // Returns size of produced data (PCM signed short) in bytes
    virtual size_t plc(int lostFrames, std::span<uint8_t> output) = 0;

};
}
#endif
