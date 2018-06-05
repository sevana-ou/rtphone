/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MT_Codec.h"

using namespace MT;

int Codec::Factory::channels()
{
  return 1;
}

#ifdef USE_RESIP_INTEGRATION
void Codec::Factory::create(CodecMap& codecs)
{
  codecs[payloadType()] = std::shared_ptr<Codec>(create());
}

void Codec::Factory::updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{
  codecs.push_back(resipCodec());
}

resip::Codec Codec::Factory::resipCodec()
{
  resip::Codec c(this->name(), this->payloadType(), this->samplerate());
  return c;
}

int Codec::Factory::processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{
  for (resip::SdpContents::Session::Medium::CodecContainer::const_iterator codecIter = codecs.begin(); codecIter != codecs.end(); ++codecIter)
  {
    if (resipCodec() == *codecIter)
      return codecIter->payloadType();
  }
  return -1;
}
#endif
