/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MT_DTMF
#define MT_DTMF

#include "../config.h"
#include <vector>
#include <string>
#include "../helper/HL_ByteBuffer.h"
#include "../helper/HL_Sync.h"

namespace MT
{

  class DtmfBuilder
  {
  public:
    // Output should be 4 bytes length
    static void buildRfc2833(int tone, int duration, int volume, bool endOfEvent, void* output);
    
    // Buf receives PCM audio
    static void buildInband(int tone, int startTime, int finishTime, int rate, short* buf);
  };

  struct Dtmf
  {
    Dtmf(): mTone(0), mDuration(0), mVolume(0), mFinishCount(3), mCurrentTime(0), mStopped(false) {}
    Dtmf(int tone, int volume, int duration): mTone(tone), mVolume(volume), mDuration(duration), mFinishCount(3), mCurrentTime(0), mStopped(false) {}
        
    int mStopped;
    int mTone;
    int mDuration; // It is zero for tones generated by startTone()..stopTone() calls.
    int mVolume;
    int mFinishCount;
    int mCurrentTime;
  };

  typedef std::vector<Dtmf> DtmfQueue;

  class DtmfContext
  {
  public:
    enum Type
    {
      Dtmf_Inband,
      Dtmf_Rfc2833
    };

    DtmfContext();
    ~DtmfContext();

    void setType(Type t);
    Type type();

    void startTone(int tone, int volume);
    void stopTone();

    void queueTone(int tone, int volume, int duration);
    void clearAllTones();

    // Returns true if result was sent to output. Output will be resized automatically.
    bool getInband(int milliseconds, int rate, ByteBuffer& output);
    bool getRfc2833(int milliseconds, ByteBuffer& output, ByteBuffer& stopPacket);

  protected:
    Mutex mGuard;
    Type mType;
    DtmfQueue mQueue;
  };

class DTMFDetector 
{
public:
  /*! The default constructor. Allocates space for detector context. */
  DTMFDetector();
  
  /*! The destructor. Free the detector context's memory. */
  ~DTMFDetector();

  /*! This method receives the input PCM 16-bit data and returns found DTMF event(s) in string representation.
   *  @param samples Input PCM buffer pointer.
   *  @param size Size of input buffer in bytes
   *  @return Found DTMF event(s) in string representation. The returned value has variable length.
   */
  std::string streamPut(unsigned char* samples, unsigned int size);

  void resetState();

protected:
  void* mState; /// DTMF detector context
};
}

#endif
