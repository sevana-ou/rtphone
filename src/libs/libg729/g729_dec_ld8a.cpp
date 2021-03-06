/*
   ITU-T G.729A Speech Coder    ANSI-C Source Code
   Version 1.1    Last modified: September 1996

   Copyright (c) 1996,
   AT&T, France Telecom, NTT, Universite de Sherbrooke
   All rights reserved.
*/

/*-----------------------------------------------------------------*
 *   Functions Init_Decod_ld8a  and Decod_ld8a                     *
 *-----------------------------------------------------------------*/

#include "g729_typedef.h"
#include "g729_basic_op.h"
#include "g729_ld8a.h"
#include "g729_tab_ld8a.h"
#include "g729_lspdec.h"
#include "g729_util.h"
#include "g729_dec_gain.h"

/*---------------------------------------------------------------*
 *   Decoder constant parameters (defined in "ld8a.h")           *
 *---------------------------------------------------------------*
 *   L_FRAME     : Frame size.                                   *
 *   L_SUBFR     : Sub-frame size.                               *
 *   M           : LPC order.                                    *
 *   MP1         : LPC order+1                                   *
 *   PIT_MIN     : Minimum pitch lag.                            *
 *   PIT_MAX     : Maximum pitch lag.                            *
 *   L_INTERPOL  : Length of filter for interpolation            *
 *   PRM_SIZE    : Size of vector containing analysis parameters *
 *---------------------------------------------------------------*/


/*-----------------------------------------------------------------*
 *   Function Init_Decod_ld8a                                      *
 *            ~~~~~~~~~~~~~~~                                      *
 *                                                                 *
 *   ->Initialization of variables for the decoder section.        *
 *                                                                 *
 *-----------------------------------------------------------------*/

DecState *
Init_Decod_ld8a (void)
{
  DecState *decoder;
  
  decoder = (DecState*) malloc (sizeof (DecState));
  
  /* Initialize pointers */

  decoder->exc = decoder->old_exc + PIT_MAX + L_INTERPOL;

  /* Static vectors to zero */

  Set_zero (decoder->old_exc, PIT_MAX + L_INTERPOL);
  Set_zero (decoder->mem_syn, M10);

  Copy (lsp_old_init, decoder->lsp_old, M10);

  decoder->sharp = SHARPMIN;
  decoder->old_T0 = 60;
  decoder->gain_code = 0;
  decoder->gain_pitch = 0;

  Lsp_decw_reset (decoder);
  
  decoder->bad_lsf = 0;  
  Set_zero (decoder->Az_dec, MP1 * 2);
  Set_zero (decoder->synth_buf, L_FRAME + M10);
  Set_zero (decoder->T2, 2);

  Copy (past_qua_en_init, decoder->past_qua_en, 4);
  
  decoder->seed = 21845;
  
  return decoder;
}

/*-----------------------------------------------------------------*
 *   Function Decod_ld8a                                           *
 *           ~~~~~~~~~~                                            *
 *   ->Main decoder routine.                                       *
 *                                                                 *
 *-----------------------------------------------------------------*/

void
Decod_ld8a (DecState *decoder,
	    Word16 parm[],	/* (i)   : vector of synthesis parameters
				   parm[0] = bad frame indicator (bfi)  */
	    Word16 synth[],	/* (o)   : synthesis speech                     */
	    Word16 A_t[],	/* (o)   : decoded LP filter in 2 subframes     */
	    Word16 * T2,	/* (o)   : decoded pitch lag in 2 subframes     */
	    Word16 * bad_lsf)
{
  Flag Overflow;
  Word16 *Az;			/* Pointer on A_t   */
  Word16 lsp_new[M10];		/* LSPs             */
  Word16 code[L_SUBFR];		/* ACELP codevector */

  /* Scalars */

  Word16 i, j, i_subfr;
  Word16 T0, T0_frac, index;
  Word16 bfi;
  Word32 L_temp;

  Word16 bad_pitch;		/* bad pitch indicator */

  /* Test bad frame indicator (bfi) */

  bfi = *parm++;

  /* Decode the LSPs */

  D_lsp (decoder, parm, lsp_new, add (bfi, *bad_lsf));
  parm += 2;

  /*
     Note: "bad_lsf" is introduce in case the standard is used with
     channel protection.
   */

  /* Interpolation of LPC for the 2 subframes */

  Int_qlpc (decoder->lsp_old, lsp_new, A_t);

  /* update the LSFs for the next frame */

  Copy (lsp_new, decoder->lsp_old, M10);

/*------------------------------------------------------------------------*
 *          Loop for every subframe in the analysis frame                 *
 *------------------------------------------------------------------------*
 * The subframe size is L_SUBFR and the loop is repeated L_FRAME/L_SUBFR  *
 *  times                                                                 *
 *     - decode the pitch delay                                           *
 *     - decode algebraic code                                            *
 *     - decode pitch and codebook gains                                  *
 *     - find the excitation and compute synthesis speech                 *
 *------------------------------------------------------------------------*/

  Az = A_t;			/* pointer to interpolated LPC parameters */

  for (i_subfr = 0; i_subfr < L_FRAME; i_subfr += L_SUBFR) {

    index = *parm++;		/* pitch index */

    if (i_subfr == 0) {
      i = *parm++;		/* get parity check result */
      bad_pitch = add (bfi, i);
      if (bad_pitch == 0) {
	Dec_lag3 (index, PIT_MIN, PIT_MAX, i_subfr, &T0, &T0_frac);
	decoder->old_T0 = T0;
      }
      else {			/* Bad frame, or parity error */

	T0 = decoder->old_T0;
	T0_frac = 0;
	decoder->old_T0 = add (decoder->old_T0, 1);
	if (sub (decoder->old_T0, PIT_MAX) > 0) {
	  decoder->old_T0 = PIT_MAX;
	}
      }
    }
    else {			/* second subframe */

      if (bfi == 0) {
	Dec_lag3 (index, PIT_MIN, PIT_MAX, i_subfr, &T0, &T0_frac);
	decoder->old_T0 = T0;
      }
      else {
	T0 = decoder->old_T0;
	T0_frac = 0;
	decoder->old_T0 = add (decoder->old_T0, 1);
	if (sub (decoder->old_T0, PIT_MAX) > 0) {
	  decoder->old_T0 = PIT_MAX;
	}
      }
    }
    *T2++ = T0;

   /*-------------------------------------------------*
    * - Find the adaptive codebook vector.            *
    *-------------------------------------------------*/

    Pred_lt_3 (&decoder->exc[i_subfr], T0, T0_frac, L_SUBFR);

   /*-------------------------------------------------------*
    * - Decode innovative codebook.                         *
    * - Add the fixed-gain pitch contribution to code[].    *
    *-------------------------------------------------------*/

    if (bfi != 0) {		/* Bad frame */

      parm[0] = Random_16 (&decoder->seed) & (Word16) 0x1fff;	/* 13 bits random */
      parm[1] = Random_16 (&decoder->seed) & (Word16) 0x000f;	/*  4 bits random */
    }
    Decod_ACELP (parm[1], parm[0], code);
    parm += 2;

    j = shl (decoder->sharp, 1);		/* From Q14 to Q15 */
    if (sub (T0, L_SUBFR) < 0) {
      for (i = T0; i < L_SUBFR; i++) {
	code[i] = add (code[i], mult (code[i - T0], j));
      }
    }

   /*-------------------------------------------------*
    * - Decode pitch and codebook gains.              *
    *-------------------------------------------------*/

    index = *parm++;		/* index of energy VQ */

    Dec_gain (decoder, index, code, L_SUBFR, bfi, &decoder->gain_pitch, &decoder->gain_code);

   /*-------------------------------------------------------------*
    * - Update pitch sharpening "sharp" with quantized gain_pitch *
    *-------------------------------------------------------------*/

    decoder->sharp = decoder->gain_pitch;
    if (sub (decoder->sharp, SHARPMAX) > 0) {
      decoder->sharp = SHARPMAX;
    }
    if (sub (decoder->sharp, SHARPMIN) < 0) {
      decoder->sharp = SHARPMIN;
    }

   /*-------------------------------------------------------*
    * - Find the total excitation.                          *
    * - Find synthesis speech corresponding to exc[].       *
    *-------------------------------------------------------*/

    for (i = 0; i < L_SUBFR; i++) {
      /* exc[i] = gain_pitch*exc[i] + gain_code*code[i]; */
      /* exc[i]  in Q0   gain_pitch in Q14               */
      /* code[i] in Q13  gain_codeode in Q1              */

      L_temp = L_mult (decoder->exc[i + i_subfr], decoder->gain_pitch);
      L_temp = L_mac (L_temp, code[i], decoder->gain_code);
      L_temp = L_shl (L_temp, 1);
      decoder->exc[i + i_subfr] = wround (L_temp);
    }

    Overflow = 0;
    Syn_filt (Az, &decoder->exc[i_subfr], &synth[i_subfr], L_SUBFR, decoder->mem_syn, 0, &Overflow);
    if (Overflow != 0) {
      /* In case of overflow in the synthesis          */
      /* -> Scale down vector exc[] and redo synthesis */

      for (i = 0; i < PIT_MAX + L_INTERPOL + L_FRAME; i++)
	decoder->old_exc[i] = shr (decoder->old_exc[i], 2);

      Syn_filt (Az, &decoder->exc[i_subfr], &synth[i_subfr], L_SUBFR, decoder->mem_syn, 1, NULL);
    }
    else
      Copy (&synth[i_subfr + L_SUBFR - M10], decoder->mem_syn, M10);

    Az += MP1;			/* interpolated LPC parameters for next subframe */
  }

 /*--------------------------------------------------*
  * Update signal for next frame.                    *
  * -> shift to the left by L_FRAME  exc[]           *
  *--------------------------------------------------*/

  Copy (&decoder->old_exc[L_FRAME], &decoder->old_exc[0], PIT_MAX + L_INTERPOL);

  return;
}
