/*******************************************************************
 *
 * typedef statements of types used in all half-rate GSM routines
 *
 ******************************************************************/

#ifndef __GSM_HR_TYPEDEFS
#define __GSM_HR_TYPEDEFS

#define DATE    "August 8, 1996    "
#define VERSION "Version 4.2       "

#define LW_SIGN (long)0x80000000       /* sign bit */
#define LW_MIN (long)0x80000000
#define LW_MAX (long)0x7fffffff

#define SW_SIGN (short)0x8000          /* sign bit for int16_t type */
#define SW_MIN (short)0x8000           /* smallest Ram */
#define SW_MAX (short)0x7fff           /* largest Ram */

#include <stdint.h>

/* Definition of Types *
 ***********************/

            /* 32 bit "accumulator" (L_*) */
            /* 16 bit "register"  (sw*) */
typedef short int int16_tRom;        /* 16 bit ROM data    (sr*) */
typedef long int int32_tRom;          /* 32 bit ROM data    (L_r*)  */

struct NormSw
{                                      /* normalized int16_t fractional
                                        * number snr.man precedes snr.sh (the
                                        * shift count)i */
  int16_t man;                       /* "mantissa" stored in 16 bit
                                        * location */
  int16_t sh;                        /* the shift count, stored in 16 bit
                                        * location */
};

/* Global constants *
 ********************/

#define NP 10                          /* order of the lpc filter */
#define N_SUB 4                        /* number of subframes */
#define F_LEN 160                      /* number of samples in a frame */
#define S_LEN 40                       /* number of samples in a subframe */
#define A_LEN 170                      /* LPC analysis length */
#define OS_FCTR 6                      /* maximum LTP lag oversampling
                                        * factor */

#define OVERHANG 8                     /* vad parameter */
#define strStr strStr16

/* global variables */
/********************/

extern int giFrmCnt;                   /* 0,1,2,3,4..... */
extern int giSfrmCnt;                  /* 0,1,2,3 */

extern int giDTXon;                    /* DTX Mode on/off */

#endif
