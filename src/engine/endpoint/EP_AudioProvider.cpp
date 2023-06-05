/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EP_AudioProvider.h"
#include "EP_Engine.h"
#include "../media/MT_Box.h"
#include "../media/MT_AudioStream.h"
#include "../media/MT_SrtpHelper.h"
#include "../media/MT_Stream.h"
#include "../helper/HL_Rtp.h"
#include "../helper/HL_StreamState.h"
#include "../helper/HL_Log.h"
#include "../helper/HL_String.h"

#define LOG_SUBSYSTEM "AudioProvider"

AudioProvider::AudioProvider(UserAgent& agent, MT::Terminal& terminal)
    :mUserAgent(agent), mTerminal(terminal), mState(0),
      mRemoteTelephoneCodec(0), mRemoteNoSdp(false)
{
    mActive = mfActive;
    mRemoteState = msSendRecv;
    mActiveStream = mTerminal.createStream(MT::Stream::Audio, mUserAgent.config());
    if (mUserAgent.config().exists(CONFIG_CODEC_PRIORITY))
        mCodecPriority.setupFrom(mUserAgent.config()[CONFIG_CODEC_PRIORITY].asVMap());
    mSrtpSuite = SRTP_NONE;
    setStateImpl((int)StreamState::SipRecv | (int)StreamState::SipSend | (int)StreamState::Receiving | (int)StreamState::Sending);
}

AudioProvider::~AudioProvider()
{
}

std::string AudioProvider::streamName()
{
    return "audio";
}

std::string AudioProvider::streamProfile()
{
    if (mState & (int)StreamState::Srtp)
        return "RTP/SAVP";
    else
        return "RTP/AVP";
}

// Sets destination IP address
void  AudioProvider::setDestinationAddress(const RtpPair<InternetAddress>& addr)
{
    if (!mActiveStream)
        return;

    mActiveStream->setDestination(addr);
}

void AudioProvider::configureMediaObserver(MT::Stream::MediaObserver *observer, void* userTag)
{
    mMediaObserver = observer;
    mMediaObserverTag = userTag;
    if (mActiveStream)
        mActiveStream->configureMediaObserver(observer, userTag);
}

// Processes incoming data
void  AudioProvider::processData(const PDatagramSocket& s, const void* dataBuffer, int dataSize, InternetAddress& source)
{
    if (!mActiveStream)
        return;

    if (RtpHelper::isRtpOrRtcp(dataBuffer, dataSize))
    {
        ICELogMedia(<<"Adding new data to stream processing");
        mActiveStream->dataArrived(s, dataBuffer, dataSize, source);
    }
}

// This method is called by user agent to send ICE packet from mediasocket
void AudioProvider::sendData(const PDatagramSocket& s, InternetAddress& destination, const void* buffer, unsigned int size)
{
    s->sendDatagram(destination, buffer, size);
}

// Create SDP offer
void AudioProvider::updateSdpOffer(resip::SdpContents::Session::Medium& sdp, SdpDirection direction)
{
    if (mRemoteNoSdp)
        return;

    if (mState & (int)StreamState::Srtp)
    {
        // Check if SRTP suite is found already or not
        if (mSrtpSuite == SRTP_NONE)
        {
            for (int suite = SRTP_AES_128_AUTH_80; suite <= SRTP_LAST; suite++)
                sdp.addAttribute("crypto", resip::Data(createCryptoAttribute((SrtpSuite)suite)));
        }
        else
            sdp.addAttribute("crypto", resip::Data(createCryptoAttribute(mSrtpSuite)));
    }
#if defined(USE_RESIP_INTEGRATION)

    // Use CodecListPriority mCodecPriority adapter to work with codec priorities
    if (mAvailableCodecs.empty())
    {
        for (int i=0; i<mCodecPriority.count(mTerminal.codeclist()); i++)
            mCodecPriority.codecAt(mTerminal.codeclist(), i).updateSdp(sdp.codecs(), direction);
        sdp.addCodec(resip::SdpContents::Session::Codec::TelephoneEvent);
    }
    else
    {
        mAvailableCodecs.front().mFactory->updateSdp(sdp.codecs(), direction);
        if (mRemoteTelephoneCodec)
            sdp.addCodec(resip::SdpContents::Session::Codec::TelephoneEvent);
    }
#endif

    // Publish stream state
    const char* attr = nullptr;
    switch (mActive)
    {
    case mfActive:
        switch(mRemoteState)
        {
        case msSendonly: attr = "recvonly"; break;
        case msInactive: attr = "recvonly"; break;
        case msRecvonly:
        case msSendRecv: break; // Do nothing here
        }
        break;

    case mfPaused:
        switch (mRemoteState)
        {
        case msRecvonly: attr = "sendonly"; break;
        case msSendonly: attr = "inactive"; break;
        case msInactive: attr = "inactive"; break;
        case msSendRecv: attr = "sendonly"; break;
        }
        break;
    }
    if (attr)
        sdp.addAttribute(attr);
}

void AudioProvider::sessionDeleted()
{
    sessionTerminated();
}

void AudioProvider::sessionTerminated()
{
    ICELogDebug(<< "sessionTerminated() for audio provider");
    setState(state() & ~((int)StreamState::Sending | (int)StreamState::Receiving));

    if (mActiveStream)
    {
        ICELogDebug(<< "Copy statistics from existing stream before freeing.");

        // Copy statistics - maybe it will be requested later
        mBackupStats = mActiveStream->statistics();

        ICELogDebug(<< "Remove stream from terminal");
        mTerminal.freeStream(mActiveStream);

        // Retrieve final statistics
        MT::AudioStream* audio_stream = dynamic_cast<MT::AudioStream*>(mActiveStream.get());
        if (audio_stream)
            audio_stream->setFinalStatisticsOutput(&mBackupStats);

        ICELogDebug(<< "Reset reference to stream.");
        mActiveStream.reset();
    }
}

void AudioProvider::sessionEstablished(int conntype)
{
    // Start media streams
    setState(state() | (int)StreamState::Receiving | (int)StreamState::Sending);

    // Available codec list can be empty in case of no-sdp offers.
    if (conntype == EV_SIP && !mAvailableCodecs.empty() && mActiveStream)
    {
        RemoteCodec& rc = mAvailableCodecs.front();
        mActiveStream->setTransmittingCodec(*rc.mFactory, rc.mRemotePayloadType);
        auto codec = dynamic_cast<MT::AudioStream*>(mActiveStream.get())->transmittingCodec();
        dynamic_cast<MT::AudioStream*>(mActiveStream.get())->setTelephoneCodec(mRemoteTelephoneCodec);
    }
}

void AudioProvider::setSocket(const RtpPair<PDatagramSocket>& p4, const RtpPair<PDatagramSocket>& p6)
{
    mSocket4 = p4;
    mSocket6 = p6;
    mActiveStream->setSocket(p4);
}

RtpPair<PDatagramSocket>& AudioProvider::socket(int family)
{
    switch (family)
    {
    case AF_INET:
        return mSocket4;

    case AF_INET6:
        return mSocket6;
    }
    return mSocket4;
}


bool AudioProvider::processSdpOffer(const resip::SdpContents::Session::Medium& media, SdpDirection sdpDirection)
{
    // Check if there is compatible codec
    mAvailableCodecs.clear();
    mRemoteTelephoneCodec = 0;

    // Check if there is SDP at all
    mRemoteNoSdp = media.codecs().empty();
    if (mRemoteNoSdp)
        return true;

    // Update RFC2833 related information
    findRfc2833(media.codecs());

    // Use CodecListPriority mCodecPriority to work with codec priorities
    int pt;
    for (int localIndex=0; localIndex<mCodecPriority.count(mTerminal.codeclist()); localIndex++)
    {
        MT::Codec::Factory& factory = mCodecPriority.codecAt(mTerminal.codeclist(), localIndex);
#if defined(USE_RESIP_INTEGRATION)
        if ((pt = factory.processSdp(media.codecs(), sdpDirection)) != -1)
            mAvailableCodecs.push_back(RemoteCodec(&factory, pt));
#endif
    }

    if (!mAvailableCodecs.size())
        return false;

    // Iterate SRTP crypto: attributes
    if (media.exists("crypto"))
    {
        // Find the most strong crypt suite
        const std::list<resip::Data>& vl = media.getValues("crypto");
        SrtpSuite ss = SRTP_NONE;
        ByteBuffer key;
        for (std::list<resip::Data>::const_iterator attrIter = vl.begin(); attrIter != vl.end(); attrIter++)
        {
            const resip::Data& attr = *attrIter;
            ByteBuffer tempkey;
            SrtpSuite suite = processCryptoAttribute(attr, tempkey);
            if (suite > ss)
            {
                ss = suite;
                mSrtpSuite = suite;
                key = tempkey;
            }
        }

        // If SRTP suite is agreed
        if (ss != SRTP_NONE)
        {
            ICELogInfo(<< "Found SRTP suite " << ss);
            mActiveStream->srtp().open(key, ss);
            setState(state() | (int)StreamState::Srtp);
        }
        else
            ICELogInfo(<< "Did not find valid SRTP suite");
    }

    DataProvider::processSdpOffer(media, sdpDirection);

    return true;
}


void AudioProvider::setState(unsigned state)
{
    setStateImpl(state);
}

unsigned AudioProvider::state()
{
    return mState;
}

MT::Statistics AudioProvider::getStatistics()
{
    if (mActiveStream)
        return mActiveStream->statistics();
    else
        return mBackupStats;
}

MT::PStream AudioProvider::activeStream()
{
    return mActiveStream;
}

std::string AudioProvider::createCryptoAttribute(SrtpSuite suite)
{
    if (!mActiveStream)
        return "";

    // Use tag 1 - it is ok, as we use only single crypto attribute
    int   srtpTag = 1;

    // Print key to base64 string
    PByteBuffer keyBuffer = mActiveStream->srtp().outgoingKey(suite).first;
    resip::Data d(keyBuffer->data(), keyBuffer->size());
    resip::Data keyText = d.base64encode();

    // Create "crypto" attribute value
    char buffer[512];
    const char* suiteName = NULL;
    switch (suite)
    {
    case SRTP_AES_128_AUTH_80: suiteName = SRTP_SUITE_NAME_1; break;
    case SRTP_AES_256_AUTH_80: suiteName = SRTP_SUITE_NAME_2; break;
    default: assert(0);
    }
    sprintf(buffer, "%d %s inline:%s", srtpTag, suiteName, keyText.c_str());

    return buffer;
}

SrtpSuite AudioProvider::processCryptoAttribute(const resip::Data& value, ByteBuffer& key)
{
    int srtpTag = 0;
    char suite[64], keyChunk[256];
    int components = sscanf(value.c_str(), "%d %63s inline: %255s", &srtpTag, suite, keyChunk);
    if (components != 3)
        return SRTP_NONE;

    const char* delimiter = strchr(keyChunk, '|');
    resip::Data keyText;
    if (delimiter)
        keyText = resip::Data(keyChunk, delimiter - keyChunk);
    else
        keyText = resip::Data(keyChunk);

    resip::Data rawkey = keyText.base64decode();
    key = ByteBuffer(rawkey.c_str(), rawkey.size());

    // Open srtp
    SrtpSuite result = SRTP_NONE;
    if (strcmp(suite, SRTP_SUITE_NAME_1) == 0)
        result = SRTP_AES_128_AUTH_80;
    else
        if (strcmp(suite, SRTP_SUITE_NAME_2) == 0)
            result = SRTP_AES_256_AUTH_80;

    return result;
}

void AudioProvider::findRfc2833(const resip::SdpContents::Session::Medium::CodecContainer& codecs)
{
    resip::SdpContents::Session::Medium::CodecContainer::const_iterator codecIter;
    for (codecIter = codecs.begin(); codecIter != codecs.end(); codecIter++)
    {
        if (strcmp("TELEPHONE-EVENT", codecIter->getName().c_str()) == 0 ||
                strcmp("telephone-event", codecIter->getName().c_str()) == 0)
            mRemoteTelephoneCodec = codecIter->payloadType();
    }
}

void AudioProvider::readFile(const Audio::PWavFileReader& stream, MT::Stream::MediaDirection direction)
{
    // Iterate stream list
    if (mActiveStream)
        mActiveStream->readFile(stream, direction);
}

void AudioProvider::writeFile(const Audio::PWavFileWriter& stream, MT::Stream::MediaDirection direction)
{
    if (mActiveStream)
        mActiveStream->writeFile(stream, direction);
}

void AudioProvider::setupMirror(bool enable)
{
    if (mActiveStream)
        mActiveStream->setupMirror(enable);
}

void AudioProvider::setStateImpl(unsigned int state) {
    mState = state;
    if (mActiveStream)
        mActiveStream->setState(state);

}
