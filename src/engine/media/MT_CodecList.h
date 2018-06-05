/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_CODEC_LIST_H
#define __MT_CODEC_LIST_H

#include "../config.h"
#ifdef USE_RESIP_INTEGRATION
# include "resiprocate/resip/stack/SdpContents.hxx"
#endif
#include "MT_Codec.h"
#include <vector>
#include <set>
#include "../helper/HL_VariantMap.h"

#define ALL_CODECS_STRING "OPUS,ISAC,ILBC,PCMU,PCMA,G722,GSM"

namespace MT
{
  class CodecList
  {
  public:
    struct Settings
    {
      bool mWrapIuUP              = false;
      bool mSkipDecode            = false;

      // AMR payload types
      std::set<int> mAmrWbPayloadType       = { MT_AMRWB_PAYLOADTYPE };
      std::set<int> mAmrNbPayloadType       = { MT_AMRNB_PAYLOADTYPE };
      std::set<int> mAmrWbOctetPayloadType  = { MT_AMRWB_OCTET_PAYLOADTYPE };
      std::set<int> mAmrNbOctetPayloadType  = { MT_AMRNB_OCTET_PAYLOADTYPE };

      bool isAmrWb(int ptype) const { return mAmrWbOctetPayloadType.count(ptype) > 0 || mAmrWbPayloadType.count(ptype) > 0; }
      bool isAmrNb(int ptype) const { return mAmrNbOctetPayloadType.count(ptype) > 0 || mAmrNbPayloadType.count(ptype) > 0; }

      int mIsac16KPayloadType     = MT_ISAC16K_PAYLOADTYPE;
      int mIsac32KPayloadType     = MT_ISAC32K_PAYLOADTYPE;
      int mIlbc20PayloadType      = MT_ILBC20_PAYLOADTYPE;
      int mIlbc30PayloadType      = MT_ILBC30_PAYLOADTYPE;
      int mGsmFrPayloadType       = 3; // GSM is codec with fixed payload type. But sometimes it has to be overwritten.
      int mGsmFrPayloadLength     = 33; // Expected GSM payload length
      int mGsmHrPayloadType       = MT_GSMHR_PAYLOADTYPE;
      int mGsmEfrPayloadType      = MT_GSMEFR_PAYLOADTYPE;

      struct OpusSpec
      {
        int mPayloadType = 0;
        int mRate = 0;
        int mChannels = 0;
      };
      std::vector<OpusSpec> mOpusSpec;

      static Settings DefaultSettings;
    };

    CodecList(const Settings& settings);
    ~CodecList();

    int count() const;
    Codec::Factory& codecAt(int index) const;
    int findCodec(const std::string& name) const;
    void fillCodecMap(CodecMap& cm);

  protected:
    typedef std::vector<Codec::Factory*> FactoryList;
    FactoryList mFactoryList;
    Settings mSettings;
  };

  class CodecListPriority
  {
  public:
    CodecListPriority();
    ~CodecListPriority();

    void setupFrom(PVariantMap vmap);
    int count(const CodecList& cl) const;
    Codec::Factory& codecAt(const CodecList& cl, int index) const;

  protected:
    struct Item
    {
      int mCodecIndex;
      int mPriority;
    };
    std::vector<Item> mPriorityList;

    static bool isNegativePriority(const CodecListPriority::Item& item);
    static bool compare(const Item& item1, const Item& item2);
  };
}
#endif
