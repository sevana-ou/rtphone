/***************************************************************************
 *
 *   Purpose:   Define the structure of the Global Constants
 *
 **************************************************************************/

#ifndef ___ROM
#define ___ROM

#if defined(__cplusplus)
extern "C" {
#endif

#include "gsmhr_typedefs.h"
//#include "gsmhr_mathhalf.h"
//#include "gsmhr_mathdp31.h"

/*_________________________________________________________________________
 |                                                                         |
 |                                Data Types                               |
 |_________________________________________________________________________|
*/

/* gsp0 vector quantizer */
/*-----------------------*/
#define GSP0_NUM_OF_TABLES 4
#define GSP0_NUM 32
#define GSP0_VECTOR_SIZE 5

extern int16_tRom pppsrGsp0
       [GSP0_NUM_OF_TABLES][GSP0_NUM][GSP0_VECTOR_SIZE];


/* unvoiced code vectors */
/*-----------------------*/
#define UVCODEVEC_NUM_OF_CODE_BOOKS 2
#define UVCODEVEC_NUM_OF_CODE_BITS 7

extern int16_tRom pppsrUvCodeVec
       [UVCODEVEC_NUM_OF_CODE_BOOKS][UVCODEVEC_NUM_OF_CODE_BITS][S_LEN];


/* voiced code vectors */
/*---------------------*/
#define VCDCODEVEC_NUM_OF_CODE_BOOKS 1
#define VCDCODEVEC_NUM_OF_CODE_BITS 9

extern int16_tRom pppsrVcdCodeVec
       [VCDCODEVEC_NUM_OF_CODE_BOOKS][VCDCODEVEC_NUM_OF_CODE_BITS][S_LEN];


/* vector quantizer tables */
/*-------------------------*/
#define QUANT_NUM_OF_TABLES 3

#define QUANT1_NUM_OF_BITS 11
#define QUANT1_NUM_OF_ROWS (1 << QUANT1_NUM_OF_BITS)
#define QUANT1_NUM_OF_STAGES 3
#define QUANT1_NUM_OF_WORDS (QUANT1_NUM_OF_ROWS*QUANT1_NUM_OF_STAGES/2)

#define QUANT2_NUM_OF_BITS 9
#define QUANT2_NUM_OF_ROWS (1 << QUANT2_NUM_OF_BITS)
#define QUANT2_NUM_OF_STAGES 3
#define QUANT2_NUM_OF_WORDS (QUANT2_NUM_OF_ROWS*QUANT2_NUM_OF_STAGES/2)

#define QUANT3_NUM_OF_BITS 8
#define QUANT3_NUM_OF_ROWS (1 << QUANT3_NUM_OF_BITS)
#define QUANT3_NUM_OF_STAGES 4
#define QUANT3_NUM_OF_WORDS (QUANT3_NUM_OF_ROWS*QUANT3_NUM_OF_STAGES/2)

extern int16_tRom psrQuant1[QUANT1_NUM_OF_WORDS];

extern int16_tRom psrQuant2[QUANT2_NUM_OF_WORDS];

extern int16_tRom psrQuant3[QUANT3_NUM_OF_WORDS];


/* lpc pre-quantizer */
/*-------------------*/
#define PREQ1_NUM_OF_BITS 6
#define PREQ1_NUM_OF_ROWS (1 << PREQ1_NUM_OF_BITS)
#define PREQ1_NUM_OF_STAGES 3
#define PREQ1_NUM_OF_WORDS (PREQ1_NUM_OF_ROWS*PREQ1_NUM_OF_STAGES/2)

#define PREQ2_NUM_OF_BITS 5
#define PREQ2_NUM_OF_ROWS (1 << PREQ2_NUM_OF_BITS)
#define PREQ2_NUM_OF_STAGES 3
#define PREQ2_NUM_OF_WORDS (PREQ2_NUM_OF_ROWS*PREQ2_NUM_OF_STAGES/2)

#define PREQ3_NUM_OF_BITS 4
#define PREQ3_NUM_OF_ROWS (1 << PREQ3_NUM_OF_BITS)
#define PREQ3_NUM_OF_STAGES 4
#define PREQ3_NUM_OF_WORDS (PREQ3_NUM_OF_ROWS*PREQ3_NUM_OF_STAGES/2)

extern int16_tRom psrPreQ1[PREQ1_NUM_OF_WORDS];

extern int16_tRom psrPreQ2[PREQ2_NUM_OF_WORDS];

extern int16_tRom psrPreQ3[PREQ3_NUM_OF_WORDS];


/* size of the vq subset in the kth segment */
/*------------------------------------------*/
extern int16_tRom psrQuantSz[QUANT_NUM_OF_TABLES];


/* pre-quantizer size */
/*--------------------*/
extern int16_tRom psrPreQSz[QUANT_NUM_OF_TABLES];


/* reflection coeff scalar quantizer */
/*-----------------------------------*/
#define SQUANT_NUM_OF_BITS 8
#define SQUANT_NUM_OF_ROWS (1 << SQUANT_NUM_OF_BITS)

extern int16_tRom psrSQuant[SQUANT_NUM_OF_ROWS];


/* index structure for LPC Vector Quantizer */
/*------------------------------------------*/
struct IsubLHn
{                                      /* index structure for LPC Vector
                                        * Quantizer */
  int16_tRom l;                      /* lowest index index range
                                        * from 1..NP */
  int16_tRom h;                      /* highest index */
  int16_tRom len;                    /* h-l+1 */
};


extern struct IsubLHn psvqIndex[QUANT_NUM_OF_TABLES];


/* square root of p0 table */
/*-------------------------*/
#define SQRTP0_NUM_OF_BITS 5
#define SQRTP0_NUM_OF_ROWS (1 << SQRTP0_NUM_OF_BITS)
#define SQRTP0_NUM_OF_MODES 3

extern int16_tRom ppsrSqrtP0[SQRTP0_NUM_OF_MODES][SQRTP0_NUM_OF_ROWS];


/* interpolation filter used for C and G */
/*---------------------------------------*/
#define CGINTFILT_MACS 6

extern int16_tRom ppsrCGIntFilt[CGINTFILT_MACS][OS_FCTR];


/* interpolation filter used pitch */
/*---------------------------------*/
#define PVECINTFILT_MACS 10

extern int16_tRom ppsrPVecIntFilt[PVECINTFILT_MACS][OS_FCTR];


/* fractional pitch lag table lag*OS_FCTR */
/*----------------------------------------*/
#define LAGTBL_NUM_OF_ROWS 256

extern int16_tRom psrLagTbl[LAGTBL_NUM_OF_ROWS];


/* R0 decision value table defines range (not the levels themselves */
/*------------------------------------------------------------------*/

#define R0DECTBL_NUM_OF_R0BITS 5
#define R0DECTBL_NUM_OF_ROWS ((1 << R0DECTBL_NUM_OF_R0BITS)*2 - 1)

extern int16_tRom psrR0DecTbl[R0DECTBL_NUM_OF_ROWS];


/* high pass filter coefficients */
/*-------------------------------*/
#define HPFCOEFS_NUM_OF_CODES 10

extern int16_tRom psrHPFCoefs[HPFCOEFS_NUM_OF_CODES];


/* spectral smoothing coefficients */
/*---------------------------------*/
#define NWCOEFS_NUM_OF_CODES 20

extern int16_tRom psrNWCoefs[NWCOEFS_NUM_OF_CODES];


/* spectral smoothing coefficients for FLAT */
/*------------------------------------------*/
#define FLATSSTCOEFS_NUM_OF_CODES 10

extern int32_tRom pL_rFlatSstCoefs[FLATSSTCOEFS_NUM_OF_CODES];

extern int16_tRom psrOldCont[4];
extern int16_tRom psrNewCont[4];

#if defined(__cplusplus)
}
#endif


#endif
