#ifndef ATOMICINCREMENT_HXX
#define ATOMICINCREMENT_HXX

#include "rutil/Mutex.hxx"

namespace resip
{

class AtomicCounter
{
public:
  AtomicCounter(int initialValue = 0);
  ~AtomicCounter();

  int value();
  int increment();
  int decrement();

protected:
  Mutex mMutex;
  int mCounterValue;
};

}

#endif // ATOMICINCREMENT_HXX
