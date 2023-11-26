/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../engine_config.h"
#include "MT_WebRtc.h"
#include "../helper/HL_Exception.h"

#include <stdlib.h>

static void checkResultCode(unsigned short code)
{
  if (code)
    throw Exception(ERR_WEBRTC, code);
}

Vad::Vad(int sampleRate)
:mSampleRate(sampleRate), mContext(NULL)
{
  checkResultCode(WebRtcVad_Create(&mContext));
}

Vad::~Vad()
{
  if (mContext)
    WebRtcVad_Free(mContext);
}

bool Vad::isSilence(short* samplePtr, int nrOfSamples)
{
  short resultCode = WebRtcVad_Process(mContext, mSampleRate, samplePtr, nrOfSamples);
  return !resultCode;
}

// -- Cng ---
Cng::Cng()
:mEncoder(NULL), mDecoder(NULL)
{
  checkResultCode(WebRtcCng_CreateEnc(&mEncoder));
  checkResultCode(WebRtcCng_CreateDec(&mDecoder));
}

Cng::~Cng()
{
  if (mEncoder) 
    WebRtcCng_FreeEnc(mEncoder);
  if (mDecoder)
    WebRtcCng_FreeDec(mDecoder);
}

void Cng::updateSid(unsigned char *sidPacket, int sidLength)
{
  WebRtcCng_UpdateSid(mDecoder, sidPacket, sidLength);
}

void Cng::generateSid(short* samples, int nrOfSamples, unsigned char* sidPacket, int* sidLength)
{
  WebRtc_Word16 produced = 0;
  checkResultCode(WebRtcCng_Encode(mEncoder, (WebRtc_Word16*)samples, nrOfSamples, sidPacket, &produced, 1/*TRUE*/));  
  
  *sidLength = (int)produced;
}

void Cng::generateNoise(short* buffer, int nrOfSamples)
{
  checkResultCode(WebRtcCng_Generate(mDecoder, buffer, nrOfSamples, 1 /*Reset CNG history*/));
}


