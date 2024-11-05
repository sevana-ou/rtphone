/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __CLIENT_AUDIO_H
#define __CLIENT_AUDIO_H

#include "../engine/audio/Audio_Interface.h"
#include "../engine/audio/Audio_Player.h"
#include "../engine/media/MT_Box.h"



enum
{
    AudioPrefix_Ring = 1,
    AudioPrefix_Zero,
    AudioPrefix_One,
    AudioPrefix_Two,
    AudioPrefix_Three,
    AudioPrefix_Four,
    AudioPrefix_Five,
    AudioPrefix_Six,
    AudioPrefix_Seven,
    AudioPrefix_Eight,
    AudioPrefix_Nine,
    AudioPrefix_Asterisk,
    AudioPrefix_Diez
};

#define AudioSessionCoeff 64

class AudioManager: public Audio::Player::EndOfAudioDelegate
{
public:
    AudioManager();
    virtual ~AudioManager();

    static AudioManager& instance();

    // Enforces to close audio devices. Used to shutdown AudioManager on exit from application
    void close();

    // Terminal and settings must be available for AudioManager
    void setTerminal(MT::Terminal* terminal);
    MT::Terminal* terminal();

    void setAudioMonitoring(Audio::DataConnection* monitoring);
    Audio::DataConnection* audioMonitoring();

    // Start/stop methods relies on usage counter; only first start and last stop opens/closes devices actually
    void start(int usageId);
    void stop(int usageId);

    enum AudioTarget
    {
        atNull,
        atReceiver,
        atRinger
    };

    enum LoopMode
    {
        lmLoopAudio,
        lmNoloop
    };

    void startPlayFile(int usageId, const std::string& path, AudioTarget target, LoopMode lm, int timelimit = 0);
    void stopPlayFile(int usageId);


    void onFilePlayed(Audio::Player::PlaylistItem& item);

    // Must be called from main loop to release used audio devices
    void process();

protected:
    Audio::PInputDevice mAudioInput;
    Audio::POutputDevice mAudioOutput;
    Audio::Player mPlayer;
    MT::Terminal* mTerminal;
    Audio::DataConnection* mAudioMonitoring;

    std::map<int, int> UsageMap;
    UsageCounter mUsage;
    std::mutex mGuard;
};
#endif
