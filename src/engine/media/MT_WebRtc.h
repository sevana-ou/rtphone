/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_WEBRTC_H
#define __MT_WEBRTC_H

#include <string>
#include <vector>
#include "jrtplib/src/rtppacket.h"
#include "webrtc/cng/webrtc_cng.h"
#include "webrtc/vad/webrtc_vad.h"

// Voice activity detector. It is tiny wrapper for webrtc functionality.
class Vad
{
public:
  Vad(int sampleRate);
  ~Vad();

  bool isSilence(short* samplePtr, int sampleCount);

protected:
  int mSampleRate;
  bool mEnabled;
  VadInst* mContext;
};

// Comfort noise helper. It is tiny wrapper for webrtc functionality.
class Cng
{
public:
  Cng();
  ~Cng();

  void updateSid(unsigned char *sidPacket, int sidLength);
  void generateSid(short* samples, int nrOfSamples, unsigned char* sidPacket, int* sidLength);
  void generateNoise(short* buffer, int nrOfSamples);

protected:
  CNG_enc_inst* mEncoder;
  CNG_dec_inst* mDecoder;
};

#endif
