#ifndef __HELPER_STATISTICS_H
#define __HELPER_STATISTICS_H

template<typename T>
struct Average
{
    int mCount = 0;
    T mSum = 0;
    T average() const
    {
        if (!mCount)
            return 0;
        return mSum / mCount;
    }

    T value() const
    {
        return average();
    }

    void process(T value)
    {
        mCount++;
        mSum += value;
    }
};

template<typename T, int minimum = 100000, int maximum = 0, int default_value = 0>
struct TestResult
{
    T mMin = minimum;
    T mMax = maximum;
    Average<T> mAverage;
    T mCurrent = default_value;

    void process(T value)
    {
        if (mMin > value)
            mMin = value;
        if (mMax < value)
            mMax = value;
        mCurrent = value;
        mAverage.process(value);
    }

    bool is_initialized() const
    {
        return mAverage.mCount > 0;
    }

    T current() const
    {
        if (is_initialized())
            return mCurrent;
        else
            return 0;
    }

    T value() const
    {
        return current();
    }

    T average() const
    {
        return mAverage.average();
    }

    TestResult<T>& operator = (T value)
    {
        process(value);
        return *this;
    }

    operator T()
    {
        return mCurrent;
    }
};


#endif