/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __HL_STREAM_STATE
#define __HL_STREAM_STATE

// How to use stream state flags.

enum class StreamState
{
  Sending = 1,          // Transmitting RTP. Set this flag to allow outgoing media stream.
  Receiving = 2,        // Receiving RTP. Set this flag to allow receiving media stream.
  Playing = 4,          // Play to audio. Unmutes the audio from specified stream.
  Grabbing = 8,         // Capture audio. Unmutes the audio to specified stream.
  Srtp = 16,            // Use SRTP. Make attempt
  SipSend = 32,         // Declare send capability in SDP
  SipRecv = 64          // Declare recv capability in SDP
};



#endif
