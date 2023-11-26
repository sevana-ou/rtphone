#include "../engine_config.h"

#include "MT_CngHelper.h"
#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

#define NOISE_AMPL 8031

#ifndef TARGET_WIN
typedef short __int16;
typedef int __int32;
#endif

namespace MT
{
  static int GenerateRandom(int maxvalue)
  {
    assert(maxvalue <= RAND_MAX);

    int result = rand();

    result = static_cast<int>(result / ((float)RAND_MAX / maxvalue) + 0.5);
    if (result > maxvalue)
      result = maxvalue;

    return result;
  }

  class LPFilter
  {
  public:
    static const int NCoef = 1;
    static const int DCgain = 128;

    __int16 ACoef[NCoef+1];
    __int16 BCoef[NCoef+1];

    __int32 y[NCoef+1] = {0, 0};
    __int16 x[NCoef+1] = {0, 0};

    LPFilter()
    {
      ACoef[0] = 16707; ACoef[1] = 16707;
      BCoef[0] = 32767; BCoef[1] = -32511;
    }

    ~LPFilter()
    {

    }

    __int16 Do(__int16 sample)
    {
      int n;

      //shift the old samples
      for(n=NCoef; n>0; n--) {
         x[n] = x[n-1];
         y[n] = y[n-1];
      }

      //Calculate the new output
      x[0] = sample;
      y[0] = ACoef[0] * x[0];
      for(n=1; n<=NCoef; n++)
          y[0] += ACoef[n] * x[n] - BCoef[n] * y[n];

      y[0] /= BCoef[0];

      return y[0] / DCgain;
    }
  };

  class HPFilter
  {
  public:
    static const int NCoef = 1;
    static const int DCgain = 128;

    __int16 ACoef[NCoef+1];
    __int16 BCoef[NCoef+1];

    __int32 y[NCoef+1] = {0, 0};
    __int16 x[NCoef+1] = {0, 0};

    HPFilter()
    {
      ACoef[0] = 16384; ACoef[1] = -16384;
      BCoef[0] = 32767; BCoef[1] = 0;
    }

    ~HPFilter()
    {

    }

    __int16 Do(__int16 sample)
    {
      int n;

      //shift the old samples
      for(n=NCoef; n>0; n--) {
         x[n] = x[n-1];
         y[n] = y[n-1];
      }

      //Calculate the new output
      x[0] = sample;
      y[0] = ACoef[0] * x[0];
      for(n=1; n<=NCoef; n++)
          y[0] += ACoef[n] * x[n] - BCoef[n] * y[n];

      y[0] /= BCoef[0];

      return y[0] / DCgain;
    }
  };


  // --------------------- CngHelper ----------------------------
  int CngHelper::parse3389(const void* buf, int size, int rate, int requestLengthInMilliseconds, short* output)
  {
    assert(buf);
    assert(output);

    if (!size || !requestLengthInMilliseconds)
      return 0;

    int outputSize = requestLengthInMilliseconds * (rate / 1000);

    // Cast pointer to input data
    const unsigned char* dataIn = (const unsigned char*)buf;

    // Get noise level
    unsigned char noiseLevel = *dataIn;
    float linear = float(1.0 / noiseLevel ? noiseLevel : 1);

    // Generate white noise for 16KHz sample rate
    LPFilter lpf; HPFilter hpf;
    for (int sampleIndex = 0; sampleIndex < outputSize; sampleIndex++)
    {
      output[sampleIndex] = GenerateRandom(NOISE_AMPL*2) - NOISE_AMPL;
      output[sampleIndex] = lpf.Do(output[sampleIndex]);
      output[sampleIndex] = hpf.Do(output[sampleIndex]);
      output[sampleIndex] = (short)((float)output[sampleIndex] * linear);
    }

    return outputSize * 2;
  }


  // ------------------- CngDecoder --------------------

  CngDecoder::CngDecoder()
  {
    WebRtcCng_CreateDec(&mContext);
    if (mContext)
      WebRtcCng_InitDec(mContext);
  }

  CngDecoder::~CngDecoder()
  {
    if (mContext)
      WebRtcCng_FreeDec(mContext);
  }

  void CngDecoder::decode3389(const void *buf, int size)
  {
    if (mContext)
      WebRtcCng_UpdateSid(mContext, (WebRtc_UWord8*)buf, size);
  }

  int CngDecoder::produce(int rate, int requestedMilliseconds, short *output, bool newPeriod)
  {
    WebRtcCng_Generate(mContext, output, requestedMilliseconds * rate / 1000, newPeriod ? 1 : 0);
    return requestedMilliseconds * rate / 1000 * 2;
  }

}
