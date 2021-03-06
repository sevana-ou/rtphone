/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_RESAMPLER_H
#define __AUDIO_RESAMPLER_H

#ifdef USE_WEBRTC_RESAMPLER
# include "signal_processing_library/signal_processing_library.h"
#endif

#include <vector>
#include <memory>
#include <map>

namespace Audio 
{
    class SpeexResampler
    {
    public:
        SpeexResampler();
        ~SpeexResampler();

        void start(int channels, int sourceRate, int destRate);
        void stop();
        size_t processBuffer(const void* source, size_t sourceLength, size_t& sourceProcessed,
                             void* dest, size_t destCapacity);
        int sourceRate();
        int destRate();
        size_t getDestLength(size_t sourceLen);
        size_t getSourceLength(size_t destLen);

        // Returns instance + speex encoder size in bytes
        size_t getSize() const;

    protected:
        void* mContext;
        int   mErrorCode;
        int   mSourceRate,
        mDestRate,
        mChannels;
        short mLastSample;
    };

    typedef SpeexResampler Resampler;
    typedef std::shared_ptr<Resampler> PResampler;

    class ChannelConverter
    {
    public:
        static int stereoToMono(const void* source, int sourceLength, void* dest, int destLength);
        static int monoToStereo(const void* source, int sourceLength, void* dest, int destLength);
    };

    // Operates with AUDIO_CHANNELS number of channels
    class UniversalResampler
    {
    public:
        UniversalResampler();
        ~UniversalResampler();

        size_t resample(int sourceRate, const void* sourceBuffer, size_t sourceLength, size_t& sourceProcessed,
                        int destRate, void* destBuffer, size_t destCapacity);
        size_t getDestLength(int sourceRate, int destRate, size_t sourceLength);
        size_t getSourceLength(int sourceRate, int destRate, size_t destLength);

    protected:
        typedef std::pair<int, int> RatePair;
        typedef std::map<RatePair, PResampler> ResamplerMap;
        ResamplerMap mResamplerMap;
        PResampler findResampler(int sourceRate, int destRate);

        void preload();
    };

    #ifdef USE_WEBRTC_RESAMPLER
    // n*10 milliseconds buffers required!
    class Resampler48kTo16k
    {
    public:
        Resampler48kTo16k();
        ~Resampler48kTo16k();
        int process(const void* source, int sourceLen, void* dest, int destLen);
    protected:
        WebRtc_Word32 mTemp[496];
        WebRtcSpl_State48khzTo16khz mContext;
    };

    class Resampler16kto48k
    {
    public:
        Resampler16kto48k();
        ~Resampler16kto48k();
        int process(const void* source, int sourceLen, void* dest, int destLen);

    protected:
        WebRtc_Word32 mTemp[336];
        WebRtcSpl_State16khzTo48khz mContext;
    };
    #endif
} // end of namespace

#endif
