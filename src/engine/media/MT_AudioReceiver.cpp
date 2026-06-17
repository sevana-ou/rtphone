/* Copyright(C) 2007-2026 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../engine_config.h"
#include "MT_AudioReceiver.h"
#include "MT_AudioCodec.h"
#include "MT_CngHelper.h"
#include "MT_Dtmf.h"
#include "../helper/HL_Log.h"
#include "../helper/HL_Time.h"
#include "../audio/Audio_Interface.h"
#include "../audio/Audio_Resampler.h"
#include <cmath>
#include <iostream>

#if !defined(TARGET_ANDROID) && !defined(TARGET_OPENWRT) && !defined(TARGET_WIN) && !defined(TARGET_RPI) && defined(USE_AMR_CODEC)
# include "MT_AmrCodec.h"
#endif

#include <algorithm>

#define LOG_SUBSYSTEM "media"

using namespace MT;

// ----------------- RtpBuffer::Packet --------------
RtpBuffer::Packet::Packet(const std::shared_ptr<RTPPacket>& packet, std::chrono::milliseconds timelength, int samplerate)
    :mRtp(packet), mTimelength(timelength), mSamplerate(samplerate)
{}

std::shared_ptr<RTPPacket> RtpBuffer::Packet::rtp() const
{
    return mRtp;
}

std::chrono::milliseconds RtpBuffer::Packet::timelength() const
{
    return mTimelength;
}

int RtpBuffer::Packet::samplerate() const
{
    return mSamplerate;
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
    if (mAddCounter)
        ICELogDebug(<< "Number of add packets: " << mAddCounter << ", number of retrieved packets " << mReturnedCounter);
}

void RtpBuffer::setHigh(std::chrono::milliseconds t)
{
    mHigh = t;
}

std::chrono::milliseconds RtpBuffer::high() const
{
    return mHigh;
}

void RtpBuffer::setLow(std::chrono::milliseconds t)
{
    mLow = t;
}

std::chrono::milliseconds RtpBuffer::low() const
{
    return mLow;
}

void RtpBuffer::setPrebuffer(std::chrono::milliseconds t)
{
    mPrebuffer = t;
}

std::chrono::milliseconds RtpBuffer::prebuffer() const
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

std::shared_ptr<RtpBuffer::Packet> RtpBuffer::add(const std::shared_ptr<jrtplib::RTPPacket>& packet, std::chrono::milliseconds timelength, int rate)
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
    mStat.mSsrc = packet->GetSSRC();

    // Update jitter
    ICELogMedia(<< "Adding new packet seqno " << packet->GetSequenceNumber() << " into jitter buffer");
    mAddCounter++;

    // Look for maximum&minimal sequence number; check for dublicates
    unsigned maxno = 0, minno = 0xFFFFFFFF;

    // New sequence number
    unsigned newSeqno = packet->GetExtendedSequenceNumber();

    for (auto& p: mPacketList)
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
    auto available = findTimelength();

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
            ICELogMedia(<< "Available " << available << " with limit " << mHigh);

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

void RtpBuffer::trimToHighWater(size_t maxPackets)
{
    Lock l(mGuard);

    auto total = findTimelength();

    // Drop the oldest packet while either bound is exceeded: the time-based
    // high-water mark (mHigh, when set) or, if maxPackets != 0, the packet-count
    // cap. Always keep at least one packet so loss/gap accounting has a reference.
    while (mPacketList.size() > 1 &&
           ((0ms != mHigh && total > mHigh) ||
            (maxPackets != 0 && mPacketList.size() > maxPackets)))
    {
        ICELogMedia( << "Dropping RTP packets from jitter buffer");
        total -= mPacketList.front()->timelength();

        // Before advancing mLastSeqno over the dropped packet, record a loss event for any
        // sequence-number gap on the wire between the previous packet we saw and this one.
        // Without this, drops silently mask real packet loss that happened between them.
        auto droppingPacket = mPacketList.front();
        uint32_t droppingSeq = droppingPacket->rtp()->GetExtendedSequenceNumber();
        if (mLastSeqno)
        {
            int gap = (int64_t)droppingSeq - (int64_t)*mLastSeqno - 1;
            if (gap > 0)
            {
                mStat.mPacketLoss += gap;
                if (mStat.mPacketLossTimeline.empty() || (mStat.mPacketLossTimeline.back().mEndSeqno != droppingSeq))
                {
                    auto gapStart = RtpHelper::toMicroseconds(*mLastReceiveTime);
                    auto gapEnd = RtpHelper::toMicroseconds(droppingPacket->rtp()->GetReceiveTime());
                    mStat.mPacketLossTimeline.emplace_back(PacketLossEvent{.mStartSeqno = *mLastSeqno,
                                                                           .mEndSeqno = droppingSeq,
                                                                           .mGap = gap,
                                                                           .mTimestampStart = gapStart,
                                                                           .mTimestampEnd = gapEnd});
                }
            }
        }

        // Save it as last packet however - to not confuse loss packet counter
        mFetchedPacket = droppingPacket;
        mLastSeqno = droppingSeq;
        mLastReceiveTime = mFetchedPacket->rtp()->GetReceiveTime();

        // Erase from packet list
        mPacketList.erase(mPacketList.begin());

        // Increase number in statistics
        mStat.mPacketDropped++;
    }
}

RtpBuffer::FetchResult RtpBuffer::fetch()
{
    Lock l(mGuard);

    FetchResult result;

    // Bound the buffer to the high-water mark before fetching.
    trimToHighWater();

    // See how much audio is buffered now.
    auto total = findTimelength();

    if (total < mLow || total == 0ms)
    {
        // Still not prebuffered
        result = {FetchResult::Status::NoPacket};
    }
    else
    {
        if (mLastSeqno) // It means we had previous packet
        {
            if (mPacketList.empty())
            {
                // Don't increase counter of lost packets here; maybe it is DTX
                result = {FetchResult::Status::NoPacket};
            }
            else
            {
                // Current sequence number ?
                auto& packet = *mPacketList.front();
                uint32_t seqno = packet.rtp()->GetExtendedSequenceNumber();

                // Gap between new packet and previous on
                int gap = (int64_t)seqno - (int64_t)*mLastSeqno - 1;
                if (gap > 0)
                {
                    // std::cout << "Increase the packet loss for SSRC " << std::hex << mSsrc << std::endl;
                    mStat.mPacketLoss += gap;

                    // Report is the onetime; there is no many sequential 1-packet gap reports
                    if (mStat.mPacketLossTimeline.empty() || (mStat.mPacketLossTimeline.back().mEndSeqno != seqno))
                    {
                        auto gapStart = RtpHelper::toMicroseconds(*mLastReceiveTime);
                        auto gapEnd = RtpHelper::toMicroseconds(packet.rtp()->GetReceiveTime());
                        mStat.mPacketLossTimeline.emplace_back(PacketLossEvent{.mStartSeqno = *mLastSeqno,
                                                                               .mEndSeqno = seqno,
                                                                               .mGap = gap,
                                                                               .mTimestampStart = gapStart,
                                                                               .mTimestampEnd = gapEnd});
                    }

                    // ToDo: here we should decide smth - 2-packet gap shoud report Status::Gap two times at least; but current implementation gives only one.
                    // It is not big problem - as gap is detected when we have smth to return usually
                    mLastSeqno = seqno;
                    mLastReceiveTime = packet.rtp()->GetReceiveTime();
                    result = {FetchResult::Status::Gap};
                }
                else
                {
                    result = {FetchResult::Status::RegularPacket, mPacketList.front()};

                    // Save last returned normal packet
                    mFetchedPacket = result.mPacket;
                    mLastSeqno = result.mPacket->rtp()->GetExtendedSequenceNumber();
                    mLastReceiveTime = result.mPacket->rtp()->GetReceiveTime();

                    // Remove returned packet from the list
                    mPacketList.erase(mPacketList.begin());
                }
            }
        }
        else
        {
            // See if prebuffer limit is reached
            if (findTimelength() >= mPrebuffer && !mPacketList.empty())
            {
                // Normal packet will be returned
                result = {FetchResult::Status::RegularPacket, mPacketList.front()};

                // Remember returned packet
                mFetchedPacket = result.mPacket;
                mLastSeqno = result.mPacket->rtp()->GetExtendedSequenceNumber();
                mLastReceiveTime = result.mPacket->rtp()->GetReceiveTime();

                // Remove returned packet from buffer list
                mPacketList.erase(mPacketList.begin());
            }
            else
            {
                ICELogMedia(<< "Jitter buffer was not prebuffered yet; resulting no packet");
                result = {FetchResult::Status::NoPacket};
            }
        }
    }

    if (result.mStatus != FetchResult::Status::NoPacket)
        mReturnedCounter++;

    return result;
}

std::chrono::milliseconds RtpBuffer::findTimelength()
{
    std::chrono::milliseconds r = 0ms;
    for (const auto& p: mPacketList)
        r += p->timelength();
    return r;
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
{}

Receiver::~Receiver()
{}

//-------------- AudioReceiver ----------------
AudioReceiver::AudioReceiver(const CodecList::Settings& settings, MT::Statistics &stat)
    :Receiver(stat), mRtpBuffer(stat), mDtmfBuffer(stat), mCodecSettings(settings), mCodecList(settings), mDtmfReceiver(stat)
{
    // Init resamplers
    mResampler8.start(AUDIO_CHANNELS, 8000, AUDIO_SAMPLERATE);
    mResampler16.start(AUDIO_CHANNELS, 16000, AUDIO_SAMPLERATE);
    mResampler32.start(AUDIO_CHANNELS, 32000, AUDIO_SAMPLERATE);
    mResampler48.start(AUDIO_CHANNELS, 48000, AUDIO_SAMPLERATE);

    // Init codecs
    mCodecList.setSettings(settings);
    mCodecList.fillCodecMap(mCodecMap);

    // 10 seconds is the maximum length of decoded audio in single step
    // It is important - DTX may produce silence up to few seconds easily
    mAvailable.setCapacity(AUDIO_SAMPLERATE * 10 * sizeof(short));

    mDtmfBuffer.setPrebuffer(0ms);
    mDtmfBuffer.setLow(0ms);
    mDtmfBuffer.setHigh(1ms);

    // Avoid collecting too much data
    mRtpBuffer.setHigh(240ms);

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

    if (mRequestedAudio != 0ms)
        ICELogDebug(<< "Requested " << mRequestedAudio << ", produced " << mProducedAudio);
    if (mDecodeCount)
        ICELogDebug(<< "Average interval between packet decoding " << mIntervalBetweenDecode / mDecodeCount);
}

// Update codec settings
void AudioReceiver::setCodecSettings(const CodecList::Settings& codecSettings)
{
    if (mCodecSettings == codecSettings)
        return;

    mCodecSettings = codecSettings;
    mCodecList.setSettings(mCodecSettings); // This builds factory list with proper payload types according to payload types in settings

    // Rebuild codec map from factory list
    mCodecList.fillCodecMap(mCodecMap);
}

CodecList::Settings& AudioReceiver::getCodecSettings()
{
    return mCodecSettings;
}

Codec* AudioReceiver::add(const std::shared_ptr<jrtplib::RTPPacket>& p)
{
    Codec* codec = nullptr;

    // Estimate time length
    int time_length = 0,
        samplerate = 8000,
        payloadLength = p->GetPayloadLength(),
        ptype = p->GetPayloadType();

    // ICELogMedia(<< "Adding packet No " << p->GetSequenceNumber());

    // Increase codec counter
    mStat.mCodecCount[ptype]++;

    // Check if we deal with telephone-event
    if (p->GetPayloadType() == mCodecSettings.mTelephoneEvent)
    {
        codec = nullptr;
        mDtmfBuffer.add(p, 10ms, 8000);
    }
    else
    {
        // Look for codec
        // Check if codec can be handled
        auto codecIter = mCodecMap.find(ptype);
        if (codecIter != mCodecMap.end())
        {
            // Check if codec is creating lazily
            if (!codecIter->second)
            {
                codecIter->second = mCodecList.createCodecByPayloadType(ptype);
            }
            codec = codecIter->second.get();

            // Return pointer to codec if needed.get()
            if (mStat.mCodecName.empty() && codec)
                mStat.mCodecName = codec->name();


            if (!codec)
                time_length = 10;
            else
            if (!codec->rtpLength())
                time_length = codec->frameTime();
            else
                time_length = lround(double(payloadLength) / codec->rtpLength() * codec->frameTime());

            if (codec)
                samplerate = codec->samplerate();
        }

        // Process jitter anyway - can we decode payload or not
        mJitterStats.process(p.get(), samplerate);
        mStat.mJitter = static_cast<float>(mJitterStats.get());

        if (!codec)
            return nullptr;

        // Check if packet is CNG
        if (payloadLength >= 1 && payloadLength <= 6 && (ptype == 0 || ptype == 8))
            time_length = mLastPacketTimeLength ? mLastPacketTimeLength : 20;
        else
        // Check if packet is too short from time length side - smth strange with found codec...
        if (time_length < 2)
        {
            // It will cause statistics to report about bad RTP packet
            // I have to replay last packet payload here to avoid report about lost packet
            mRtpBuffer.add(p, std::chrono::milliseconds(time_length), samplerate);
            return nullptr;
        }

        // Queue packet to buffer
        mRtpBuffer.add(p, std::chrono::milliseconds(time_length), samplerate).get();
    }
    return codec;
}

void AudioReceiver::processDecoded(Audio::DataWindow& output, DecodeOptions options)
{
    // Write to audio dump if requested
    if (mDecodedDump && mDecodedLength)
        mDecodedDump->write(mDecodedFrame.data(), mDecodedLength);

    // Resample to target rate
    makeMonoAndResample(options.mResampleToMainRate ? mCodec->samplerate() : 0, mCodec->channels());

    // Send to output
    output.add(mResampledFrame.data(), mResampledLength);
}

void AudioReceiver::produceSilence(std::chrono::milliseconds length, Audio::DataWindow& output, DecodeOptions options)
{
    if (!mCodec)
        return;

    // Fill mDecodeBuffer as much as needed and call processDecoded()
    // Depending on used codec mono or stereo silence should be produced

    size_t chunks = length.count() / 10;
    size_t tail = length.count() % 10;
    size_t chunk_size = 10 * sizeof(int16_t) * mCodec->samplerate() / 1000 * mCodec->channels();
    size_t tail_size = tail * sizeof(int16_t) * mCodec->samplerate() / 1000 * mCodec->channels();
    for (size_t i = 0; i < chunks; i++)
    {
        memset(mDecodedFrame.data(), 0, chunk_size);
        mDecodedLength = chunk_size;
        processDecoded(output, options);
    }
    if (tail)
    {
        memset(mDecodedFrame.data(), 0, tail_size);
        mDecodedLength = tail_size;
        processDecoded(output, options);
    }
}

void AudioReceiver::produceCNG(std::chrono::milliseconds length, Audio::DataWindow& output, DecodeOptions options)
{
    int frames100ms = length.count() / 100;
    for (int frameIndex = 0; frameIndex < frames100ms; frameIndex++)
    {
        if (options.mSkipDecode)
            mDecodedLength = 0;
        else
            mDecodedLength = mCngDecoder.produce(mCodec->samplerate(), 100, mDecodedFrame.data(), false);

        if (mDecodedLength)
            processDecoded(output, options);
    }

    // Do not forget about tail!
    int tail = length.count() % 100;
    if (tail)
    {
        if (options.mSkipDecode)
            mDecodedLength = 0;
        else
            mDecodedLength = mCngDecoder.produce(mCodec->samplerate(), tail, reinterpret_cast<short*>(mDecodedFrame.data()), false);

        if (mDecodedLength)
            processDecoded(output, options);
    }
}

AudioReceiver::DecodeResult AudioReceiver::decodeGapTo(Audio::DataWindow& output, DecodeOptions options)
{
    ICELogMedia(<< "Gap detected.");

    mDecodedLength = mResampledLength = 0;
    if (mCngPacket && mCodec)
    {
        if (mCngPacket->rtp()->GetPayloadType() == 13)
        {
            // Synthesize comfort noise. It will be done on AUDIO_SAMPLERATE rate directly to mResampledFrame buffer.
            // Do not forget to send this noise to analysis
            mDecodedLength = mCngDecoder.produce(mCodec->samplerate(), mLastPacketTimeLength, reinterpret_cast<short*>(mDecodedFrame.data()), false);
        }
        else
            decodePacketTo(output, options, mCngPacket);
    }
    else
    if (mCodec && mFrameCount && !mCodecSettings.mSkipDecode)
    {
        // Do PLC to mDecodedFrame/mDecodedLength
        if (options.mSkipDecode)
            mDecodedLength = 0;
        else
        {
            mDecodedLength = mCodec->plc(mFrameCount, {(uint8_t*)mDecodedFrame.data(), mDecodedFrame.size() * sizeof(int16_t)});
            if (!mDecodedLength)
            {
                // PLC is not support or failed
                // So substitute the silence
                size_t nr_of_samples = mCodec->frameTime() * mCodec->samplerate() / 1000 * sizeof(short);
                mDecodedLength = nr_of_samples * sizeof(short);
                memset(mDecodedFrame.data(), 0, mDecodedLength);
            }
        }
    }

    if (mDecodedLength)
    {
        processDecoded(output, options);
        return {.mStatus = DecodeResult::Status::Ok, .mSamplerate = mCodec->samplerate(), .mChannels = mCodec->channels()};
    }
    else
        return {.mStatus = DecodeResult::Status::Skip};
}

AudioReceiver::DecodeResult AudioReceiver::decodePacketTo(Audio::DataWindow& output, DecodeOptions options, const std::shared_ptr<RtpBuffer::Packet>& packet)
{
    if (!packet || !packet->rtp())
        return {DecodeResult::Status::Skip};

    DecodeResult result = {.mStatus = DecodeResult::Status::Skip};
    auto& rtp = *packet->rtp(); // Syntax sugar

    mFailedCount = 0;

    // Check if we need to emit silence - it may happen in the case if next packet has RTP timestamp much beyond the previous one; maybe DTX was active.
    if (mLastPacketTimestamp && mLastPacketTimeLength && mCodec)
    {
         int units = rtp.GetTimestamp() - *mLastPacketTimestamp;
         int milliseconds = units / (mCodec->samplerate() / 1000);
         if (milliseconds > mLastPacketTimeLength)
         {
             auto silenceLength = std::chrono::milliseconds(milliseconds - mLastPacketTimeLength);
             ICELogDebug(<< "Emit " << silenceLength << " silence while requested " << options.mElapsed);
             silenceLength = std::min(silenceLength, options.mElapsed);
             if (mCngPacket && options.mFillGapByCNG)
                 produceCNG(silenceLength, output, options);
             else
                 produceSilence(silenceLength, output, options);
         }
     }

    mLastPacketTimestamp = rtp.GetTimestamp();

    // Find codec by payload type
    int ptype = rtp.GetPayloadType();

    // Look into mCodecMap if exists
    auto codecIter = mCodecMap.find(ptype);
    if (codecIter == mCodecMap.end())
        return  {};

    if (!codecIter->second)
        codecIter->second = mCodecList.createCodecByPayloadType(ptype);

    mCodec = codecIter->second;
    if (mCodec)
    {
        result.mChannels = mCodec->channels();
        result.mSamplerate = mCodec->samplerate();

        // Check if it is CNG packet
        if (((ptype == 0 || ptype == 8) && rtp.GetPayloadLength() >= 1 && rtp.GetPayloadLength() <= 6) || rtp.GetPayloadType() == 13)
        {
            if (options.mSkipDecode)
                mDecodedLength = 0;
            else
            {
                ICELogDebug(<< "Decoding CNG");
                mCngPacket = packet;
                mCngDecoder.decode3389(rtp.GetPayloadData(), rtp.GetPayloadLength());

                // Emit CNG mLastPacketLength milliseconds
                mDecodedLength = mCngDecoder.produce(mCodec->samplerate(), mLastPacketTimeLength, (short*)mDecodedFrame.data(), true);
                if (mDecodedLength)
                    processDecoded(output, options);
            }
            result.mStatus = DecodeResult::Status::Ok;
        }
        else
        {
            // Reset CNG packet as we get regular RTP packet
            mCngPacket.reset();

            // Handle here regular RTP packets
            // Check if payload length is ok
            size_t payload_length = rtp.GetPayloadLength();
            size_t rtp_frame_length = mCodec->rtpLength();

            int tail = rtp_frame_length ? payload_length % rtp_frame_length : 0;

            if (!tail)
            {
                // Find number of frames
                mFrameCount = mCodec->rtpLength() ? rtp.GetPayloadLength() / mCodec->rtpLength() : 1;
                int frameLength = mCodec->rtpLength() ? mCodec->rtpLength() : (int)rtp.GetPayloadLength();

                // Save last packet time length
                mLastPacketTimeLength = mFrameCount * mCodec->frameTime();

                // Decode
                for (int i=0; i<mFrameCount && !mCodecSettings.mSkipDecode; i++)
                {
                    if (options.mSkipDecode)
                        mDecodedLength = 0;
                    else
                    {
                        // Decode frame by frame
                        auto codecInput = std::span{rtp.GetPayloadData() + i * mCodec->rtpLength(), (size_t)frameLength};
                        auto codecOutput = std::span{(uint8_t*)mDecodedFrame.data(), mDecodedFrame.size() * sizeof(int16_t)};
                        auto r = mCodec->decode(codecInput, codecOutput);
                        mDecodedLength = r.mDecoded;
                        if (mDecodedLength > 0)
                            processDecoded(output, options);

                        // What is important - here we may have packet marked as CNG
                        if (r.mIsCng)
                            mCngPacket = packet;
                    }
                }
                result.mStatus = mFrameCount > 0 ? DecodeResult::Status::Ok : DecodeResult::Status::Skip;

                // Check for bitrate counter
                updateAmrCodecStats(mCodec.get());
            }
            else
            {
                // RTP packet with tail - it should not happen
                result.mStatus = DecodeResult::Status::BadPacket;
            }
        }
    }
    return result;
}

AudioReceiver::DecodeResult AudioReceiver::decodeEmptyTo(Audio::DataWindow& output, DecodeOptions options)
{
    // There are two cases
    // First is we have no ready time estimated how much audio should be emitted i.e. audio is decoded right after the next packet arrives.
    // In this case we just skip the analysis - we should not be called in this situation
    if (options.mElapsed == 0ms || !mCodec)
        return {.mStatus = DecodeResult::Status::Skip};

    // No packet available at all (and no previous CNG packet) - so return the silence
    if (options.mElapsed != 0ms && mCodec)
    {
        Audio::Format fmt = options.mResampleToMainRate ? Audio::Format(AUDIO_SAMPLERATE, 1) : mCodec->getAudioFormat();
        if (mCngPacket)
        {
            // Try to decode it - replay previous audio decoded or use CNG decoder (if payload type is 13)
            if (mCngPacket->rtp()->GetPayloadType() == 13)
            {
                // Using latest CNG packet to produce comfort noise.
                // Clamp the produced amount to the remaining capacity of the output window -
                // the CNG decoder writes straight into its buffer.
                size_t bytesPerMs = (size_t)fmt.rate() / 1000 * sizeof(short) * fmt.channels();
                size_t room = output.capacity() - output.filled();
                int ms = bytesPerMs ? (int)std::min<int64_t>(options.mElapsed.count(), (int64_t)(room / bytesPerMs)) : 0;
                if (ms <= 0)
                    return {.mStatus = DecodeResult::Status::Skip};
                auto produced = mCngDecoder.produce(fmt.rate(), ms, (short*)(output.mutableData() + output.filled()), false);
                output.setFilled(output.filled() + produced);
                return {.mStatus = DecodeResult::Status::Ok, .mSamplerate = fmt.rate(), .mChannels = fmt.channels()};
            }
            else
            {
                // Here we have another packet marked as CNG - for another decoder
                // Just decode it +1 time
                return decodePacketTo(output, options, mCngPacket);
            }
        }
        else
        {
            // Emit silence if codec information is available - it is to properly handle the gaps
            auto avail = output.getTimeLength(fmt);
            if (options.mElapsed > avail)
                output.addZero(fmt.sizeFromTime(options.mElapsed - avail));
        }
    }

    mFailedCount++;
    return {.mStatus = DecodeResult::Status::Skip};
}

void MT::AudioReceiver::processDtmf()
{
    if (mDtmfBuffer.getCount())
    {
        auto fr = mDtmfBuffer.fetch();
        if (fr.mPacket && fr.mStatus == RtpBuffer::FetchResult::Status::RegularPacket)
            mDtmfReceiver.add(fr.mPacket->rtp());
    }
}

void MT::AudioReceiver::updateDecodingTimeStatistics()
{
    if (!mDecodeTimestamp)
        mDecodeTimestamp = std::chrono::steady_clock::now();
    else
    {
        auto t = std::chrono::steady_clock::now();
        mStat.mDecodingInterval.process(std::chrono::duration_cast<std::chrono::milliseconds>(t - *mDecodeTimestamp).count());
        mDecodeTimestamp = t;
    }
}

AudioReceiver::DecodeResult AudioReceiver::getAudioTo(Audio::DataWindow& output, DecodeOptions options)
{
    // ICELogDebug(<< "getAudioTo() for " << options.mElapsed);
    assert (options.mElapsed != 0ms);

    // First decode on this receiver: allocate the scratch buffers. Network-MOS-only
    // streams never reach this point, so they never pay for them.
    ensureDecodeBuffers();

    // Increase counter of requested audio
    mRequestedAudio += options.mElapsed;

    DecodeResult result = {.mStatus = DecodeResult::Status::Skip};

    // Process RFC2833 here; it doesn't result in any audio - only callbacks and statistics
    processDtmf();

    // How much time length audio we produced here
    auto produced = 0ms;
    Audio::Format fmt;

    // Have we anything from the previous decode attempts ?
    if (mAvailable.filled())
    {
        // Find what audio format is used in mAvailable data
        fmt = options.mResampleToMainRate ? Audio::Format(AUDIO_SAMPLERATE, 1) : mCodec->getAudioFormat();

        // How much milliseconds are available ?
        auto availTime = mAvailable.getTimeLength(fmt);
        if (availTime != 0ms)
        {
            // How much we can consume from the mAvailable buffer ?
            std::chrono::milliseconds resultTime = std::min(availTime, options.mElapsed);

            // Number of bytes
            mAvailable.moveTo(output, fmt.sizeFromTime(resultTime));

            // Increase the counter of produced milliseconds
            produced += resultTime;
        }
    }

    while (produced < options.mElapsed)
    {
        // Get next packet from buffer
        RtpBuffer::FetchResult fr = mRtpBuffer.fetch();

        // Decode to mAvailable buffer
        switch (fr.mStatus)
        {
        case RtpBuffer::FetchResult::Status::Gap:           result = decodeGapTo(mAvailable, options.decreaseElapsedBy(produced));                                                      break;
        case RtpBuffer::FetchResult::Status::NoPacket:      result = decodeEmptyTo(mAvailable, options.decreaseElapsedBy(produced));                                                    break;
        case RtpBuffer::FetchResult::Status::RegularPacket: result = decodePacketTo(mAvailable, options.decreaseElapsedBy(produced), fr.mPacket);   updateDecodeIntervalStatistics();   break;
        default:
            assert(0);
        }

        // Was there decoding at all ?
        if (!mCodec)
            break; // No sense to continue - we have no information at all

        fmt = options.mResampleToMainRate ? Audio::Format(AUDIO_SAMPLERATE, 1) : mCodec->getAudioFormat();
        result.mSamplerate  = fmt.rate();
        result.mChannels    = fmt.channels();

        // How much milliseconds we have in audio buffer ?
        auto bufferAvailable = mAvailable.getTimeLength(fmt);
        if (bufferAvailable == 0ms)
            break; // No sense to continue - decoding / CNG / PLC stopped totally

        // How much data should be moved to result buffer ?
        std::chrono::milliseconds resultTime = std::min(bufferAvailable, options.mElapsed - produced);
        mAvailable.moveTo(output, fmt.sizeFromTime(resultTime));
        produced += resultTime;
    }

    if (produced != 0ms)
    {
        result.mStatus = DecodeResult::Status::Ok;
        updateDecodingTimeStatistics();
    }

    mProducedAudio += produced;
    // ICELogDebug(<< "Requested " << options.mElapsed << ", produced " << produced << ", remains " << mAvailable.getTimeLength(fmt) << ", packets " << getRtpBuffer().getCount());
    return result;
}

void AudioReceiver::ensureDecodeBuffers()
{
    // Allocate the decode/convert/resample scratch buffers to full capacity on the
    // first decode. mDecodedFrame being empty means none are allocated yet; they
    // are always allocated together, so checking one is enough.
    if (mDecodedFrame.empty())
    {
        mDecodedFrame.resize(MT_MAX_DECODEBUFFER);
        mConvertedFrame.resize(MT_MAX_DECODEBUFFER * 2);
        mResampledFrame.resize(MT_MAX_DECODEBUFFER);
    }
}

void AudioReceiver::makeMonoAndResample(int rate, int channels)
{
    // Make mono from stereo - engine works with mono only for now
    mConvertedLength = 0;
    if (channels != AUDIO_CHANNELS)
    {
        if (channels == 1)
            mConvertedLength = Audio::ChannelConverter::monoToStereo(mDecodedFrame.data(), mDecodedLength, mConvertedFrame.data(), mDecodedLength * 2);
        else
            mDecodedLength = Audio::ChannelConverter::stereoToMono(mDecodedFrame.data(), mDecodedLength, mDecodedFrame.data(), mDecodedLength / 2);
    }

    void* frames = mConvertedLength ? (void*)mConvertedFrame.data() : (void*)mDecodedFrame.data();
    unsigned length = mConvertedLength ? mConvertedLength : mDecodedLength;

    Audio::Resampler* r = nullptr;
    switch (rate)
    {
    case 8000:     r = &mResampler8; break;
    case 16000:    r = &mResampler16; break;
    case 32000:    r = &mResampler32; break;
    case 48000:    r = &mResampler48; break;
    default:
        memcpy(mResampledFrame.data(), frames, length);
        mResampledLength = length;
        return;
    }

    size_t processedInput = 0;
    mResampledLength = r->processBuffer(frames, length, processedInput, mResampledFrame.data(), r->getDestLength(length));
    // processedInput result value is ignored - it is always equal to length as internal sample rate is 8/16/32/48K
}

Codec* AudioReceiver::findCodec(int payloadType)
{
    MT::CodecMap::const_iterator codecIter = mCodecMap.find(payloadType);
    if (codecIter == mCodecMap.end())
        return nullptr;

    return codecIter->second.get();
}


void AudioReceiver::updateAmrCodecStats(Codec* c)
{
#if !defined(TARGET_ANDROID) && !defined(TARGET_OPENWRT) && !defined(TARGET_WIN) && !defined(TARGET_RPI) && defined(USE_AMR_CODEC)
    AmrNbCodec* nb = dynamic_cast<AmrNbCodec*>(c);
    AmrWbCodec* wb = dynamic_cast<AmrWbCodec*>(c);

    if (nb != nullptr)
    {
        mStat.mBitrateSwitchCounter = nb->getSwitchCounter();
        mStat.mCng = nb->getCngCounter();
    }
    else
    if (wb != nullptr)
    {
        mStat.mBitrateSwitchCounter = wb->getSwitchCounter();
        mStat.mCng = wb->getCngCounter();
    }
#endif
}

int AudioReceiver::getSize() const
{
    int result = 0;
    result += sizeof(*this) + mResampler8.getSize() + mResampler16.getSize() + mResampler32.getSize() + mResampler48.getSize();

    if (mCodec)
        ; // ToDo: need the way to calculate size of codec instances

    return result;
}

AudioReceiver::MediaInfo AudioReceiver::infoFor(jrtplib::RTPPacket& p)
{
    CodecMap::iterator codecIter = mCodecMap.find(p.GetPayloadType());
    if (codecIter == mCodecMap.end())
        return {};
    PCodec codec = codecIter->second;
    if (!codec)
        return {};

    std::chrono::milliseconds packetTime = 0ms;

    if (codec->rtpLength() != 0)
    {
        int frameCount = static_cast<int>(p.GetPayloadLength() / codec->rtpLength());
        if (p.GetPayloadType() == 9/*G729A silence*/ && p.GetPayloadLength() % codec->rtpLength())
            frameCount++;

        packetTime = std::chrono::milliseconds(frameCount * codec->frameTime());
    }
    else
    if (typeid(*codec) == typeid(OpusCodec))
    {
        OpusCodec* oc = dynamic_cast<OpusCodec*>(codec.get());
        assert(oc);
        size_t samplesCount = oc->getNumberOfSamples({p.GetPayloadData(), p.GetPayloadLength()});
        int sampleratePerMs = codec->samplerate() / 1000;
        packetTime = std::chrono::milliseconds(samplesCount / sampleratePerMs);
    }
    else
    {
        packetTime = std::chrono::milliseconds(codec->frameTime());
    }

    return {packetTime, codec->samplerate()};
}

void AudioReceiver::updateDecodeIntervalStatistics()
{
    auto now = std::chrono::steady_clock::now();
    if (mLastDecodeTimestamp)
    {
        mIntervalBetweenDecode += std::chrono::duration_cast<std::chrono::microseconds>(now - *mLastDecodeTimestamp);
        mDecodeCount ++;
    }
    mLastDecodeTimestamp = now;
}

// ----------------------- DtmfReceiver -------------------
DtmfReceiver::DtmfReceiver(Statistics& stat)
    :Receiver(stat)
{}

DtmfReceiver::~DtmfReceiver()
{}

void DtmfReceiver::add(const std::shared_ptr<RTPPacket>& p)
{
    auto ev = DtmfBuilder::parseRfc2833({p->GetPayloadData(), p->GetPayloadLength()});
    if (!ev.mTone)
        return; // Malformed or unknown event payload

    // A new digit begins when the tone changes, or when the same tone starts
    // again after the previous occurrence ended. Retransmitted start/end
    // packets keep both fields unchanged and are ignored. The end packet of
    // the current tone only updates state - the digit was already reported.
    bool newEvent = (ev.mTone != mEvent) || (mEventEnded && !ev.mEnd);

    if (newEvent)
    {
        if (mCallback)
            mCallback(ev.mTone);

        // Queue statistics item
        mStat.mDtmf2833Timeline.emplace_back(Dtmf2833Event{.mTone = ev.mTone,
                                                           .mTimestamp = RtpHelper::toMicroseconds(p->GetReceiveTime())});
    }

    mEvent = ev.mTone;
    mEventEnded = ev.mEnd;
}
