/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_PLAYER_H
#define __AUDIO_PLAYER_H

#include "../helper/HL_Log.h"
#include "../helper/HL_Sync.h"
#include "Audio_Interface.h"
#include <deque>
#include <map>
#include <vector>

namespace Audio
{
  class Player: public DataConnection
  {
  friend class DevicePair;
  public:
    struct PlaylistItem
    {
      PWavFileReader mFile;
      bool mLoop;
      int mTimelength;
      int mUsageId;
    };
    typedef std::deque<PlaylistItem> Playlist;

    class EndOfAudioDelegate
    {
    public:
      virtual void onFilePlayed(PlaylistItem& item) = 0;
    };

  protected:
    typedef std::map<int, int> UsageMap;
    Audio::POutputDevice mOutput;
    UsageMap mUsage;  // References map
    std::vector<int> mFinishedUsages; // Finished plays

    Mutex mGuard;
    Playlist mPlaylist;
    float mPlayedTime;
    EndOfAudioDelegate* mDelegate;

    void onMicData(const Format& f, const void* buffer, int length);
    void onSpkData(const Format& f, void* buffer, int length);
    void onFilePlayed();
    void scheduleRelease();
    void obtain(int usageId);
  public:
    Player();
    ~Player();
    void setDelegate(EndOfAudioDelegate* d);
    EndOfAudioDelegate* getDelegate() const;
    void setOutput(POutputDevice output);
    POutputDevice getOutput() const;
    void add(int usageId, PWavFileReader file, bool loop, int timelength);
    void release(int usageId);
    void clear();
    int releasePlayed();
    void retrieveUsageIds(std::vector<int>& ids);
  };
}
#endif
