/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MT_SingleAudioStream.h"
#include "MT_CodecList.h"
//#include "resip/stack/SdpContents.hxx"
#include "../engine/helper/HL_Log.h"

#define LOG_SUBSYSTEM "media"

using namespace MT;

SingleAudioStream::SingleAudioStream(const CodecList::Settings& codecSettings, Statistics& stat)
    :mReceiver(codecSettings, stat),
     mDtmfReceiver(stat)
{}

SingleAudioStream::~SingleAudioStream()
{}

void SingleAudioStream::process(const std::shared_ptr<jrtplib::RTPPacket>& packet)
{
    ICELogMedia(<< "Processing incoming RTP/RTCP packet");
    if (packet->GetPayloadType() == mReceiver.getCodecSettings().mTelephoneEvent)
        mDtmfReceiver.add(packet);
    else
        mReceiver.add(packet);
}

void SingleAudioStream::copyPcmTo(Audio::DataWindow& output, int needed)
{
    // Packet by packet
    while (output.filled() < needed)
    {
        // Number of bytes to fill on this step
        auto requested = needed - output.filled();

        auto options = AudioReceiver::DecodeOptions{
            .mRealtimeProcessing = true,
            .mResampleToMainRate = true,
            .mSkipDecode = false,
            .mElapsed  = std::chrono::milliseconds(requested / (AUDIO_SAMPLERATE / 1000))
        };

        // Try to get the data from receiver / decoder
        if (options.mElapsed != 0ms) {
            if (mReceiver.getAudioTo(output, options).mStatus != AudioReceiver::DecodeResult::Status::Ok)
                break;
        } else
            break;
    }

    // if (output.filled() < needed)
    //    ICELogError(<< "Not enough data for speaker's mixer");
}

