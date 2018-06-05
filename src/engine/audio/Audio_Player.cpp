/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Audio_Player.h"

using namespace Audio;
// -------------- Player -----------
Player::Player()
:mDelegate(NULL), mPlayedTime(0)
{
}

Player::~Player()
{
}

void Player::setDelegate(EndOfAudioDelegate* d)
{
  mDelegate = d;
}

Player::EndOfAudioDelegate* Player::getDelegate() const
{
  return mDelegate;
}

void Player::setOutput(POutputDevice output)
{
  mOutput = output;
  if (mOutput)
    mOutput->setConnection(this);
}

POutputDevice Player::getOutput() const
{
  return mOutput;
}
void Player::onMicData(const Format& f, const void* buffer, int length)
{
  // Do nothing here - this data sink is not used in player
}

#define BYTES_PER_MILLISECOND (AUDIO_SAMPLERATE / 1000 * 2 * AUDIO_CHANNELS)

void Player::onSpkData(const Format& f, void* buffer, int length)
{
  Lock l(mGuard);
  
  // Fill buffer by zero if player owns dedicated device
  if (mOutput)
    memset(buffer, 0, length);

  // See if there is item in playlist
  int produced = 0;
  while (mPlaylist.size() && produced < length)
  {
    PlaylistItem& item = mPlaylist.front();
    // Check for timelength
    if (item.mTimelength > 0 && item.mTimelength < mPlayedTime)
    {
      onFilePlayed();
      continue;
    }

    int wasread = item.mFile->read((char*)buffer+produced, length-produced);
    mPlayedTime += float(wasread) / BYTES_PER_MILLISECOND;
    produced += wasread;
    if (wasread < length-produced)
    {
      if (item.mLoop)
      {
        item.mFile->rewind();
        wasread = item.mFile->read((char*)buffer+produced, (length - produced));
        mPlayedTime += float(wasread) / BYTES_PER_MILLISECOND;
        produced += wasread;
      }
      else
        onFilePlayed();
    }
  }
}

void Player::onFilePlayed()
{
  // Save usage id to release later from main loop
  mFinishedUsages.push_back(mPlaylist.front().mUsageId);
  
  // Send event
  if (mDelegate)
    mDelegate->onFilePlayed(mPlaylist.front());
  
  // Remove played item & reset played time
  mPlaylist.pop_front();
  mPlayedTime = 0;
}

void Player::obtain(int usage)
{
  Lock l(mGuard);
  UsageMap::iterator usageIter = mUsage.find(usage);
  if (usageIter == mUsage.end())
    mUsage[usage] = 1;
  else
    usageIter->second = usageIter->second + 1;

  if (mUsage.size() == 1 && mOutput)
    mOutput->open();
}

void Player::release(int usage)
{
  Lock l(mGuard);
  UsageMap::iterator usageIter = mUsage.find(usage);
  if (usageIter == mUsage.end())
    return;

  usageIter->second = usageIter->second - 1;
  if (!usageIter->second)
    mUsage.erase(usageIter);

  for (unsigned i=0; i<mPlaylist.size(); i++)
    if (mPlaylist[i].mUsageId == usage)
      mPlaylist.erase(mPlaylist.begin() + i);

  if (mUsage.empty() && mOutput)
    mOutput->close();
}

int Player::releasePlayed()
{
  Lock l(mGuard);
  int result = mFinishedUsages.size();
  while (mFinishedUsages.size())
  {
    release(mFinishedUsages.front());
    mFinishedUsages.erase(mFinishedUsages.begin());
  }
  return result;
}

void Player::add(int usageId, PWavFileReader file, bool loop, int timelength)
{
  Lock l(mGuard);
  PlaylistItem item;
  item.mFile = file;
  item.mLoop = loop;
  item.mTimelength = timelength;
  item.mUsageId = usageId;
  mPlaylist.push_back(item);

  obtain(usageId);
}

void Player::clear()
{
  Lock l(mGuard);
  while (mPlaylist.size())
    onFilePlayed();
}

void Player::retrieveUsageIds(std::vector<int>& ids)
{
  ids.assign(mFinishedUsages.begin(), mFinishedUsages.end());
  mFinishedUsages.clear();
}
