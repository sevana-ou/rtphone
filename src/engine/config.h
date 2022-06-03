/* Copyright(C) 2007-2020 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __TOOLKIT_CONFIG_H
#define __TOOLKIT_CONFIG_H

#define USE_SPEEX_AEC

// TODO: test implementation with webrtc aec; be careful - it needs fixes!
//#define USE_WEBRTC_AEC
#define USER


#define AUDIO_SAMPLE_WIDTH 16
#define AUDIO_CHANNELS 1

// Samplerate must be 8 / 16 / 24 / 32 / 48 KHz
#define AUDIO_SAMPLERATE 8000
#define AUDIO_MIC_BUFFER_COUNT 16
#define AUDIO_MIC_BUFFER_LENGTH 10
#define AUDIO_MIC_BUFFER_SIZE  (AUDIO_MIC_BUFFER_LENGTH * AUDIO_SAMPLERATE / 1000 * 2 * AUDIO_CHANNELS)
#define AUDIO_SPK_BUFFER_COUNT 16
#define AUDIO_SPK_BUFFER_LENGTH 10
#define AUDIO_SPK_BUFFER_SIZE (AUDIO_SPK_BUFFER_LENGTH * AUDIO_SAMPLERATE / 1000 * 2 * AUDIO_CHANNELS)
#define AUDIO_MIX_CHANNEL_COUNT 16
#define AUDIO_DEVICEPAIR_INPUTBUFFER 16384

// Avoid too high resampler quality - it can take many CPU and cause gaps in playing
#define AUDIO_RESAMPLER_QUALITY 1
#define AEC_FRAME_TIME 10
#define AEC_TAIL_TIME 160


// Defined these two lines to get dumping of audio input/output
//#define AUDIO_DUMPINPUT
//#define AUDIO_DUMPOUTPUT


#define UA_REGISTRATION_TIME 3600
#define UA_MEDIA_PORT_START  20000
#define UA_MEDIA_PORT_FINISH 30000
#define UA_MAX_UDP_PACKET_SIZE 576
#define UA_PUBLICATION_ID "314"

#define MT_SAMPLERATE AUDIO_SAMPLERATE

#define MT_MAXAUDIOFRAME 1440
#define MT_MAXRTPPACKET 1500
#define MT_DTMF_END_PACKETS 3

#define RTP_BUFFER_HIGH 24480
#define RTP_BUFFER_LOW 10
#define RTP_BUFFER_PREBUFFER 80
#define RTP_DECODED_CAPACITY 2048

#define DEFAULT_SUBSCRIPTION_TIME 1200
#define DEFAULT_SUBSCRIPTION_REFRESHTIME 500

#define PRESENCE_IN_REG_HEADER "PresenceInReg"

// Maximum UDP packet length
#define MAX_UDPPACKET_SIZE 65535
#define MAX_VALID_UDPPACKET_SIZE 2048

// AMR codec defines - it requires USE_AMR_CODEC defined
// #define USE_AMR_CODEC
#define MT_AMRNB_PAYLOADTYPE 112
#define MT_AMRNB_CODECNAME "amr"

#define MT_AMRNB_OCTET_PAYLOADTYPE 113

#define MT_AMRWB_PAYLOADTYPE 96
#define MT_AMRWB_CODECNAME "amr-wb"

#define MT_AMRWB_OCTET_PAYLOADTYPE 97

#define MT_GSMEFR_PAYLOADTYPE 126
#define MT_GSMEFR_CODECNAME "GERAN-EFR"

#define MT_EVS_PAYLOADTYPE 127
#define MT_EVS_CODECNAME "EVS"

// OPUS codec defines
// #define USE_OPUS_CODEC
#define MT_OPUS_CODEC_PT 106

// ILBC codec defines
#define MT_ILBC20_PAYLOADTYPE -1
#define MT_ILBC30_PAYLOADTYPE -1

// ISAC codec defines
#define MT_ISAC16K_PAYLOADTYPE -1
#define MT_ISAC32K_PAYLOADTYPE -1

// GSM HR payload type
#define MT_GSMHR_PAYLOADTYPE -1

// Mirror buffer capacity
#define MT_MIRROR_CAPACITY 32768

// Mirror buffer readiness threshold - 50 milliseconds
#define MT_MIRROR_PREBUFFER (MT_SAMPLERATE / 10)

#if defined(TARGET_OSX) || defined(TARGET_LINUX)
# define TEXT(X) X
#endif

// In milliseconds
#define MT_SEVANA_FRAME_TIME 680

#endif
