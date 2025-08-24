/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MT_AudioStream.h"
#include "MT_Dtmf.h"
#include "../helper/HL_StreamState.h"
#include "../helper/HL_Log.h"
#include "../audio/Audio_Resampler.h"
#include "../audio/Audio_Interface.h"

#include "jrtplib/src/rtpipv4address.h"
#include "jrtplib/src/rtpipv6address.h"
#include "jrtplib/src/rtppacket.h"
#include "jrtplib/src/rtptransmitter.h"
#include "jrtplib/src/rtpsessionparams.h"

#define LOG_SUBSYSTEM "AudioStream"

//#define DUMP_SENDING_AUDIO

using namespace MT;
AudioStream::AudioStream(const CodecList::Settings& settings)
    :mPacketTime(0), mEncodedTime(0), mCodecSettings(settings),
      mRemoteTelephoneCodec(0), mRtpSession(), mTransmittingPayloadType(-1),
      mRtpSender(mStat)
{
    mOutputBuffer.setCapacity(16384);
    mCapturedAudio.setCapacity(16384);
    mCaptureResampler8.start(AUDIO_CHANNELS, AUDIO_SAMPLERATE, 8000);
    mCaptureResampler16.start(AUDIO_CHANNELS, AUDIO_SAMPLERATE, 16000);
    mCaptureResampler32.start(AUDIO_CHANNELS, AUDIO_SAMPLERATE, 32000);
    mCaptureResampler48.start(AUDIO_CHANNELS, AUDIO_SAMPLERATE, 48000);

    // Configure transmitter
    jrtplib::RTPExternalTransmissionParams params(&mRtpSender, 0);

    jrtplib::RTPSessionParams sessionParams;
    sessionParams.SetAcceptOwnPackets(true);
    sessionParams.SetMaximumPacketSize(MT_MAXRTPPACKET);
    sessionParams.SetResolveLocalHostname(false);
    sessionParams.SetUsePollThread(false);
    sessionParams.SetOwnTimestampUnit(1/8000.0);
    mRtpSession.Create(sessionParams, &params, jrtplib::RTPTransmitter::ExternalProto);
    mRtpDtmfSession.Create(sessionParams, &params, jrtplib::RTPTransmitter::ExternalProto);

    // Attach srtp session to sender
    mRtpSender.setSrtpSession(&mSrtpSession);
    //mRtpDump = new RtpDump("d:\\outgoing.rtp");
    //mRtpSender.setDumpWriter(mRtpDump);

#if defined(DUMP_SENDING_AUDIO)
    mSendingDump = std::make_shared<WavFileWriter>();
    mSendingDump->open("sending_audio.wav", 8000, 1);
#endif
}

AudioStream::~AudioStream()
{
    ICELogInfo(<< "Delete AudioStream instance");
    if (mSendingDump)
    {
        mSendingDump->close();
        mSendingDump.reset();
    }

    // Delete used rtp streams
    for (AudioStreamMap::iterator streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
        delete streamIter->second;
    mStreamMap.clear();

    if (mRtpDtmfSession.IsActive())
        mRtpDtmfSession.Destroy();
    if (mRtpSession.IsActive())
        mRtpSession.Destroy();

#if defined(USE_RTPDUMP)
    if (mRtpDump)
    {
        mRtpDump->flush();
        delete mRtpDump;
    }
#endif

    mCaptureResampler8.stop();
    mCaptureResampler16.stop();
    mCaptureResampler32.stop();
    mCaptureResampler48.stop();
    ICELogInfo(<< "Encoded " << mEncodedTime << " milliseconds of audio");

    if (mDumpStreams.mStreamForRecordingIncoming)
        mDumpStreams.mStreamForRecordingIncoming->close();
    if (mDumpStreams.mStreamForReadingOutgoing)
        mDumpStreams.mStreamForReadingOutgoing->close();

    if (mFinalStatistics)
        *mFinalStatistics = mStat;

    ICELogInfo(<< mStat.toString());
}

void AudioStream::setDestination(const RtpPair<InternetAddress>& dest)
{
    Lock l(mMutex);
    Stream::setDestination(dest);
    mRtpSender.setDestination(dest);
}

void AudioStream::setTransmittingCodec(Codec::Factory& factory, int payloadType)  
{
    ICELogInfo(<< "Selected codec " << factory.name() << "/" << factory.samplerate() << " for transmitting");

    Lock l(mMutex);
    mTransmittingCodec = factory.create();
    mTransmittingPayloadType = payloadType;
    if (mRtpSession.IsActive())
        mRtpSession.SetTimestampUnit(1.0 / mTransmittingCodec->samplerate());
}

PCodec AudioStream::transmittingCodec()
{
    Lock l(mMutex);
    return mTransmittingCodec;
}

void AudioStream::addData(const void* buffer, int bytes)
{
    assert(bytes == AUDIO_MIC_BUFFER_SIZE);

    // Read predefined audio if configured
    if (mDumpStreams.mStreamForReadingOutgoing)
    {
        if (mDumpStreams.mStreamForReadingOutgoing->isOpened())
            mDumpStreams.mStreamForReadingOutgoing->read(const_cast<void*>(buffer), bytes);
    }

    // Read mirrored audio if needed
    if (mMirror && mMirrorPrebuffered)
        mMirrorBuffer.read(const_cast<void*>(buffer), bytes);

    if (mMediaObserver)
        mMediaObserver->onMedia(buffer, bytes, MT::Stream::MediaDirection::Outgoing, this, mMediaObserverTag);

    Codec* codec = nullptr;
    {
        Lock l(mMutex);
        codec = mTransmittingCodec.get();
        if (nullptr == codec) {
            // ICELogDebug(<< "No transmitting codec selected.");
            return;
        }
    }

    // Resample
    unsigned dstlen = unsigned(float(codec->samplerate() / float(AUDIO_SAMPLERATE)) * bytes);
    Audio::Resampler* r = nullptr;
    switch (codec->samplerate())
    {
    case 8000:   r = &mCaptureResampler8; break;
    case 16000:  r = &mCaptureResampler16; break;
    case 32000:  r = &mCaptureResampler32; break;
    case 48000:  r = &mCaptureResampler48; break;
    default:
        assert(0);
    }

    size_t processedInput = 0;
    dstlen = r->processBuffer(buffer, bytes, processedInput, mResampleBuffer, dstlen);
    // ProcessedInput output value is ignored - because sample rate of input is always 8/16/32/48K - so all buffer is processed

    // See if we need stereo <-> mono conversions
    unsigned stereolen = 0;
    if (codec->channels() != AUDIO_CHANNELS)
    {
        if (codec->channels() == 2)
            stereolen = Audio::ChannelConverter::monoToStereo(mResampleBuffer, dstlen, mStereoBuffer, dstlen * 2);
        else
            dstlen = Audio::ChannelConverter::stereoToMono(mResampleBuffer, dstlen, mResampleBuffer, dstlen / 2);
    }

    // See if inband dtmf audio should be sent instead
    ByteBuffer dtmf;
    if (mDtmfContext.type() == DtmfContext::Dtmf_Inband && mDtmfContext.getInband(AUDIO_MIC_BUFFER_LENGTH, codec->samplerate(), dtmf))
        mCapturedAudio.add(dtmf.data(), dtmf.size());
    else
        mCapturedAudio.add(stereolen ? mStereoBuffer : mResampleBuffer, stereolen ? stereolen : dstlen);

    // See if it is time to send RFC2833 tone
    ByteBuffer rfc2833, stopPacket;
    if (mDtmfContext.type() == DtmfContext::Dtmf_Rfc2833 && mDtmfContext.getRfc2833(AUDIO_MIC_BUFFER_LENGTH, rfc2833, stopPacket))
    {
        if (rfc2833.size())
            mRtpDtmfSession.SendPacket(rfc2833.data(), rfc2833.size(), mRemoteTelephoneCodec, true, AUDIO_MIC_BUFFER_LENGTH * 8);

        if (stopPacket.size())
        {
            for (int i=0; i<3; i++)
                mRtpDtmfSession.SendPacket(stopPacket.data(), stopPacket.size(), mRemoteTelephoneCodec, true, AUDIO_MIC_BUFFER_LENGTH * 8);
        }
    }

    int processed = 0;
    int encodedTime = 0;
    int packetTime = mPacketTime ? mPacketTime : codec->frameTime();

    // Make stereo version if required
    for (int i=0; i<mCapturedAudio.filled() / codec->pcmLength(); i++)
    {
        if (mSendingDump)
            mSendingDump->write((const char*)mCapturedAudio.data() + codec->pcmLength() * i, codec->pcmLength());

        int produced;
        produced = codec->encode((const char*)mCapturedAudio.data() + codec->pcmLength()*i,
                                 codec->pcmLength(), mFrameBuffer, MT_MAXAUDIOFRAME);
        
        // Counter of processed input bytes of raw pcm data from microphone
        processed += codec->pcmLength();
        encodedTime += codec->frameTime();
        mEncodedTime += codec->frameTime();

        if (produced)
        {
            mEncodedAudio.appendBuffer(mFrameBuffer, produced);
            if (packetTime <= encodedTime)
            {
                // Time to send packet
                ICELogMedia(<< "Sending RTP packet pt = " << mTransmittingPayloadType << ", plength = " << (int)mEncodedAudio.size());
                mRtpSession.SendPacketEx(mEncodedAudio.data(), mEncodedAudio.size(), mTransmittingPayloadType, false,
                                         packetTime * codec->samplerate()/1000, 0, NULL, 0);
                mEncodedAudio.clear();
                encodedTime = 0;
            }
        }
    }
    if (processed > 0)
        mCapturedAudio.erase(processed);
}

void AudioStream::copyDataTo(Audio::Mixer& mixer, int needed)
{
    // Local audio mixer - used to send audio to media observer
    Audio::Mixer localMixer;
    Audio::DataWindow forObserver;

    // Iterate
    for (auto& streamIter: mStreamMap)
    {
        Audio::DataWindow w;
        w.setCapacity(32768);

        SingleAudioStream* sas = streamIter.second;
        if (sas)
        {
            sas->copyPcmTo(w, needed);

            // Provide mirroring if needed
            if (mMirror)
            {
                mMirrorBuffer.add(w.data(), w.filled());
                if (!mMirrorPrebuffered)
                    mMirrorPrebuffered = mMirrorBuffer.filled() >= MT_MIRROR_PREBUFFER;
            }

            if (!(state() & (int)StreamState::Receiving))
                w.zero(needed);

            // Check if we do not need input from this stream
            if (w.filled())
            {
                if (mDumpStreams.mStreamForRecordingIncoming)
                {
                    if (mDumpStreams.mStreamForRecordingIncoming->isOpened())
                        mDumpStreams.mStreamForRecordingIncoming->write(w.data(), w.filled());
                }
                mixer.addPcm(this, streamIter.first, w, AUDIO_SAMPLERATE, false);

                if (mMediaObserver)
                    localMixer.addPcm(this, streamIter.first, w, AUDIO_SAMPLERATE, false);
            }
        }
    }

    if (mMediaObserver)
    {
        localMixer.mixAndGetPcm(forObserver);
        mMediaObserver->onMedia(forObserver.data(), forObserver.capacity(), MT::Stream::MediaDirection::Incoming, this, mMediaObserverTag);
    }
}

void AudioStream::dataArrived(PDatagramSocket s, const void* buffer, int length, InternetAddress& source)
{
    jrtplib::RTPIPv6Address addr6;
    jrtplib::RTPIPv4Address addr4;
    jrtplib::RTPExternalTransmissionInfo* info = dynamic_cast<jrtplib::RTPExternalTransmissionInfo*>(mRtpSession.GetTransmissionInfo());
    assert(info);

    // Drop RTP packets if stream is not receiving now; let RTCP go
    if (!(state() & (int)StreamState::Receiving) && RtpHelper::isRtpOrRtcp(buffer, length))
    {
        ICELogMedia(<< "Stream is not allowed to receive RTP stream. Ignore the RT(C)P packet");
        return;
    }

    // Copy incoming data to temp buffer to perform possible srtp unprotect
    int receiveLength = length;
    memcpy(mReceiveBuffer, buffer, length);

    if (mSrtpSession.active())
    {
        bool srtpResult;
        size_t srcLength = length; size_t dstLength = sizeof mSrtpDecodeBuffer;
        if (RtpHelper::isRtp(mReceiveBuffer, receiveLength))
            srtpResult = mSrtpSession.unprotectRtp(mReceiveBuffer, srcLength, mSrtpDecodeBuffer, &dstLength);
        else
            srtpResult = mSrtpSession.unprotectRtcp(mReceiveBuffer, srcLength, mSrtpDecodeBuffer, &dstLength);
        if (!srtpResult)
        {
            ICELogError(<<"Cannot decrypt SRTP packet.");
            return;
        }

        memcpy(mReceiveBuffer, mSrtpDecodeBuffer, dstLength);
        receiveLength = dstLength;
    }

    switch (source.family())
    {
    case AF_INET:
        addr4.SetIP(source.sockaddr4()->sin_addr.s_addr);
        addr4.SetPort(source.port());
        ICELogMedia(<< "Injecting RTP/RTCP packet into jrtplib");
        info->GetPacketInjector()->InjectRTPorRTCP(mReceiveBuffer, receiveLength, addr4);
        break;

    case AF_INET6:
        addr6.SetIP(source.sockaddr6()->sin6_addr);
        addr6.SetPort(source.port());
        ICELogMedia(<< "Injecting RTP/RTCP packet into jrtplib");
        info->GetPacketInjector()->InjectRTPorRTCP(mReceiveBuffer, receiveLength, addr6);
        break;

    default:
        assert(0);
    }

    mStat.mReceived += length;
    if (RtpHelper::isRtp(mReceiveBuffer, receiveLength))
    {
        if (!mStat.mFirstRtpTime.is_initialized())
            mStat.mFirstRtpTime = std::chrono::system_clock::now();
        mStat.mReceivedRtp++;
    }
    else
        mStat.mReceivedRtcp++;

    mRtpSession.Poll(); // maybe it is extra with external transmitter
    bool hasData = mRtpSession.GotoFirstSourceWithData();
    while (hasData)
    {
        std::shared_ptr<jrtplib::RTPPacket> packet(mRtpSession.GetNextPacket());
        if (packet)
        {
            ICELogMedia(<< "jrtplib returned packet");

            // Find right handler for rtp stream
            SingleAudioStream* rtpStream = nullptr;
            AudioStreamMap::iterator streamIter = mStreamMap.find(packet->GetSSRC());
            if (streamIter == mStreamMap.end())
                mStreamMap[packet->GetSSRC()] = rtpStream = new SingleAudioStream(mCodecSettings, mStat);
            else
                rtpStream = streamIter->second;

            // Process incoming data packet
            rtpStream->process(packet);

            double rtt = mRtpSession.GetCurrentSourceInfo()->INF_GetRoundtripTime().GetDouble();
            if (rtt > 0)
                mStat.mRttDelay.process(rtt);
        }
        hasData = mRtpSession.GotoNextSourceWithData();
    }
}

void AudioStream::setState(unsigned state)
{
    Stream::setState(state);
}

void AudioStream::setTelephoneCodec(int payloadType)
{
    mRemoteTelephoneCodec = payloadType;
}

void AudioStream::setSocket(const RtpPair<PDatagramSocket>& socket)
{
    Stream::setSocket(socket);
    mRtpSender.setSocket(socket);
}

DtmfContext& AudioStream::queueOfDtmf()
{
    return mDtmfContext;
}

void AudioStream::readFile(const Audio::PWavFileReader& stream, MediaDirection direction)
{
    switch (direction)
    {
    case MediaDirection::Outgoing: mDumpStreams.mStreamForReadingOutgoing = stream; break;
    case MediaDirection::Incoming: mDumpStreams.mStreamForReadingIncoming = stream; break;
    }
}

void AudioStream::writeFile(const Audio::PWavFileWriter& writer, MediaDirection direction)
{
    switch (direction)
    {
    case MediaDirection::Outgoing: mDumpStreams.mStreamForRecordingOutgoing = writer; break;
    case MediaDirection::Incoming: mDumpStreams.mStreamForRecordingIncoming = writer; break;
    }
}

void AudioStream::setupMirror(bool enable)
{
    if (!mMirror && enable)
    {
        mMirrorBuffer.setCapacity(MT_MIRROR_CAPACITY);
        mMirrorPrebuffered = false;
    }
    mMirror = enable;
}

void AudioStream::setFinalStatisticsOutput(Statistics* stats)
{
    mFinalStatistics = stats;
}
