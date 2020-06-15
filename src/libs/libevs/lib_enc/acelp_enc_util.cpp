/*====================================================================================
    EVS Codec 3GPP TS26.443 Nov 13, 2018. Version 12.11.0 / 13.7.0 / 14.3.0 / 15.1.0
  ====================================================================================*/

#include <math.h>
#include <memory.h>
#include <assert.h>
#include "options.h"
#include "typedef.h"
#include "prot.h"
#include "cnst.h"
#include "rom_com.h"

namespace evs {

/*-------------------------------------------------------------------*
* E_ACELP_toeplitz_mul()
*
* Multiplication of Toeplitz matrix with vector c, such that d = toeplitz(R)*c
*-------------------------------------------------------------------*/

void E_ACELP_toeplitz_mul(
    float R[],
    float c[],
    float d[]
)
{
    short k, j;
    float s;

    for (k=0; k<L_SUBFR; k++)
    {
        s = R[k]*c[0];

        for (j=1; j<k; j++)
        {
            s += R[k-j]*c[j];
        }

        for (; j<L_SUBFR; j++)
        {
            s += R[j-k]*c[j];
        }
        d[k] = s;
    }

    return;
}

} // end of namespace evs
