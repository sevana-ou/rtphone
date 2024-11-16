/* Copyright(C) 2007-2021 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define NOMINMAX

#include "../engine_config.h"
#include "MT_AudioReceiver.h"
#include "MT_AudioCodec.h"
#include "MT_CngHelper.h"
#include "../helper/HL_Log.h"
#include "../helper/HL_Time.h"
#include "../audio/Audio_Interface.h"
#include "../audio/Audio_Resampler.h"
#include <cmath>

#if !defined(TARGET_ANDROID) && !defined(TARGET_OPENWRT) && !defined(TARGET_WIN) && !defined(TARGET_RPI) && defined(USE_AMR_CODEC)
# include "MT_AmrCodec.h"
#endif

#include <algorithm>

#define LOG_SUBSYSTEM "AudioReceiver"

//#define DUMP_DECODED

using namespace MT;

// ----------------- RtpBuffer::Packet --------------
RtpBuffer::Packet::Packet(const std::shared_ptr<RTPPacket>& packet, int timelength, int rate)
    :mRtp(packet), mTimelength(timelength), mRate(rate)
{
}

std::shared_ptr<RTPPacket> RtpBuffer::Packet::rtp() const
{
    return mRtp;
}

int RtpBuffer::Packet::timelength() const
{
    return mTimelength;
}

int RtpBuffer::Packet::rate() const
{
    return mRate;
}

const std::vector<short>& RtpBuffer::Packet::pcm() const
{
    return mPcm;
}

std::vector<short>& RtpBuffer::Packet::pcm()
{
    return mPcm;
}

// ------------ RtpBuffer ----------------
RtpBuffer::RtpBuffer(Statistics& stat)
    :mStat(stat)
{
}

RtpBuffer::~RtpBuffer()
{
    ICELogDebug(<< "Number of add packets: " << mAddCounter << ", number of retrieved packets " << mReturnedCounter);
}

void RtpBuffer::setHigh(int milliseconds)
{
    mHigh = milliseconds;
}

int RtpBuffer::high()
{
    return mHigh;
}

void RtpBuffer::setLow(int milliseconds)
{
    mLow = milliseconds;
}

int RtpBuffer::low()
{
    return mLow;
}

void RtpBuffer::setPrebuffer(int milliseconds)
{
    mPrebuffer = milliseconds;
}

int RtpBuffer::prebuffer()
{
    return mPrebuffer;
}

int RtpBuffer::getCount() const
{
    Lock l(mGuard);
    return static_cast<int>(mPacketList.size());
}

bool SequenceSort(const std::shared_ptr<RtpBuffer::Packet>& p1, const std::shared_ptr<RtpBuffer::Packet>& p2)
{
    return p1->rtp()->GetExtendedSequenceNumber() < p2->rtp()->GetExtendedSequenceNumber();
}

std::shared_ptr<RtpBuffer::Packet> RtpBuffer::add(std::shared_ptr<jrtplib::RTPPacket> packet, int timelength, int rate)
{
    if (!packet)
        return std::shared_ptr<Packet>();

    Lock l(mGuard);


    // Update statistics
    if (mLastAddTime == 0.0)
        mLastAddTime = now_ms();
    else
    {
        float t = now_ms();
        mStat.mPacketInterval.process(t - mLastAddTime);
        mLastAddTime = t;
    }
    mStat.mSsrc = static_cast<uint16_t>(packet->GetSSRC());

    // Update jitter
    ICELogMedia(<< "Adding new packet into jitter buffer");
    mAddCounter++;

    // Look for maximum&minimal sequence number; check for dublicates
    unsigned maxno = 0xFFFFFFFF, minno = 0;

    // New sequence number
    unsigned newSeqno = packet->GetExtendedSequenceNumber();

    for (std::shared_ptr<Packet>& p: mPacketList)
    {
        unsigned seqno = p->rtp()->GetExtendedSequenceNumber();

        if (seqno == newSeqno)
        {
            mStat.mDuplicatedRtp++;
            ICELogMedia(<< "Discovered duplicated packet, skipping");
            return std::shared_ptr<Packet>();
        }

        if (seqno > maxno)
            maxno = seqno;
        if (seqno < minno)
            minno = seqno;
    }

    // Get amount of available audio (in milliseconds) in jitter buffer
    int available = findTimelength();

    if (newSeqno > minno || (available < mHigh))
    {
        // Insert into queue
        auto p = std::make_shared<Packet>(packet, timelength, rate);
        mPacketList.push_back(p);

        // Sort again
        std::sort(mPacketList.begin(), mPacketList.end(), SequenceSort);

        // Limit by max timelength
        available = findTimelength();

        if (available > mHigh)
            ICELogMedia(<< "Available " << available << "ms with limit " << mHigh << "ms");

        return p;
    }
    else
    {
        ICELogMedia(<< "Too old packet, skipping");
        mStat.mOldRtp++;

        return std::shared_ptr<Packet>();
    }

    return std::shared_ptr<Packet>();
}

RtpBuffer::FetchResult RtpBuffer::fetch(ResultList& rl)
{
    Lock l(mGuard);

    FetchResult result = FetchResult::NoPacket;
    rl.clear();

    // See if there is enough information in buffer
    int total = findTimelength();

    while (total > mHigh && mPacketList.size())
    {
        ICELogMedia( << "Dropping RTP packets from jitter buffer");
        total -= mPacketList.front()->timelength();

        // Save it as last packet however - to not confuse loss packet counter
        mFetchedPacket = mPacketList.front();

        // Erase from packet list
        mPacketList.erase(mPacketList.begin());

        // Increase number in statistics
        mStat.mPacketDropped++;
    }

    if (total < mLow)
        result = FetchResult::NoPacket;
    else
    {
        bool is_fetched_packet = mFetchedPacket.get() != nullptr;
        if (is_fetched_packet)
            is_fetched_packet &= mFetchedPacket->rtp().get() != nullptr;

        if (is_fetched_packet)
        {
            if (mPacketList.empty())
            {
                result = FetchResult::NoPacket;
                mStat.mPacketLoss++;
            }
            else
            {
                // Current sequence number ?
                unsigned seqno = mPacketList.front()->rtp()->GetExtendedSequenceNumber();

                // Gap between new packet and previous on
                int gap = (int64_t)seqno - (int64_t)mFetchedPacket->rtp()->GetExtendedSequenceNumber() - 1;
                gap = std::min(gap, 127);
                if (gap > 0 && mPacketList.empty())
                {
                    result = FetchResult::Gap;
                    mStat.mPacketLoss += gap;
                    mStat.mLoss[gap]++;
                }
                else
                {
                    if (gap > 0)
                    {
                        mStat.mPacketLoss += gap;
                        mStat.mLoss[gap]++;
                    }

                    result = FetchResult::RegularPacket;
                    rl.push_back(mPacketList.front());

                    // Save last returned normal packet
                    mFetchedPacket = mPacketList.front();

                    // Remove returned packet from the list
                    mPacketList.erase(mPacketList.begin());
                }
            }
        }
        else
        {
            // See if prebuffer limit is reached
            if (findTimelength() >= mPrebuffer)
            {
                // Normal packet will be returned
                result = FetchResult::RegularPacket;

                // Put it to output list
                rl.push_back(mPacketList.front());

                // Remember returned packet
                mFetchedPacket = mPacketList.front();

                // Remove returned packet from buffer list
                mPacketList.erase(mPacketList.begin());
            }
            else
            {
                ICELogMedia(<< "Jitter buffer was not prebuffered yet; resulting no packet");
                result = FetchResult::NoPacket;
            }
        }
    }

    if (result != FetchResult::NoPacket)
        mReturnedCounter++;

    return result;
}

int RtpBuffer::findTimelength()
{
    int available = 0;
    for (unsigned i = 0; i < mPacketList.size(); i++)
        available += mPacketList[i]->timelength();
    return available;
}

int RtpBuffer::getNumberOfReturnedPackets() const
{
    return mReturnedCounter;
}

int RtpBuffer::getNumberOfAddPackets() const
{
    return mAddCounter;
}

//-------------- Receiver ---------------
Receiver::Receiver(Statistics& stat)
    :mStat(stat)
{
}

Receiver::~Receiver()
{
}

//-------------- AudioReceiver ----------------
AudioReceiver::AudioReceiver(const CodecList::Settings& settings, MT::Statistics &stat)
    :Receiver(stat), mBuffer(stat), mCodecSettings(settings),
      mCodecList(settings)
{
    // Init resamplers
    mResampler8.start(AUDIO_CHANNELS, 8000, AUDIO_SAMPLERATE);
    mResampler16.start(AUDIO_CHANNELS, 16000, AUDIO_SAMPLERATE);
    mResampler32.start(AUDIO_CHANNELS, 32000, AUDIO_SAMPLERATE);
    mResampler48.start(AUDIO_CHANNELS, 48000, AUDIO_SAMPLERATE);

    // Init codecs
    mCodecList.setSettings(settings);
    mCodecList.fillCodecMap(mCodecMap);

#if defined(DUMP_DECODED)
    mDecodedDump = std::make_shared<Audio::WavFileWriter>();
    mDecodedDump->open("decoded.wav", 8000 /*G711*/, AUDIO_CHANNELS);
#endif
}

AudioReceiver::~AudioReceiver()
{
    mResampler8.stop();
    mResampler16.stop();
    mResampler32.stop();
    mResampler48.stop();
    mDecodedDump.reset();
}

// Update codec settings
void AudioReceiver::setCodecSettings(const CodecList::Settings& codecSettings)
{
    if (mCodecSettings == codecSettings)
        return;

    mCodecSettings = codecSettings;
    mCodecMap.clear();
    mCodecList.setSettings(mCodecSettings);
    mCodecList.fillCodecMap(mCodecMap);
}

size_t decode_packet(Codec& codec, RTPPacket& p, void* output_buffer, size_t output_capacity)
{
    // How much data was produced
    size_t result = 0;

    // Handle here regular RTP packets
    // Check if payload length is ok
    int tail = codec.rtpLength() ? p.GetPayloadLength() % codec.rtpLength() : 0;

    if (!tail)
    {
        // Find number of frames
        int frame_count = codec.rtpLength() ? p.GetPayloadLength() / codec.rtpLength() : 1;
        int frame_length = codec.rtpLength() ? codec.rtpLength() : (int)p.GetPayloadLength();

        // Save last packet time length
        // mLastPacketTimeLength = mFrameCount * mCodec->frameTime();

        // Decode

        for (int i=0; i < frame_count; i++)
        {
            auto decoded_length = codec.decode(p.GetPayloadData() + i * codec.rtpLength(),
                                               frame_length,
                                               output_buffer,
                                               output_capacity);

            result += decoded_length;
        }
    }
    else
        ICELogMedia(<< "RTP packet with tail.");

    return result;
}

bool AudioReceiver::add(const std::shared_ptr<jrtplib::RTPPacket>& p, Codec** codec)
{
    // ICELogInfo(<< "Adding packet No " << p->GetSequenceNumber());
    // Increase codec counter
    mStat.mCodecCount[p->GetPayloadType()]++;

    // Check if codec can be handled
    CodecMap::iterator codecIter = mCodecMap.find(p->GetPayloadType());
    if (codecIter == mCodecMap.end())
    {
        ICELogMedia(<< "Cannot find codec in available codecs");
        return false; // Reject packet with unknown payload type
    }

    // Check if codec is created actually
    if (!codecIter->second)
    {
        // Look for ptype
        for (int codecIndex = 0; codecIndex < mCodecList.count(); codecIndex++)
            if (mCodecList.codecAt(codecIndex).payloadType() == p->GetPayloadType())
                codecIter->second = mCodecList.codecAt(codecIndex).create();
    }

    // Return pointer to codec if needed.get()
    if (codec)
        *codec = codecIter->second.get();

    if (mStat.mCodecName.empty())
        mStat.mCodecName = codecIter->second->name();

    // Estimate time length
    int time_length = 0, payloadLength = p->GetPayloadLength(), ptype = p->GetPayloadType();

    if (!codecIter->second->rtpLength())
        time_length = codecIter->second->frameTime();
    else
        time_length = lround(double(payloadLength) / codecIter->second->rtpLength() * codecIter->second->frameTime());

    // Process jitter
    mJitterStats.process(p.get(), codecIter->second->samplerate());
    mStat.mJitter = static_cast<float>(mJitterStats.get());

    // Check if packet is CNG
    if (payloadLength >= 1 && payloadLength <= 6 && (ptype == 0 || ptype == 8))
        time_length = mLastPacketTimeLength ? mLastPacketTimeLength : 20;
    else
    // Check if packet is too short from time length side
    if (time_length < 2)
    {
        // It will cause statistics to report about bad RTP packet
        // I have to replay last packet payload here to avoid report about lost packet
        mBuffer.add(p, time_length, codecIter->second->samplerate());
        return false;
    }

    // Queue packet to buffer
    auto packet = mBuffer.add(p, time_length, codecIter->second->samplerate()).get();

    if (packet)
    {
        // Check if early decoding configured
        if (mEarlyDecode && *codec)
        {
            // Move data to packet buffer
            size_t available = decode_packet(**codec, *p, mDecodedFrame, sizeof mDecodedFrame);
            packet->pcm().resize(available / 2);
            memcpy(packet->pcm().data(), mDecodedFrame, available / 2);
        }
        return true;
    }
    else
        return false;
}

void AudioReceiver::processDecoded(Audio::DataWindow& output, int options)
{
    // Write to audio dump if requested
    if (mDecodedDump && mDecodedLength)
        mDecodedDump->write(mDecodedFrame, mDecodedLength);

    // Resample to target rate
    bool resample = !(options & DecodeOptions_DontResample);
    makeMonoAndResample(resample ? mCodec->samplerate() : 0,
                        mCodec->channels());

    // Send to output
    output.add(mResampledFrame, mResampledLength);
}

AudioReceiver::DecodeResult AudioReceiver::getAudio(Audio::DataWindow& output, int options, int* rate)
{
    DecodeResult result = DecodeResult_Skip;
    bool had_decode = false;

    // Get next packet from buffer
    RtpBuffer::ResultList rl;
    RtpBuffer::FetchResult fr = mBuffer.fetch(rl);
    switch (fr)
    {
    case RtpBuffer::FetchResult::Gap:
        ICELogDebug(<< "Gap detected.");

        mDecodedLength = mResampledLength = 0;
        if (mCngPacket && mCodec)
        {
            // Synthesize comfort noise. It will be done on AUDIO_SAMPLERATE rate directly to mResampledFrame buffer.
            // Do not forget to send this noise to analysis
            mDecodedLength = mCngDecoder.produce(mCodec->samplerate(), mLastPacketTimeLength,
                                                 reinterpret_cast<short*>(mDecodedFrame), false);
        }
        else
        if (mCodec && mFrameCount && !mCodecSettings.mSkipDecode)
        {
            // Do PLC to mDecodedFrame/mDecodedLength
            if (options & DecodeOptions_SkipDecode)
                mDecodedLength = 0;
            else
                mDecodedLength = mCodec->plc(mFrameCount, mDecodedFrame, sizeof mDecodedFrame);
        }

        if (mDecodedLength)
        {
            processDecoded(output, options);
            result = DecodeResult_Ok;
        }
        break;

    case RtpBuffer::FetchResult::NoPacket:
        ICELogDebug(<< "No packet available in jitter buffer");
        mFailedCount++;
        break;

    case RtpBuffer::FetchResult::RegularPacket:
        mFailedCount = 0;

        for (std::shared_ptr<RtpBuffer::Packet>& p: rl)
        {
            assert(p);
            // Check if previously CNG packet was detected. Emit CNG audio here if needed.
            if (options & DecodeOptions_FillCngGap && mCngPacket && mCodec)
            {
                // Fill CNG audio is server mode is present
                int units = p->rtp()->GetTimestamp() - mCngPacket->GetTimestamp();
                int milliseconds = units / (mCodec->samplerate() / 1000);
                if (milliseconds > mLastPacketTimeLength)
                {
                    int frames100ms = milliseconds / 100;
                    for (int frameIndex = 0; frameIndex < frames100ms; frameIndex++)
                    {
                        if (options & DecodeOptions_SkipDecode)
                            mDecodedLength = 0;
                        else
                            mDecodedLength = mCngDecoder.produce(mCodec->samplerate(), 100,
                                                                 reinterpret_cast<short*>(mDecodedFrame), false);

                        if (mDecodedLength)
                            processDecoded(output, options);
                    }
                    // Do not forget about tail!
                    int tail = milliseconds % 100;
                    if (tail)
                    {
                        if (options & DecodeOptions_SkipDecode)
                            mDecodedLength = 0;
                        else
                            mDecodedLength = mCngDecoder.produce(mCodec->samplerate(), tail,
                                                                 reinterpret_cast<short*>(mDecodedFrame), false);

                        if (mDecodedLength)
                            processDecoded(output, options);
                    }
                    result = DecodeResult_Ok;
                }
            }

            if (mEarlyDecode)
            {
                // ToDo - copy the decoded data to output buffer

            }
            else
            {
                // Find codec by payload type
                int ptype = p->rtp()->GetPayloadType();
                mCodec = mCodecMap[ptype];
                if (mCodec)
                {
                    if (rate)
                        *rate = mCodec->samplerate();

                    // Check if it is CNG packet
                    if ((ptype == 0 || ptype == 8) && p->rtp()->GetPayloadLength() >= 1 && p->rtp()->GetPayloadLength() <= 6)
                    {
                        if (options & DecodeOptions_SkipDecode)
                            mDecodedLength = 0;
                        else
                        {
                            mCngPacket = p->rtp();
                            mCngDecoder.decode3389(p->rtp()->GetPayloadData(), p->rtp()->GetPayloadLength());
                            // Emit CNG mLastPacketLength milliseconds
                            mDecodedLength = mCngDecoder.produce(mCodec->samplerate(), mLastPacketTimeLength,
                                                                 (short*)mDecodedFrame, true);
                            if (mDecodedLength)
                                processDecoded(output, options);
                        }
                        result = DecodeResult_Ok;
                    }
                    else
                    {
                        // Reset CNG packet
                        mCngPacket.reset();

                        // Handle here regular RTP packets
                        // Check if payload length is ok
                        size_t payload_length = p->rtp()->GetPayloadLength();
                        size_t rtp_frame_length = mCodec->rtpLength();

                        int tail = rtp_frame_length ? payload_length % rtp_frame_length : 0;

                        if (!tail)
                        {
                            // Find number of frames
                            mFrameCount = mCodec->rtpLength() ? p->rtp()->GetPayloadLength() / mCodec->rtpLength() : 1;
                            int frameLength = mCodec->rtpLength() ? mCodec->rtpLength() : (int)p->rtp()->GetPayloadLength();

                            // Save last packet time length
                            mLastPacketTimeLength = mFrameCount * mCodec->frameTime();

                            // Decode
                            for (int i=0; i<mFrameCount && !mCodecSettings.mSkipDecode; i++)
                            {
                                if (options & DecodeOptions_SkipDecode)
                                    mDecodedLength = 0;
                                else
                                {
                                    // Trigger the statistics
                                    had_decode = true;

                                    // Decode frame by frame
                                    mDecodedLength = mCodec->decode(p->rtp()->GetPayloadData() + i * mCodec->rtpLength(),
                                                                    frameLength, mDecodedFrame, sizeof mDecodedFrame);
                                    if (mDecodedLength > 0)
                                        processDecoded(output, options);
                                }
                            }
                            result = mFrameCount > 0 ? DecodeResult_Ok : DecodeResult_Skip;

                            // Check for bitrate counter
                            processStatisticsWithAmrCodec(mCodec.get());
                        }
                        else
                        {
                            result = DecodeResult_BadPacket;
                            // ICELogMedia(<< "RTP packet with tail.");
                        }
                    }
                }
            }
        }
        break;

    default:
        assert(0);
    }

    if (had_decode)
    {
        // mStat.mDecodeRequested++;
        if (mLastDecodeTime == 0.0)
            mLastDecodeTime = now_ms();
        else
        {
            float t = now_ms();
            mStat.mDecodingInterval.process(t - mLastDecodeTime);
            mLastDecodeTime = t;
        }
    }
    return result;
}

void AudioReceiver::makeMonoAndResample(int rate, int channels)
{
    // Make mono from stereo - engine works with mono only for now
    mConvertedLength = 0;
    if (channels != AUDIO_CHANNELS)
    {
        if (channels == 1)
            mConvertedLength = Audio::ChannelConverter::monoToStereo(mDecodedFrame, mDecodedLength, mConvertedFrame, mDecodedLength * 2);
        else
            mDecodedLength = Audio::ChannelConverter::stereoToMono(mDecodedFrame, mDecodedLength, mDecodedFrame, mDecodedLength / 2);
    }

    void* frames = mConvertedLength ? mConvertedFrame : mDecodedFrame;
    unsigned length = mConvertedLength ? mConvertedLength : mDecodedLength;

    Audio::Resampler* r = nullptr;
    switch (rate)
    {
    case 8000:     r = &mResampler8; break;
    case 16000:    r = &mResampler16; break;
    case 32000:    r = &mResampler32; break;
    case 48000:    r = &mResampler48; break;
    default:
        memcpy(mResampledFrame, frames, length);
        mResampledLength = length;
        return;
    }

    size_t processedInput = 0;
    mResampledLength = r->processBuffer(frames, length, processedInput, mResampledFrame, r->getDestLength(length));
    // processedInput result value is ignored - it is always equal to length as internal sample rate is 8/16/32/48K
}

Codec* AudioReceiver::findCodec(int payloadType)
{
    MT::CodecMap::const_iterator codecIter = mCodecMap.find(payloadType);
    if (codecIter == mCodecMap.end())
        return nullptr;

    return codecIter->second.get();
}


void AudioReceiver::processStatisticsWithAmrCodec(Codec* c)
{
#if !defined(TARGET_ANDROID) && !defined(TARGET_OPENWRT) && !defined(TARGET_WIN) && !defined(TARGET_RPI) && defined(USE_AMR_CODEC)
    AmrNbCodec* nb = dynamic_cast<AmrNbCodec*>(c);
    AmrWbCodec* wb = dynamic_cast<AmrWbCodec*>(c);

    if (nb != nullptr)
        mStat.mBitrateSwitchCounter = nb->getSwitchCounter();
    else
        if (wb != nullptr)
            mStat.mBitrateSwitchCounter = wb->getSwitchCounter();
#endif
}

int AudioReceiver::getSize() const
{
    int result = 0;
    result += sizeof(*this) + mResampler8.getSize() + mResampler16.getSize() + mResampler32.getSize()
            + mResampler48.getSize();

    if (mCodec)
        result += mCodec->getSize();

    return result;
}

int AudioReceiver::timelengthFor(jrtplib::RTPPacket& p)
{
    CodecMap::iterator codecIter = mCodecMap.find(p.GetPayloadType());
    if (codecIter == mCodecMap.end())
        return 0;

    PCodec codec = codecIter->second;
    if (codec)
    {
        int frame_count = 0;
        if (codec->rtpLength() != 0)
        {
            frame_count = static_cast<int>(p.GetPayloadLength() / codec->rtpLength());
            if (p.GetPayloadType() == 9/*G729A silence*/ && p.GetPayloadLength() % codec->rtpLength())
                frame_count++;
        }
        else
            frame_count = 1;

        return frame_count * codec->frameTime();
    }
    else
        return 0;
}

int AudioReceiver::samplerateFor(jrtplib::RTPPacket& p)
{
    CodecMap::iterator codecIter = mCodecMap.find(p.GetPayloadType());
    if (codecIter != mCodecMap.end())
    {
        PCodec codec = codecIter->second;
        if (codec)
            return codec->samplerate();
    }

    return 8000;
}

// ----------------------- DtmfReceiver -------------------
DtmfReceiver::DtmfReceiver(Statistics& stat)
    :Receiver(stat)
{
}

DtmfReceiver::~DtmfReceiver()
{
}

void DtmfReceiver::add(std::shared_ptr<RTPPacket> /*p*/)
{
}
