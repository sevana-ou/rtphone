/*====================================================================================
    EVS Codec 3GPP TS26.443 Nov 13, 2018. Version 12.11.0 / 13.7.0 / 14.3.0 / 15.1.0
  ====================================================================================*/

#include <math.h>
#include <stdio.h>
#include "cnst.h"
#include "prot.h"
#include "stat_com.h"
#include "assert.h"
#include "basop_util.h"

#ifndef int32
#define int32   int
#endif



/*---------------------------------------------------------------
  Ari 14 bits common routines
  -------------------------------------------------------------*/

/**
 * \brief 	Integer Multiply
 *
 * \param[i] r
 * \param[i] c
 *
 * \return r*c
 */
long mul_sbc_14bits(long r,long c)
{
    long temp;
    /*function in line*/
    temp = (((int32) r)*((int32) c))>>stat_bitsnew;
    return temp;

    /*function in line*/
}
