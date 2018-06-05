#ifndef __AUDIO_NULL_H
#define __AUDIO_NULL_H

#include <thread>
#include "Audio_Interface.h"

namespace Audio
{
  class NullTimer
  {
  public:
    class Delegate
    {
    public:
      virtual void onTimerSignal(NullTimer& timer) = 0;
    };

  protected:
    std::thread mWorkerThread;
    volatile bool mShutdown;
    Delegate* mDelegate;
    int mInterval,  // Interval - wanted number of milliseconds
        mTail; // Number of milliseconds that can be sent immediately to sink
    std::string mThreadName;

    void start();
    void stop();
    void run();
  public:
    /* Interval is in milliseconds. */
    NullTimer(int interval, Delegate* delegate, const char* name = nullptr);
    ~NullTimer();
  };

  class NullInputDevice: public InputDevice, public NullTimer::Delegate
  {
  protected:
    void* mBuffer = nullptr;
    std::shared_ptr<NullTimer> mTimer;
    int64_t mTimeCounter = 0, mDataCounter = 0;
  public:
    NullInputDevice();
    virtual ~NullInputDevice();

    bool open() override;
    void close() override;
    Format getFormat() override;

    void onTimerSignal(NullTimer& timer) override;
  };

  class NullOutputDevice: public OutputDevice, public NullTimer::Delegate
  {
  protected:
    std::shared_ptr<NullTimer> mTimer;
    void* mBuffer = nullptr;
    int64_t mDataCounter = 0, mTimeCounter = 0;
  public:
    NullOutputDevice();
    virtual ~NullOutputDevice();

    bool open() override;
    void close() override;
    Format getFormat() override;

    void onTimerSignal(NullTimer& timer) override;
  };

  class NullEnumerator: public Enumerator
  {
  public:
    NullEnumerator();
    ~NullEnumerator();

    void open(int direction) override;
    void close() override;

    int count() override;
    std::tstring nameAt(int index) override;
    int idAt(int index) override;
    int indexOfDefaultDevice() override;

  };
}

#endif
