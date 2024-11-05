/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EP_DataProvider.h"
#include "../helper/HL_StreamState.h"

bool DataProvider::isSupported(const char* name)
{
    return !strcmp(name, "audio");

    //return (!strcmp(name, "screen") || !strcmp(name, "data") || !strcmp(name, "audio") || !strcmp(name, "video"));
}

void DataProvider::pause()
{
    /*if (state() & STATE_SIPRECV)
    setState( state() & ~STATE_SIPRECV );*/

    // Stop receive RTP stream
    if (state() & (int)StreamState::Receiving)
        setState( state() & ~(int)StreamState::Receiving );

    mActive = mfPaused;
}

void DataProvider::resume()
{
    // Tell remote peer about resumed receiving in SDP
    //setState( state() | STATE_SIPRECV );

    // Start receive RTP stream
    setState( state() | (int)StreamState::Receiving );

    mActive = mfActive;
}

bool DataProvider::processSdpOffer(const resip::SdpContents::Session::Medium& media, SdpDirection sdpDirection)
{
    // Process paused and inactive calls
    if (media.exists("sendonly"))
    {
        mRemoteState = msSendonly;
        setState(state() & ~(int)StreamState::Sending);
    }
    else
        if (media.exists("recvonly"))
        {
            mRemoteState = msRecvonly;
            setState(state() & ~(int)StreamState::Receiving);
        }
        else
            if (media.exists("inactive"))
            {
                mRemoteState = msInactive;
                setState(state() & ~((int)StreamState::Sending | (int)StreamState::Receiving) );
            }
            else
            {
                mRemoteState = msSendRecv;
                switch (mActive)
                {
                case mfActive:
                    setState(state() | (int)StreamState::Sending | (int)StreamState::Receiving);
                    break;

                case mfPaused:
                    setState(state() | (int)StreamState::Sending );
                    break;
                }
            }
    return true;
}
