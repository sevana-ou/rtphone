/*====================================================================================
    EVS Codec 3GPP TS26.443 Nov 13, 2018. Version 12.11.0 / 13.7.0 / 14.3.0 / 15.1.0
  ====================================================================================*/

#include "options.h"
#include "prot.h"

namespace evs { 



/*--------------------------------------------------------------------------
 *  get_delay()
 *
 *  Function returns various types of delays in the codec in ms.
 *--------------------------------------------------------------------------*/

float get_delay(                /* o  : delay value in ms                         */
    const short what_delay,     /* i  : what delay? (ENC or DEC)                  */
    const int   io_fs           /* i  : input/output sampling frequency           */
)
{
    float delay = 0;

    if( what_delay == ENC )
    {
        delay = (DELAY_FIR_RESAMPL_NS + ACELP_LOOK_NS);
    }
    else
    {
        if( io_fs == 8000 )
        {
            delay = DELAY_CLDFB_NS;
        }
        else
        {
            delay = DELAY_BWE_TOTAL_NS;
        }
    }

    return delay;
}

} // end of namespace
