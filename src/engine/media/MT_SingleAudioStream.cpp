/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MT_SingleAudioStream.h"
#include "MT_CodecList.h"
#include "resip/stack/SdpContents.hxx"
#include "../engine/helper/HL_Log.h"

#define LOG_SUBSYSTEM "SingleAudioStream"

using namespace MT;

SingleAudioStream::SingleAudioStream(const CodecList::Settings& codecSettings, Statistics& stat)
:mReceiver(codecSettings, stat), mDtmfReceiver(stat)
{
}

SingleAudioStream::~SingleAudioStream()
{
}

void SingleAudioStream::process(std::shared_ptr<jrtplib::RTPPacket> packet)
{
  ICELogMedia(<< "Processing incoming RTP/RTCP packet");
  if (packet->GetPayloadType() == resip::Codec::TelephoneEvent.payloadType())
    mDtmfReceiver.add(packet);
  else
    mReceiver.add(packet);
}

void SingleAudioStream::copyPcmTo(Audio::DataWindow& output, int needed)
{
  while (output.filled() < needed)
    if (!mReceiver.getAudio(output))
      break;

  if (output.filled() < needed)
    ICELogCritical(<< "Not enough data for speaker's mixer");
}

