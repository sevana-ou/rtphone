/*====================================================================================
    EVS Codec 3GPP TS26.443 Nov 13, 2018. Version 12.11.0 / 13.7.0 / 14.3.0 / 15.1.0
  ====================================================================================*/

#include "options.h"
#include "cnst.h"
#include "prot.h"

namespace evs { 


/*-------------------------------------------------------------------*
 * enr_1_Az()
 *
 * Find Energy of the 1/A(z) impulse response
 *-------------------------------------------------------------------*/

float enr_1_Az(        /* o  : impulse response energy */
    const float Aq[],  /* i  : LP filter coefs         */
    const short len    /* i  : impulse response length */
)
{
    float enr_LP, h1[2*L_SUBFR], mem[M];


    set_f( h1, 0, len );                          /* Find the impulse response        */
    set_f( mem, 0, M );
    h1[0] = 1.0f;
    syn_filt( Aq, M, h1, h1, len, mem, 0 );
    enr_LP = dotp( h1, h1, len ) + 0.01f;         /* Find the impulse response energy */

    return enr_LP;
}

} // end of namespace
