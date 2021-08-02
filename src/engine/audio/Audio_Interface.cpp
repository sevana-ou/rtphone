/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Audio_Interface.h"
#include "../helper/HL_OsVersion.h"

#if !defined(USE_NULL_AUDIO)
# ifdef TARGET_WIN
#   include "Audio_Wmme.h"
#   include "Audio_DirectSound.h"
# endif
# ifdef TARGET_OSX
#   include "Audio_CoreAudio.h"
# endif
# ifdef TARGET_ANDROID
#   include "Audio_Android.h"
# endif
#endif

#include "Audio_Helper.h"
#include "Audio_Null.h"

using namespace Audio;

Device::Device()
    :mConnection(nullptr)
{
}

Device::~Device()
{
}


void Device::setConnection(DataConnection* connection)
{
    mConnection = connection;
}

DataConnection* Device::connection()
{
    return mConnection;
}

InputDevice::InputDevice()
{
}

InputDevice::~InputDevice()
{
}

InputDevice* InputDevice::make(int devId)
{
#if defined(USE_NULL_AUDIO)
    return new NullInputDevice();
#else
#if defined(TARGET_WIN) && defined(_MSC_VER)
    // return new WmmeInputDevice(index);
    return new DSoundInputDevice(DSoundHelper::deviceId2Guid(devId, true));
#endif
#ifdef TARGET_OSX
    return new MacInputDevice(devId);
#endif
#ifdef TARGET_ANDROID
    return new AndroidInputDevice(devId);
#endif
#endif
    return nullptr;
}

OutputDevice::OutputDevice()
{
}

OutputDevice::~OutputDevice()
{
}

OutputDevice* OutputDevice::make(int devId)
{
#if defined(USE_NULL_AUDIO)
    return new NullOutputDevice();
#else
#if defined(TARGET_WIN)
    //return new WmmeOutputDevice(index);
    return new DSoundOutputDevice(DSoundHelper::deviceId2Guid(devId, false));
#endif
#ifdef TARGET_OSX
    return new MacOutputDevice(devId);
#endif
#ifdef TARGET_ANDROID
    return new AndroidOutputDevice(devId);
#endif
#endif
    return nullptr;
}


// --- Enumerator ---
Enumerator::Enumerator()
{
}

Enumerator::~Enumerator()
{
}

int Enumerator::nameToIndex(const std::tstring& name)
{
    for (int i = 0; i < count(); i++)
        if (nameAt(i) == name)
            return i;
    return -1;
}


Enumerator* Enumerator::make(bool useNull)
{

    if (useNull)
        return new NullEnumerator();
#ifndef USE_NULL_AUDIO

#ifdef TARGET_WIN
    if (winVersion() > Win_Xp)
        return new VistaEnumerator();
    else
        return new XpEnumerator();
#endif
#ifdef TARGET_OSX
    return new MacEnumerator();
#endif
#endif
    return new NullEnumerator();
}

// ----- OsEngine ------------

OsEngine* OsEngine::instance()
{
#ifdef USE_NULL_AUDIO
    return nullptr;
#endif

#ifdef TARGET_ANDROID
    return nullptr; // As we use Oboe library for now
    //return &OpenSLEngine::instance();
#endif

    return nullptr;
}
