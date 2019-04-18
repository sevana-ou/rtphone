/* Copyright(C) 2007-2019 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../config.h"
#include "MT_CodecList.h"
#include "MT_AudioCodec.h"

#if !defined(TARGET_ANDROID) && !defined(TARGET_OPENWRT) && !defined(TARGET_WIN) && !defined(TARGET_RPI)
# include "MT_AmrCodec.h"
#endif

#include <algorithm>

using namespace MT;

CodecList::Settings CodecList::Settings::DefaultSettings;

CodecList::CodecList(const Settings& settings)
  :mSettings(settings)
{
  //mFactoryList.push_back(new OpusCodec::OpusFactory(16000, 1));
#if !defined(TARGET_ANDROID)
  if (settings.mOpusSpec.empty())
  {
    mFactoryList.push_back(new OpusCodec::OpusFactory(48000, 2, MT_OPUS_CODEC_PT));
  }
  else
  {
    for (auto spec: settings.mOpusSpec)
    {
      mFactoryList.push_back(new OpusCodec::OpusFactory(spec.mRate, spec.mChannels, spec.mPayloadType));
    }
  }
#endif

#if !defined(TARGET_ANDROID) && !defined(TARGET_OPENWRT) && !defined(TARGET_WIN) && !defined(TARGET_RPI)
  for (int pt: mSettings.mAmrWbPayloadType)
    mFactoryList.push_back(new AmrWbCodec::CodecFactory({mSettings.mWrapIuUP, false, pt}));
  for (int pt: mSettings.mAmrWbOctetPayloadType)
    mFactoryList.push_back(new AmrWbCodec::CodecFactory({mSettings.mWrapIuUP, true, pt}));

  for (int pt: mSettings.mAmrNbPayloadType)
    mFactoryList.push_back(new AmrNbCodec::CodecFactory({mSettings.mWrapIuUP, false, pt}));
  for (int pt: mSettings.mAmrNbOctetPayloadType)
    mFactoryList.push_back(new AmrNbCodec::CodecFactory({mSettings.mWrapIuUP, true, pt}));

  mFactoryList.push_back(new GsmEfrCodec::GsmEfrFactory(mSettings.mWrapIuUP, mSettings.mGsmEfrPayloadType));
#endif

  //mFactoryList.push_back(new IsacCodec::IsacFactory16K(mSettings.mIsac16KPayloadType));
  //mFactoryList.push_back(new IlbcCodec::IlbcFactory(mSettings.mIlbc20PayloadType, mSettings.mIlbc30PayloadType));
  mFactoryList.push_back(new G711Codec::AlawFactory());
  mFactoryList.push_back(new G711Codec::UlawFactory());

  mFactoryList.push_back(new GsmCodec::GsmFactory(mSettings.mGsmFrPayloadLength == 32 ? GsmCodec::Type::Bytes_32 : GsmCodec::Type::Bytes_33, mSettings.mGsmFrPayloadType));
  mFactoryList.push_back(new G722Codec::G722Factory());
  mFactoryList.push_back(new G729Codec::G729Factory());
#ifndef TARGET_ANDROID
  mFactoryList.push_back(new GsmHrCodec::GsmHrFactory(mSettings.mGsmHrPayloadType));
#endif
}

CodecList::~CodecList()
{
  for (FactoryList::size_type i=0; i<mFactoryList.size(); i++)
    delete mFactoryList[i];
}

int CodecList::count() const
{
  return static_cast<int>(mFactoryList.size());
}

Codec::Factory& CodecList::codecAt(int index) const
{
  return *mFactoryList[index];
}

int CodecList::findCodec(const std::string &name) const
{
  for (int i=0; i<count(); i++)
  {
    if (codecAt(i).name() == name)
      return i;
  }
  return -1;
}

void CodecList::fillCodecMap(CodecMap& cm)
{
  for (auto& factory: mFactoryList)
  {
    // Create codec here. Although they are not needed right now - they can be needed to find codec's info.
    cm[factory->payloadType()] = factory->create();
  }
}


CodecListPriority::CodecListPriority()
{

}

CodecListPriority::~CodecListPriority()
{

}

bool CodecListPriority::isNegativePriority(const CodecListPriority::Item& item)
{
  return item.mPriority < 0;
}

bool CodecListPriority::compare(const Item& item1, const Item& item2)
{
  return item1.mPriority < item2.mPriority;
}

void CodecListPriority::setupFrom(PVariantMap vmap)
{
  CodecList::Settings settings;
  CodecList cl(settings);
  //mPriorityList.resize(cl.count());
  bool emptyVmap = vmap ? vmap->empty() : true;

  if (emptyVmap)
  {
    for (int i=0; i<cl.count(); i++)
    {
      Item item;
      item.mCodecIndex = i;
      item.mPriority = i;
      mPriorityList.push_back(item);
    }
  }
  else
  {
    for (int i=0; i<cl.count(); i++)
    {
      Item item;
      item.mCodecIndex = i;
      item.mPriority = vmap->exists(i) ? vmap->at(i).asInt() : -1;
      mPriorityList.push_back(item);
    }

    // Remove -1 records
    mPriorityList.erase(std::remove_if(mPriorityList.begin(), mPriorityList.end(), isNegativePriority), mPriorityList.end());

    // Sort by priority
    std::sort(mPriorityList.begin(), mPriorityList.end(), compare);
  }
}

int CodecListPriority::count(const CodecList &cl) const
{
  return mPriorityList.size();
}

Codec::Factory& CodecListPriority::codecAt(const CodecList& cl, int index) const
{
  return cl.codecAt(mPriorityList[index].mCodecIndex);
}
