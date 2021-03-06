/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_CODEC_H
#define __MT_CODEC_H

#if defined(USE_RESIP_INTEGRATION)
# include "resiprocate/resip/stack/SdpContents.hxx"
#endif
#include "../helper/HL_Types.h"
#include <map>
#include "../helper/HL_Pointer.h"


namespace MT
{
class Codec;
typedef std::shared_ptr<Codec> PCodec;

class CodecMap: public std::map<int, PCodec>
{
};

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
#if defined(USE_RESIP_INTEGRATION)
        typedef std::map<int, PCodec > CodecMap;
        virtual void create(CodecMap& codecs);
        virtual void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction);
        // Returns payload type from chosen codec if success. -1 is returned for negative result.
        virtual int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction);
        resip::Codec resipCodec();
#endif
    };
    virtual ~Codec() {}
    virtual const char* name() = 0;
    virtual int samplerate() = 0;
    virtual float timestampUnit() { return float(1.0 / samplerate()); }

    // Size of decoded audio frame in bytes
    virtual int pcmLength() = 0;

    // Time length of single audio frame
    virtual int frameTime() = 0;

    // Size of RTP frame in bytes. Can be zero for variable sized codecs.
    virtual int rtpLength() = 0;

    // Number of audio channels
    virtual int channels() { return 1; }


    // Returns size of encoded data (RTP) in bytes
    virtual int encode(const void* input, int inputBytes, void* output, int outputCapacity) = 0;

    // Returns size of decoded data (PCM signed short) in bytes
    virtual int decode(const void* input, int inputBytes, void* output, int outputCapacity) = 0;

    // Returns size of produced data (PCM signed short) in bytes
    virtual int plc(int lostFrames, void* output, int outputCapacity) = 0;

    // Returns size of codec in memory
    virtual int getSize() const { return 0; };
};
}
#endif
