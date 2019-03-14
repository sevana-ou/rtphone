/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_AUDIO_RECEIVER_H
#define __MT_AUDIO_RECEIVER_H

#include "MT_Stream.h"
#include "MT_CodecList.h"
#include "MT_AudioCodec.h"
#include "MT_CngHelper.h"
#include "../helper/HL_Pointer.h"
#include "../helper/HL_Sync.h"
#include "../helper/HL_Optional.hpp"

#include "jrtplib/src/rtppacket.h"
#include "jrtplib/src/rtcppacket.h"
#include "jrtplib/src/rtpsourcedata.h"
#include "../audio/Audio_DataWindow.h"
#include "../audio/Audio_Resampler.h"

#include <map>

#if defined(USE_PVQA_LIBRARY)
# include "MT_SevanaMos.h"
#endif


// #define DUMP_DECODED

namespace MT
{
  using jrtplib::RTPPacket;
  class RtpBuffer
  {
  public:
    enum class FetchResult
    {
      RegularPacket,
      Gap,
      NoPacket
    };

    // Owns rtp packet data
    class Packet
    {
    public:
      Packet(std::shared_ptr<RTPPacket> packet, int timelen, int rate);
      std::shared_ptr<RTPPacket> rtp() const;
      int timelength() const;
      int rate() const;

    protected:
      std::shared_ptr<RTPPacket> mRtp;
      int mTimelength = 0, mRate = 0;
    };

    RtpBuffer(Statistics& stat);
    ~RtpBuffer();

    unsigned ssrc();
    void setSsrc(unsigned ssrc);
    void setHigh(int milliseconds);
    int high();
    void setLow(int milliseconds);
    int low();
    void setPrebuffer(int milliseconds);
    int prebuffer();
    int getNumberOfReturnedPackets() const;
    int getNumberOfAddPackets() const;
    int findTimelength();
    int getCount() const;
    // Returns false if packet was not add - maybe too old or too new or duplicate
    bool add(std::shared_ptr<RTPPacket> packet, int timelength, int rate);

    typedef std::vector<std::shared_ptr<RTPPacket>> ResultList;
    typedef std::shared_ptr<ResultList> PResultList;

    FetchResult fetch(ResultList& rl);
    
  protected:
    unsigned mSsrc;
    int mHigh, mLow, mPrebuffer;
    int mReturnedCounter, mAddCounter;
    mutable Mutex mGuard;
    typedef std::vector<Packet> PacketList;
    PacketList mPacketList;
    Statistics& mStat;
    bool mFirstPacketWillGo;
    jrtplib::RTPSourceStats mRtpStats;
    Packet mFetchedPacket;
  };

  class Receiver
  {
  public:
    Receiver(Statistics& stat);
    virtual ~Receiver();

  protected:
    Statistics& mStat;
  };

  class AudioReceiver: public Receiver
  {
  public:
    AudioReceiver(const CodecList::Settings& codecSettings, Statistics& stat);
    ~AudioReceiver();
    
    // Returns false when packet is rejected as illegal. codec parameter will show codec which will be used for decoding.
    // Lifetime of pointer to codec is limited by lifetime of AudioReceiver (it is container).
    bool add(std::shared_ptr<jrtplib::RTPPacket> p, Codec** codec = nullptr);

    // Returns false when there is no rtp data from jitter
    enum class DecodeOptions
    {
      ResampleToMainRate = 0,
      DontResample = 1,
      FillCngGap = 2,
      SkipDecode = 4
    };

    bool getAudio(Audio::DataWindow& output, DecodeOptions options = DecodeOptions::ResampleToMainRate, int* rate = nullptr);

    // Looks for codec by payload type
    Codec* findCodec(int payloadType);
    RtpBuffer& getRtpBuffer() { return mBuffer; }

    // Returns size of AudioReceiver's instance in bytes (including size of all data + codecs + etc.)
    int getSize() const;

    // Returns timelength for given packet
    int timelengthFor(jrtplib::RTPPacket& p);

    // Return samplerate for given packet
    int samplerateFor(jrtplib::RTPPacket& p);

  protected:
    RtpBuffer mBuffer;
    CodecMap mCodecMap;    
    PCodec mCodec;
    int mFrameCount = 0;
    CodecList::Settings mCodecSettings;
    CodecList mCodecList;
    JitterStatistics mJitterStats;
    std::shared_ptr<jrtplib::RTPPacket> mCngPacket;
    CngDecoder mCngDecoder;

    // Buffer to hold decoded data
    char mDecodedFrame[65536];
    int mDecodedLength = 0;

    // Buffer to hold data converted to stereo/mono
    char mConvertedFrame[32768];
    int mConvertedLength = 0;

    // Buffer to hold data resampled to AUDIO_SAMPLERATE
    char mResampledFrame[65536];
    int mResampledLength = 0;

    // Last packet time length
    int mLastPacketTimeLength = 0;

    int mFailedCount;
    Audio::Resampler  mResampler8, mResampler16,
                      mResampler32, mResampler48;

    Audio::PWavFileWriter mDecodedDump;

    // Zero rate will make audio mono but resampling will be skipped
    void makeMonoAndResample(int rate, int channels);

    // Resamples, sends to analysis, writes to dump and queues to output decoded frames from mDecodedFrame
    void processDecoded(Audio::DataWindow& output, DecodeOptions options);

#if defined(USE_PVQA_LIBRARY) && !defined(PVQA_SERVER)
    std::shared_ptr<SevanaPVQA> mPVQA;
    void initPvqa();
    void updatePvqa(const void* data, int size);
    float calculatePvqaMos(int rate, std::string& report);
    std::shared_ptr<Audio::DataWindow> mPvqaBuffer;
#endif

    void processStatisticsWithAmrCodec(Codec* c);
  };
  
  class DtmfReceiver: public Receiver
  {
  public:
    DtmfReceiver(Statistics& stat);
    ~DtmfReceiver();

    void add(std::shared_ptr<RTPPacket> p);
  };
}

#endif
