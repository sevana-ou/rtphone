#ifndef __MT_CNG_HELPER_H
#define __MT_CNG_HELPER_H

#include "webrtc/cng/webrtc_cng.h"

namespace MT
{
  class CngHelper
  {
  public:
    /* Parses RTP 3389 payload and produces CNG to audio buffer. Returns size of produced audio in bytes. */
    static int parse3389(const void* buf, int size, int rate, int requestLengthInMilliseconds, short* output);
  };

  class CngDecoder
  {
  protected:
    CNG_dec_inst* mContext;

  public:
    CngDecoder();
    ~CngDecoder();

    void decode3389(const void* buf, int size);
    int produce(int rate, int requestedMilliseconds, short* output, bool newPeriod);
  };
}

#endif
