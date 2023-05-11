#include "rutil/AtomicCounter.hxx"
#include "rutil/Lock.hxx"

using namespace resip;
AtomicCounter::AtomicCounter(int initialValue)
  :mCounterValue(initialValue)
{

}


AtomicCounter::~AtomicCounter()
{

}

int AtomicCounter::value()
{
  return mCounterValue;
}

int AtomicCounter::increment()
{
  Lock l(mMutex);
  return ++mCounterValue;
}

int AtomicCounter::decrement()
{
  Lock l(mMutex);
  return --mCounterValue;
}
