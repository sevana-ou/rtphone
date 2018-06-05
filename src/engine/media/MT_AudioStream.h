/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_AUDIOSTREAM_H
#define __MT_AUDIOSTREAM_H

#include "../config.h"
#include "MT_Stream.h"
#include "MT_NativeRtpSender.h"
#include "MT_SingleAudioStream.h"
#include "MT_Dtmf.h"
#include "../helper/HL_VariantMap.h"
#include "../helper/HL_ByteBuffer.h"
#include "../helper/HL_NetworkSocket.h"
#include "../helper/HL_Rtp.h"
#include "../audio/Audio_DataWindow.h"
#include "../audio/Audio_Mixer.h"
#include "../audio/Audio_Resampler.h"
#include "ice/ICESync.h"
#include "jrtplib/src/rtpsession.h"
#include "jrtplib/src/rtpexternaltransmitter.h"
#include "audio/Audio_WavFile.h"

namespace MT
{
  
  class AudioStream: public Stream
  {
  public:
    AudioStream(const CodecList::Settings& codecSettings);
    ~AudioStream();

    void setDestination(const RtpPair<InternetAddress>& dest) override;
    //void setPacketTime(int packetTime);
    
    void setTransmittingCodec(Codec::Factory& factory, int payloadType) override;
    PCodec transmittingCodec();

    // Called to queue data captured from microphone. 
    // Buffer holds 16bits PCM data with AUDIO_SAMPLERATE rate and AUDIO_CHANNELS channels.
    void addData(const void* buffer, int length);
    
    // Called to get data to speaker (or mixer)
    void copyDataTo(Audio::Mixer& mixer, int needed);

    // Called to process incoming rtp packet
    void dataArrived(PDatagramSocket s, const void* buffer, int length, InternetAddress& source) override;
    void setSocket(const RtpPair<PDatagramSocket>& socket) override;
    void setState(unsigned state) override;
    
    void setTelephoneCodec(int payloadType);
    DtmfContext& queueOfDtmf();    

    void readFile(const Audio::PWavFileReader& stream, MediaDirection direction) override;
    void writeFile(const Audio::PWavFileWriter& writer, MediaDirection direction) override;
    void setupMirror(bool enable) override;

    void setFinalStatisticsOutput(Statistics* stats);

  protected:
    Audio::DataWindow mCapturedAudio;     // Data from microphone
    Audio::DataWindow mStereoCapturedAudio;
    char mIncomingPcmBuffer[AUDIO_MIC_BUFFER_SIZE]; // Temporary buffer to allow reading from file
    char mResampleBuffer[AUDIO_MIC_BUFFER_SIZE*8];  // Temporary buffer to hold data
    char mStereoBuffer[AUDIO_MIC_BUFFER_SIZE*8];    // Temporary buffer to hold data converted to stereo
    PCodec mTransmittingCodec;                      // Current encoding codec
    int mTransmittingPayloadType;                   // Payload type to mark outgoing packets
    int mPacketTime;                                // Required packet time
    char mFrameBuffer[MT_MAXAUDIOFRAME];            // Temporary buffer to hold results of encoder
    ByteBuffer mEncodedAudio;                       // Encoded frame(s)
    int mEncodedTime;                               // Time length of encoded audio
    const CodecList::Settings& mCodecSettings;      // Configuration for stream
    Mutex mMutex;                      	            // Mutex
    int mRemoteTelephoneCodec;                      // Payload for remote telephone codec
    jrtplib::RTPSession mRtpSession;                // Rtp session
    jrtplib::RTPSession mRtpDtmfSession;            // Rtp dtmf session
    NativeRtpSender mRtpSender;
    AudioStreamMap mStreamMap;                      // Map of media streams. Key is RTP's SSRC value.
    Audio::DataWindow mOutputBuffer;
    RtpDump* mRtpDump;
    Audio::Resampler  mCaptureResampler8,
                      mCaptureResampler16,
                      mCaptureResampler32,
                      mCaptureResampler48;
    DtmfContext mDtmfContext;  
    char mReceiveBuffer[MAX_VALID_UDPPACKET_SIZE];

    struct
    {
      Audio::PWavFileWriter  mStreamForRecordingIncoming,
                      mStreamForRecordingOutgoing;
      Audio::PWavFileReader  mStreamForReadingIncoming,
                      mStreamForReadingOutgoing;
    } mDumpStreams;

    Audio::PWavFileWriter mSendingDump;

    bool mMirror = false;
    bool mMirrorPrebuffered = false;
    Audio::DataWindow mMirrorBuffer;

    Statistics* mFinalStatistics = nullptr;

    bool decryptSrtp(void* data, int* len);
  };
};

#endif
