/*====================================================================================
    EVS Codec 3GPP TS26.443 Nov 13, 2018. Version 12.11.0 / 13.7.0 / 14.3.0 / 15.1.0
  ====================================================================================*/

#include "options.h"
#include "prot.h"

namespace evs { 


/*----------------------------------------------------------------------------------*
 * procedure updt_tar:
 *
 * Update the target vector for codebook search.
 *----------------------------------------------------------------------------------*/
void updt_tar(
    const float *x,    /* i  : old target (for pitch search)     */
    float *x2,   /* o  : new target (for codebook search)  */
    const float *y,    /* i  : filtered adaptive codebook vector */
    const float gain,  /* i  : adaptive codebook gain            */
    const short L      /* i  : subframe size                     */
)
{
    short i;


    for (i = 0; i < L; i++)
    {
        x2[i] = x[i] - gain*y[i];
    }
}

} // end of namespace
