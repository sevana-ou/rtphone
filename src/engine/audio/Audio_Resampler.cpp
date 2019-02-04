/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../config.h"
#include "Audio_Resampler.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <algorithm>
#include "speex/speex_resampler.h"

#define IS_FRACTIONAL_RATE(X) (((X) % 8000) != 0)

namespace Audio
{


SpeexResampler::SpeexResampler()
    :mContext(NULL), mErrorCode(0), mSourceRate(0), mDestRate(0), mLastSample(0)
{
}

void SpeexResampler::start(int channels, int sourceRate, int destRate)
{
    if (mSourceRate == sourceRate && mDestRate == destRate && mContext)
        return;

    if (mContext)
        stop();

    mSourceRate = sourceRate;
    mDestRate = destRate;
    mChannels = channels;

    if (sourceRate != destRate)
    {
        // Defer context creation until first request
        //mContext = speex_resampler_init(channels, sourceRate, destRate, AUDIO_RESAMPLER_QUALITY, &mErrorCode);
        //assert(mContext != NULL);
    }
}

void SpeexResampler::stop()
{
    if (mContext)
    {
        speex_resampler_destroy((SpeexResamplerState*)mContext);
        mContext = NULL;
    }
}

SpeexResampler::~SpeexResampler()
{
    stop();
}

size_t SpeexResampler::processBuffer(const void* src, size_t sourceLength, size_t& sourceProcessed, void* dest, size_t destCapacity)
{
    assert(mSourceRate != 0 && mDestRate != 0);

    if (mDestRate == mSourceRate)
    {
        assert(destCapacity >= sourceLength);
        memcpy(dest, src, sourceLength);
        sourceProcessed = sourceLength;
        return sourceLength;
    }

    if (!mContext)
    {
        mContext = speex_resampler_init(mChannels, mSourceRate, mDestRate,
                                        AUDIO_RESAMPLER_QUALITY, &mErrorCode);
        if (!mContext)
            return 0;
    }

    // Check if there is zero samples passed
    if (sourceLength / (sizeof(short) * mChannels) == 0)
    {
        // Consume all data
        sourceProcessed = sourceLength;

        // But no output
        return 0;
    }

    size_t outLen = getDestLength(sourceLength);
    if (outLen > destCapacity)
        return 0; // Skip resampling if not enough space

    assert(destCapacity >= outLen);

    // Calculate number of samples - input length is in bytes
    unsigned inLen = sourceLength / (sizeof(short) * mChannels);
    outLen /= sizeof(short) * mChannels;
    assert(mContext != NULL);
    spx_uint32_t in_len = static_cast<spx_uint32_t>(inLen),
            out_len = static_cast<spx_uint32_t>(outLen);

    int speexCode = speex_resampler_process_interleaved_int((SpeexResamplerState *)mContext,
                                                            (spx_int16_t*)src, &in_len,
                                                            (spx_int16_t*)dest, &out_len);
    assert(speexCode == RESAMPLER_ERR_SUCCESS);

    inLen = static_cast<size_t>(in_len);
    outLen = static_cast<size_t>(out_len);

    // Return results in bytes
    sourceProcessed = inLen * sizeof(short) * mChannels;
    return outLen * sizeof(short) * mChannels;
}

int SpeexResampler::sourceRate()
{
    return mSourceRate;
}

int SpeexResampler::destRate()
{
    return mDestRate;
}

size_t SpeexResampler::getDestLength(size_t sourceLen)
{
    return size_t(sourceLen * (float(mDestRate) / mSourceRate) + 0.5f) / 2 * 2;
}

size_t SpeexResampler::getSourceLength(size_t destLen)
{
    return size_t(destLen * (float(mSourceRate) / mDestRate) + 0.5f) / 2 * 2;
}

// Returns instance + speex resampler size in bytes
size_t SpeexResampler::getSize() const
{
    return sizeof(*this) + 200; // 200 is approximate size of speex resample structure
}

// -------------------------- ChannelConverter --------------------
int ChannelConverter::stereoToMono(const void *source, int sourceLength, void *dest, int destLength)
{
    assert(destLength == sourceLength / 2);
    const short* input = (const short*)source;
    short* output = (short*)dest;
    for (int sampleIndex = 0; sampleIndex < destLength/2; sampleIndex++)
    {
        output[sampleIndex] = (input[sampleIndex*2] + input[sampleIndex*2+1]) >> 1;
    }
    return sourceLength / 2;
}

int ChannelConverter::monoToStereo(const void *source, int sourceLength, void *dest, int destLength)
{
    assert(destLength == sourceLength * 2);
    const short* input = (const short*)source;
    short* output = (short*)dest;
    // Convert starting from the end of buffer to allow inplace conversion
    for (int sampleIndex = sourceLength/2 - 1; sampleIndex >= 0; sampleIndex--)
    {
        output[2*sampleIndex] = output[2*sampleIndex+1] = input[sampleIndex];
    }
    return sourceLength * 2;
}


#if defined(USE_WEBRTC_RESAMPLER)
Resampler48kTo16k::Resampler48kTo16k()
{
    WebRtcSpl_ResetResample48khzTo16khz(&mContext);
}

Resampler48kTo16k::~Resampler48kTo16k()
{
    WebRtcSpl_ResetResample48khzTo16khz(&mContext);
}

int Resampler48kTo16k::process(const void *source, int sourceLen, void *dest, int destLen)
{
    const short* input = (const short*)source; int inputLen = sourceLen / 2;
    short* output = (short*)dest; //int outputCapacity = destLen / 2;
    assert(inputLen % 480 == 0);
    int frames = inputLen / 480;
    for (int i=0; i<frames; i++)
        WebRtcSpl_Resample48khzTo16khz(input + i * 480, output + i * 160, &mContext, mTemp);

    return sourceLen / 3;
}


Resampler16kto48k::Resampler16kto48k()
{
    WebRtcSpl_ResetResample16khzTo48khz(&mContext);
}

Resampler16kto48k::~Resampler16kto48k()
{
    WebRtcSpl_ResetResample16khzTo48khz(&mContext);
}

int Resampler16kto48k::process(const void *source, int sourceLen, void *dest, int destLen)
{
    const WebRtc_Word16* input = (const WebRtc_Word16*)source; int inputLen = sourceLen / 2;
    WebRtc_Word16* output = (WebRtc_Word16*)dest; //int outputCapacity = destLen / 2;
    assert(inputLen % 160 == 0);
    int frames = inputLen / 160;
    for (int i=0; i<frames; i++)
        WebRtcSpl_Resample16khzTo48khz(input + i * 160, output + i * 480, &mContext, mTemp);

    return sourceLen * 3;
}

#endif

// ---------------- UniversalResampler -------------------
UniversalResampler::UniversalResampler()
{

}

UniversalResampler::~UniversalResampler()
{

}

size_t UniversalResampler::resample(int sourceRate, const void *sourceBuffer, size_t sourceLength,
                                    size_t& sourceProcessed, int destRate, void *destBuffer, size_t destCapacity)
{
    assert(destBuffer && sourceBuffer);
    size_t result;
    if (sourceRate == destRate)
    {
        assert(destCapacity >= sourceLength);
        memcpy(destBuffer, sourceBuffer, sourceLength);
        sourceProcessed = sourceLength;
        result = sourceLength;
    }
    else
    {
        PResampler r = findResampler(sourceRate, destRate);
        result = r->processBuffer(sourceBuffer, sourceLength, sourceProcessed, destBuffer, destCapacity);
    }
    return result;
}

void UniversalResampler::preload()
{

}

size_t UniversalResampler::getDestLength(int sourceRate, int destRate, size_t sourceLength)
{
    if (sourceRate == destRate)
        return sourceLength;
    else
        return findResampler(sourceRate, destRate)->getDestLength(sourceLength);
}

size_t UniversalResampler::getSourceLength(int sourceRate, int destRate, size_t destLength)
{
    if (sourceRate == destRate)
        return destLength;
    else
        return findResampler(sourceRate, destRate)->getSourceLength(destLength);
}

PResampler UniversalResampler::findResampler(int sourceRate, int destRate)
{
    assert(sourceRate != destRate);
    ResamplerMap::iterator resamplerIter = mResamplerMap.find(RatePair(sourceRate, destRate));
    PResampler r;
    if (resamplerIter == mResamplerMap.end())
    {
        r = PResampler(new Resampler());
        r->start(AUDIO_CHANNELS, sourceRate, destRate);
        mResamplerMap[RatePair(sourceRate, destRate)] = r;
    }
    else
        r = resamplerIter->second;
    return r;
}

} // end of namespace
