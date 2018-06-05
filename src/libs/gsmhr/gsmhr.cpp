#include "gsmhr.h"

namespace GsmHr {

/***************************************************************************
 *
 *   File Name:  mathdp31.c
 *
 *   Purpose:  Contains functions increased-precision arithmetic operations.
 *
 *      Below is a listing of all the functions in this file.  There
 *      is no interdependence among the functions.
 *
 *      L_mpy_ls()
 *      L_mpy_ll()
 *      isLwLimit()
 *      isSwLimit()
 *
 ***************************************************************************/
/*_________________________________________________________________________
 |                                                                         |
 |                              Include Files                              |
 |_________________________________________________________________________|
*/


/****************************************************************************
 *
 *     FUNCTION NAME: isLwLimit
 *
 *     PURPOSE:
 *
 *        Check to see if the input int32_t is at the
 *        upper or lower limit of its range.  i.e.
 *        0x7fff ffff or -0x8000 0000
 *
 *        Ostensibly this is a check for an overflow.
 *        This does not truly mean an overflow occurred,
 *        it means the value reached is the
 *        maximum/minimum value representable.  It may
 *        have come about due to an overflow.
 *
 *     INPUTS:
 *
 *       L_In               A int32_t input variable
 *
 *
 *     OUTPUTS:             none
 *
 *     RETURN VALUE:        1 if input == 0x7fff ffff or -0x8000 0000
 *                          0 otherwise
 *
 *     KEYWORDS: saturation, limit
 *
 ***************************************************************************/

short  isLwLimit(int32_t L_In)
{

  int32_t L_ls;
  short  siOut;

  if (L_In != 0)
  {
    L_ls = L_shl(L_In, 1);
    if (L_sub(L_In, L_ls) == 0)
      siOut = 1;
    else
      siOut = 0;
  }
  else
  {
    siOut = 0;
  }
  return (siOut);
}

/****************************************************************************
 *
 *     FUNCTION NAME: isSwLimit
 *
 *     PURPOSE:
 *
 *        Check to see if the input int16_t is at the
 *        upper or lower limit of its range.  i.e.
 *        0x7fff or -0x8000
 *
 *        Ostensibly this is a check for an overflow.
 *        This does not truly mean an overflow occurred,
 *        it means the value reached is the
 *        maximum/minimum value representable.  It may
 *        have come about due to an overflow.
 *
 *     INPUTS:
 *
 *       swIn               A int16_t input variable
 *
 *
 *     OUTPUTS:             none
 *
 *     RETURN VALUE:        1 if input == 0x7fff or -0x8000
 *                          0 otherwise
 *
 *     KEYWORDS: saturation, limit
 *
 ***************************************************************************/

short  isSwLimit(int16_t swIn)
{

  int16_t swls;
  short  siOut;

  if (swIn != 0)
  {
    swls = shl(swIn, 1);
    if (sub(swIn, swls) == 0)          /* logical compare outputs 1/0 */
      siOut = 1;
    else
      siOut = 0;
  }
  else
  {
    siOut = 0;
  }
  return (siOut);

}

/****************************************************************************
 *
 *     FUNCTION NAME: L_mpy_ll
 *
 *     PURPOSE:    Multiply a 32 bit number (L_var1) and a 32 bit number
 *                 (L_var2), and return a 32 bit result.
 *
 *     INPUTS:
 *
 *       L_var1             A int32_t input variable
 *
 *       L_var2             A int32_t input variable
 *
 *     OUTPUTS:             none
 *
 *     IMPLEMENTATION:
 *
 *        Performs a 31x31 bit multiply, Complexity=24 Ops.
 *
 *        Let x1x0, or y1y0, be the two constituent halves
 *        of a 32 bit number.  This function performs the
 *        following:
 *
 *        low = ((x0 >> 1)*(y0 >> 1)) >> 16     (low * low)
 *        mid1 = [(x1 * (y0 >> 1)) >> 1 ]       (high * low)
 *        mid2 = [(y1 * (x0 >> 1)) >> 1]        (high * low)
 *        mid =  (mid1 + low + mid2) >> 14      (sum so far)
 *        output = (y1*x1) + mid                (high * high)
 *
 *
 *     RETURN VALUE:        A int32_t value
 *
 *     KEYWORDS: mult,mpy,multiplication
 *
 ***************************************************************************/

int32_t L_mpy_ll(int32_t L_var1, int32_t L_var2)
{
  int16_t swLow1,
         swLow2,
         swHigh1,
         swHigh2;
  int32_t L_varOut,
         L_low,
         L_mid1,
         L_mid2,
         L_mid;


  swLow1 = shr(extract_l(L_var1), 1);
  swLow1 = SW_MAX & swLow1;

  swLow2 = shr(extract_l(L_var2), 1);
  swLow2 = SW_MAX & swLow2;
  swHigh1 = extract_h(L_var1);
  swHigh2 = extract_h(L_var2);

  L_low = L_mult(swLow1, swLow2);
  L_low = L_shr(L_low, 16);

  L_mid1 = L_mult(swHigh1, swLow2);
  L_mid1 = L_shr(L_mid1, 1);
  L_mid = L_add(L_mid1, L_low);

  L_mid2 = L_mult(swHigh2, swLow1);
  L_mid2 = L_shr(L_mid2, 1);
  L_mid = L_add(L_mid, L_mid2);

  L_mid = L_shr(L_mid, 14);
  L_varOut = L_mac(L_mid, swHigh1, swHigh2);

  return (L_varOut);
}

/****************************************************************************
 *
 *     FUNCTION NAME: L_mpy_ls
 *
 *     PURPOSE:    Multiply a 32 bit number (L_var2) and a 16 bit
 *                 number (var1) returning a 32 bit result. L_var2
 *                 is truncated to 31 bits prior to executing the
 *                 multiply.
 *
 *     INPUTS:
 *
 *       L_var2             A int32_t input variable
 *
 *       var1               A int16_t input variable
 *
 *     OUTPUTS:             none
 *
 *     RETURN VALUE:        A int32_t value
 *
 *     KEYWORDS: mult,mpy,multiplication
 *
 ***************************************************************************/

int32_t L_mpy_ls(int32_t L_var2, int16_t var1)
{
  int32_t L_varOut;
  int16_t swtemp;

  swtemp = shr(extract_l(L_var2), 1);
  swtemp = (short) 32767 & (short) swtemp;

  L_varOut = L_mult(var1, swtemp);
  L_varOut = L_shr(L_varOut, 15);
  L_varOut = L_mac(L_varOut, var1, extract_h(L_var2));
  return (L_varOut);
}

// From mathhalf.c

/***************************************************************************
 *
 *   FUNCTION NAME: saturate
 *
 *   PURPOSE:
 *
 *     Limit the 32 bit input to the range of a 16 bit word.
 *
 *
 *   INPUTS:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   KEYWORDS: saturation, limiting, limit, saturate, 16 bits
 *
 *************************************************************************/

static int16_t saturate(int32_t L_var1)
{
  int16_t swOut;

  if (L_var1 > SW_MAX)
  {
    swOut = SW_MAX;
  }
  else if (L_var1 < SW_MIN)
  {
    swOut = SW_MIN;
  }
  else
    swOut = (int16_t) L_var1;        /* automatic type conversion */
  return (swOut);
}

/***************************************************************************
 *
 *   FUNCTION NAME: abs_s
 *
 *   PURPOSE:
 *
 *     Take the absolute value of the 16 bit input.  An input of
 *     -0x8000 results in a return value of 0x7fff.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0x0000 0000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     Take the absolute value of the 16 bit input.  An input of
 *     -0x8000 results in a return value of 0x7fff.
 *
 *   KEYWORDS: absolute value, abs
 *
 *************************************************************************/

int16_t abs_s(int16_t var1)
{
  int16_t swOut;

  if (var1 == SW_MIN)
  {
    swOut = SW_MAX;
  }
  else
  {
    if (var1 < 0)
      swOut = -var1;
    else
      swOut = var1;
  }
  return (swOut);
}

/***************************************************************************
 *
 *   FUNCTION NAME: add
 *
 *   PURPOSE:
 *
 *     Perform the addition of the two 16 bit input variable with
 *     saturation.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     Perform the addition of the two 16 bit input variable with
 *     saturation.
 *
 *     swOut = var1 + var2
 *
 *     swOut is set to 0x7fff if the operation results in an
 *     overflow.  swOut is set to 0x8000 if the operation results
 *     in an underflow.
 *
 *   KEYWORDS: add, addition
 *
 *************************************************************************/

int16_t add(int16_t var1, int16_t var2)
{
  int32_t L_sum;
  int16_t swOut;

  L_sum = (int32_t) var1 + var2;
  swOut = saturate(L_sum);
  return (swOut);
}

/***************************************************************************
 *
 *   FUNCTION NAME: divide_s
 *
 *   PURPOSE:
 *
 *     Divide var1 by var2.  Note that both must be positive, and
 *     var1 >=  var2.  The output is set to 0 if invalid input is
 *     provided.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     In the case where var1==var2 the function returns 0x7fff.  The output
 *     is undefined for invalid inputs.  This implementation returns zero
 *     and issues a warning via stdio if invalid input is presented.
 *
 *   KEYWORDS: divide
 *
 *************************************************************************/

int16_t divide_s(int16_t var1, int16_t var2)
{
  int32_t L_div;
  int16_t swOut;

  if (var1 < 0 || var2 < 0 || var1 > var2)
  {
    /* undefined output for invalid input into divide_s */
    return (0);
  }

  if (var1 == var2)
    return (0x7fff);

  L_div = ((0x00008000L * (int32_t) var1) / (int32_t) var2);
  swOut = saturate(L_div);
  return (swOut);
}

/***************************************************************************
 *
 *   FUNCTION NAME: extract_h
 *
 *   PURPOSE:
 *
 *     Extract the 16 MS bits of a 32 bit int32_t.  Return the 16 bit
 *     number as a int16_t.  This is used as a "truncation" of a fractional
 *     number.
 *
 *   INPUTS:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *   KEYWORDS: assign, truncate
 *
 *************************************************************************/

int16_t extract_h(int32_t L_var1)
{
  int16_t var2;

  var2 = (int16_t) (0x0000ffffL & (L_var1 >> 16));
  return (var2);
}

/***************************************************************************
 *
 *   FUNCTION NAME: extract_l
 *
 *   PURPOSE:
 *
 *     Extract the 16 LS bits of a 32 bit int32_t.  Return the 16 bit
 *     number as a int16_t.  The upper portion of the input int32_t
 *     has no impact whatsoever on the output.
 *
 *   INPUTS:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *
 *   KEYWORDS: extract, assign
 *
 *************************************************************************/

int16_t extract_l(int32_t L_var1)
{
  int16_t var2;

  var2 = (int16_t) (0x0000ffffL & L_var1);
  return (var2);
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_abs
 *
 *   PURPOSE:
 *
 *     Take the absolute value of the 32 bit input.  An input of
 *     -0x8000 0000 results in a return value of 0x7fff ffff.
 *
 *   INPUTS:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *
 *
 *   KEYWORDS: absolute value, abs
 *
 *************************************************************************/
int32_t L_abs(int32_t L_var1)
{
  int32_t L_Out;

  if (L_var1 == LW_MIN)
  {
    L_Out = LW_MAX;
  }
  else
  {
    if (L_var1 < 0)
      L_Out = -L_var1;
    else
      L_Out = L_var1;
  }
  return (L_Out);
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_add
 *
 *   PURPOSE:
 *
 *     Perform the addition of the two 32 bit input variables with
 *     saturation.
 *
 *   INPUTS:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *     L_var2
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   IMPLEMENTATION:
 *
 *     Perform the addition of the two 32 bit input variables with
 *     saturation.
 *
 *     L_Out = L_var1 + L_var2
 *
 *     L_Out is set to 0x7fff ffff if the operation results in an
 *     overflow.  L_Out is set to 0x8000 0000 if the operation
 *     results in an underflow.
 *
 *   KEYWORDS: add, addition
 *
 *************************************************************************/
int32_t L_add(int32_t L_var1, int32_t L_var2)
{

  int32_t L_Sum,
         L_SumLow,
         L_SumHigh;

  L_Sum = L_var1 + L_var2;

  if ((L_var1 > 0 && L_var2 > 0) || (L_var1 < 0 && L_var2 < 0))
  {

    /* an overflow is possible */

    L_SumLow = (L_var1 & 0xffff) + (L_var2 & 0xffff);
    L_SumHigh = ((L_var1 >> 16) & 0xffff) + ((L_var2 >> 16) & 0xffff);
    if (L_SumLow & 0x10000)
    {
      /* carry into high word is set */
      L_SumHigh += 1;
    }

    /* update sum only if there is an overflow or underflow */
    /*------------------------------------------------------*/

    if ((0x10000 & L_SumHigh) && !(0x8000 & L_SumHigh))
      L_Sum = LW_MIN;                  /* underflow */
    else if (!(0x10000 & L_SumHigh) && (0x8000 & L_SumHigh))
      L_Sum = LW_MAX;                  /* overflow */
  }

  return (L_Sum);

}

/***************************************************************************
 *
 *   FUNCTION NAME: L_deposit_h
 *
 *   PURPOSE:
 *
 *     Put the 16 bit input into the 16 MSB's of the output int32_t.  The
 *     LS 16 bits are zeroed.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff 0000.
 *
 *
 *   KEYWORDS: deposit, assign, fractional assign
 *
 *************************************************************************/

int32_t L_deposit_h(int16_t var1)
{
  int32_t L_var2;

  L_var2 = (int32_t) var1 << 16;
  return (L_var2);
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_deposit_l
 *
 *   PURPOSE:
 *
 *     Put the 16 bit input into the 16 LSB's of the output int32_t with
 *     sign extension i.e. the top 16 bits are set to either 0 or 0xffff.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= L_var1 <= 0x0000 7fff.
 *
 *   KEYWORDS: deposit, assign
 *
 *************************************************************************/

int32_t L_deposit_l(int16_t var1)
{
  int32_t L_Out;

  L_Out = var1;
  return (L_Out);
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_mac
 *
 *   PURPOSE:
 *
 *     Multiply accumulate.  Fractionally multiply two 16 bit
 *     numbers together with saturation.  Add that result to the
 *     32 bit input with saturation.  Return the 32 bit result.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *     L_var3
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   IMPLEMENTATION:
 *
 *     Fractionally multiply two 16 bit numbers together with
 *     saturation.  The only numbers which will cause saturation on
 *     the multiply are 0x8000 * 0x8000.
 *
 *     Add that result to the 32 bit input with saturation.
 *     Return the 32 bit result.
 *
 *     Please note that this is not a true multiply accumulate as
 *     most processors would implement it.  The 0x8000*0x8000
 *     causes and overflow for this instruction.  On most
 *     processors this would cause an overflow only if the 32 bit
 *     input added to it were positive or zero.
 *
 *   KEYWORDS: mac, multiply accumulate
 *
 *************************************************************************/

int32_t L_mac(int32_t L_var3, int16_t var1, int16_t var2)
{
  return (L_add(L_var3, L_mult(var1, var2)));
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_msu
 *
 *   PURPOSE:
 *
 *     Multiply and subtract.  Fractionally multiply two 16 bit
 *     numbers together with saturation.  Subtract that result from
 *     the 32 bit input with saturation.  Return the 32 bit result.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *     L_var3
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   IMPLEMENTATION:
 *
 *     Fractionally multiply two 16 bit numbers together with
 *     saturation.  The only numbers which will cause saturation on
 *     the multiply are 0x8000 * 0x8000.
 *
 *     Subtract that result from the 32 bit input with saturation.
 *     Return the 32 bit result.
 *
 *     Please note that this is not a true multiply accumulate as
 *     most processors would implement it.  The 0x8000*0x8000
 *     causes and overflow for this instruction.  On most
 *     processors this would cause an overflow only if the 32 bit
 *     input added to it were negative or zero.
 *
 *   KEYWORDS: mac, multiply accumulate, msu
 *
 *************************************************************************/

int32_t L_msu(int32_t L_var3, int16_t var1, int16_t var2)
{
  return (L_sub(L_var3, L_mult(var1, var2)));
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_mult
 *
 *   PURPOSE:
 *
 *     Perform a fractional multipy of the two 16 bit input numbers
 *     with saturation.  Output a 32 bit number.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   IMPLEMENTATION:
 *
 *     Multiply the two the two 16 bit input numbers. If the
 *     result is within this range, left shift the result by one
 *     and output the 32 bit number.  The only possible overflow
 *     occurs when var1==var2==-0x8000.  In this case output
 *     0x7fff ffff.
 *
 *   KEYWORDS: multiply, mult, mpy
 *
 *************************************************************************/

int32_t L_mult(int16_t var1, int16_t var2)
{
  int32_t L_product;

  if (var1 == SW_MIN && var2 == SW_MIN)
    L_product = LW_MAX;                /* overflow */
  else
  {
    L_product = (int32_t) var1 *var2; /* integer multiply */

    L_product = L_product << 1;
  }
  return (L_product);
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_negate
 *
 *   PURPOSE:
 *
 *     Negate the 32 bit input. 0x8000 0000's negated value is
 *     0x7fff ffff.
 *
 *   INPUTS:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0001 <= L_var1 <= 0x7fff ffff.
 *
 *   KEYWORDS: negate, negative
 *
 *************************************************************************/

int32_t L_negate(int32_t L_var1)
{
  int32_t L_Out;

  if (L_var1 == LW_MIN)
    L_Out = LW_MAX;
  else
    L_Out = -L_var1;
  return (L_Out);
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_shift_r
 *
 *   PURPOSE:
 *
 *     Shift and round.  Perform a shift right. After shifting, use
 *     the last bit shifted out of the LSB to round the result up
 *     or down.
 *
 *   INPUTS:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *
 *   IMPLEMENTATION:
 *
 *     Shift and round.  Perform a shift right. After shifting, use
 *     the last bit shifted out of the LSB to round the result up
 *     or down.  This is just like shift_r above except that the
 *     input/output is 32 bits as opposed to 16.
 *
 *     if var2 is positve perform a arithmetic left shift
 *     with saturation (see L_shl() above).
 *
 *     If var2 is zero simply return L_var1.
 *
 *     If var2 is negative perform a arithmetic right shift (L_shr)
 *     of L_var1 by (-var2)+1.  Add the LS bit of the result to
 *     L_var1 shifted right (L_shr) by -var2.
 *
 *     Note that there is no constraint on var2, so if var2 is
 *     -0xffff 8000 then -var2 is 0x0000 8000, not 0x0000 7fff.
 *     This is the reason the L_shl function is used.
 *
 *
 *   KEYWORDS:
 *
 *************************************************************************/

int32_t L_shift_r(int32_t L_var1, int16_t var2)
{
  int32_t L_Out,
         L_rnd;

  if (var2 < -31)
  {
    L_Out = 0;
  }
  else if (var2 < 0)
  {
    /* right shift */
    L_rnd = L_shl(L_var1, var2 + 1) & 0x1;
    L_Out = L_add(L_shl(L_var1, var2), L_rnd);
  }
  else
    L_Out = L_shl(L_var1, var2);

  return (L_Out);
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_shl
 *
 *   PURPOSE:
 *
 *     Arithmetic shift left (or right).
 *     Arithmetically shift the input left by var2.   If var2 is
 *     negative then an arithmetic shift right (L_shr) of L_var1 by
 *     -var2 is performed.
 *
 *   INPUTS:
 *
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *
 *   IMPLEMENTATION:
 *
 *     Arithmetically shift the 32 bit input left by var2.  This
 *     operation maintains the sign of the input number. If var2 is
 *     negative then an arithmetic shift right (L_shr) of L_var1 by
 *     -var2 is performed.  See description of L_shr for details.
 *
 *     Equivalent to the Full-Rate GSM ">> n" operation.  Note that
 *     ANSI-C does not guarantee operation of the C ">>" or "<<"
 *     operator for negative numbers.
 *
 *   KEYWORDS: shift, arithmetic shift left,
 *
 *************************************************************************/

int32_t L_shl(int32_t L_var1, int16_t var2)
{

  int32_t L_Mask,
         L_Out;
  int    i,
         iOverflow = 0;

  if (var2 == 0 || L_var1 == 0)
  {
    L_Out = L_var1;
  }
  else if (var2 < 0)
  {
    if (var2 <= -31)
    {
      if (L_var1 > 0)
        L_Out = 0;
      else
        L_Out = 0xffffffffL;
    }
    else
      L_Out = L_shr(L_var1, -var2);
  }
  else
  {

    if (var2 >= 31)
      iOverflow = 1;

    else
    {

      if (L_var1 < 0)
        L_Mask = LW_SIGN;              /* sign bit mask */
      else
        L_Mask = 0x0;
      L_Out = L_var1;
      for (i = 0; i < var2 && !iOverflow; i++)
      {
        /* check the sign bit */
        L_Out = (L_Out & 0x7fffffffL) << 1;
        if ((L_Mask ^ L_Out) & LW_SIGN)
          iOverflow = 1;
      }
    }

    if (iOverflow)
    {
      /* saturate */
      if (L_var1 > 0)
        L_Out = LW_MAX;
      else
        L_Out = LW_MIN;
    }
  }

  return (L_Out);
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_shr
 *
 *   PURPOSE:
 *
 *     Arithmetic shift right (or left).
 *     Arithmetically shift the input right by var2.   If var2 is
 *     negative then an arithmetic shift left (shl) of var1 by
 *     -var2 is performed.
 *
 *   INPUTS:
 *
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *
 *   IMPLEMENTATION:
 *
 *     Arithmetically shift the input right by var2.  This
 *     operation maintains the sign of the input number. If var2 is
 *     negative then an arithmetic shift left (shl) of L_var1 by
 *     -var2 is performed.  See description of L_shl for details.
 *
 *     The input is a 32 bit number, as is the output.
 *
 *     Equivalent to the Full-Rate GSM ">> n" operation.  Note that
 *     ANSI-C does not guarantee operation of the C ">>" or "<<"
 *     operator for negative numbers.
 *
 *   KEYWORDS: shift, arithmetic shift right,
 *
 *************************************************************************/

int32_t L_shr(int32_t L_var1, int16_t var2)
{

  int32_t L_Mask,
         L_Out;

  if (var2 == 0 || L_var1 == 0)
  {
    L_Out = L_var1;
  }
  else if (var2 < 0)
  {
    /* perform a left shift */
    /*----------------------*/
    if (var2 <= -31)
    {
      /* saturate */
      if (L_var1 > 0)
        L_Out = LW_MAX;
      else
        L_Out = LW_MIN;
    }
    else
      L_Out = L_shl(L_var1, -var2);
  }
  else
  {

    if (var2 >= 31)
    {
      if (L_var1 > 0)
        L_Out = 0;
      else
        L_Out = 0xffffffffL;
    }
    else
    {
      L_Mask = 0;

      if (L_var1 < 0)
      {
        L_Mask = ~L_Mask << (32 - var2);
      }

      L_var1 >>= var2;
      L_Out = L_Mask | L_var1;
    }
  }
  return (L_Out);
}

/***************************************************************************
 *
 *   FUNCTION NAME: L_sub
 *
 *   PURPOSE:
 *
 *     Perform the subtraction of the two 32 bit input variables with
 *     saturation.
 *
 *   INPUTS:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *     L_var2
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     L_Out
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   IMPLEMENTATION:
 *
 *     Perform the subtraction of the two 32 bit input variables with
 *     saturation.
 *
 *     L_Out = L_var1 - L_var2
 *
 *     L_Out is set to 0x7fff ffff if the operation results in an
 *     overflow.  L_Out is set to 0x8000 0000 if the operation
 *     results in an underflow.
 *
 *   KEYWORDS: sub, subtraction
 *
 *************************************************************************/
int32_t L_sub(int32_t L_var1, int32_t L_var2)
{
  int32_t L_Sum;

  /* check for overflow */
  if ((L_var1 > 0 && L_var2 < 0) || (L_var1 < 0 && L_var2 > 0))
  {
    if (L_var2 == LW_MIN)
    {
      L_Sum = L_add(L_var1, LW_MAX);
      L_Sum = L_add(L_Sum, 1);
    }
    else
      L_Sum = L_add(L_var1, -L_var2);
  }
  else
  {                                    /* no overflow possible */
    L_Sum = L_var1 - L_var2;
  }
  return (L_Sum);
}

/***************************************************************************
 *
 *   FUNCTION NAME:mac_r
 *
 *   PURPOSE:
 *
 *     Multiply accumulate and round.  Fractionally multiply two 16
 *     bit numbers together with saturation.  Add that result to
 *     the 32 bit input with saturation.  Finally round the result
 *     into a 16 bit number.
 *
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *     L_var3
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     Fractionally multiply two 16 bit numbers together with
 *     saturation.  The only numbers which will cause saturation on
 *     the multiply are 0x8000 * 0x8000.
 *
 *     Add that result to the 32 bit input with saturation.
 *     Round the 32 bit result by adding 0x0000 8000 to the input.
 *     The result may overflow due to the add.  If so, the result
 *     is saturated.  The 32 bit rounded number is then shifted
 *     down 16 bits and returned as a int16_t.
 *
 *     Please note that this is not a true multiply accumulate as
 *     most processors would implement it.  The 0x8000*0x8000
 *     causes and overflow for this instruction.  On most
 *     processors this would cause an overflow only if the 32 bit
 *     input added to it were positive or zero.
 *
 *   KEYWORDS: mac, multiply accumulate, macr
 *
 *************************************************************************/

int16_t mac_r(int32_t L_var3, int16_t var1, int16_t var2)
{
  return (round(L_add(L_var3, L_mult(var1, var2))));
}

/***************************************************************************
 *
 *   FUNCTION NAME:  msu_r
 *
 *   PURPOSE:
 *
 *     Multiply subtract and round.  Fractionally multiply two 16
 *     bit numbers together with saturation.  Subtract that result from
 *     the 32 bit input with saturation.  Finally round the result
 *     into a 16 bit number.
 *
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *     L_var3
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var2 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     Fractionally multiply two 16 bit numbers together with
 *     saturation.  The only numbers which will cause saturation on
 *     the multiply are 0x8000 * 0x8000.
 *
 *     Subtract that result from the 32 bit input with saturation.
 *     Round the 32 bit result by adding 0x0000 8000 to the input.
 *     The result may overflow due to the add.  If so, the result
 *     is saturated.  The 32 bit rounded number is then shifted
 *     down 16 bits and returned as a int16_t.
 *
 *     Please note that this is not a true multiply accumulate as
 *     most processors would implement it.  The 0x8000*0x8000
 *     causes and overflow for this instruction.  On most
 *     processors this would cause an overflow only if the 32 bit
 *     input added to it were positive or zero.
 *
 *   KEYWORDS: mac, multiply accumulate, macr
 *
 *************************************************************************/

int16_t msu_r(int32_t L_var3, int16_t var1, int16_t var2)
{
  return (round(L_sub(L_var3, L_mult(var1, var2))));
}

/***************************************************************************
 *
 *   FUNCTION NAME: mult
 *
 *   PURPOSE:
 *
 *     Perform a fractional multipy of the two 16 bit input numbers
 *     with saturation and truncation.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     Perform a fractional multipy of the two 16 bit input
 *     numbers.  If var1 == var2 == -0x8000, output 0x7fff.
 *     Otherwise output var1*var2 >> 15.  The output is a
 *     16 bit number.
 *
 *   KEYWORDS: mult, mulitply, mpy
 *
 *************************************************************************/

int16_t mult(int16_t var1, int16_t var2)
{
  int32_t L_product;
  int16_t swOut;

  L_product = L_mult(var1, var2);
  swOut = extract_h(L_product);
  return (swOut);
}

/***************************************************************************
 *
 *   FUNCTION NAME: mult_r
 *
 *   PURPOSE:
 *
 *     Perform a fractional multipy and round of the two 16 bit
 *     input numbers with saturation.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     This routine is defined as the concatenation of the multiply
 *     operation and the round operation.
 *
 *     The fractional multiply (L_mult) produces a saturated 32 bit
 *     output.  This is followed by a an add of 0x0000 8000 to the
 *     32 bit result.  The result may overflow due to the add.  If
 *     so, the result is saturated.  The 32 bit rounded number is
 *     then shifted down 16 bits and returned as a int16_t.
 *
 *
 *   KEYWORDS: multiply and round, round, mult_r, mpyr
 *
 *************************************************************************/


int16_t mult_r(int16_t var1, int16_t var2)
{
  int16_t swOut;

  swOut = round(L_mult(var1, var2));
  return (swOut);
}

/***************************************************************************
 *
 *   FUNCTION NAME: negate
 *
 *   PURPOSE:
 *
 *     Negate the 16 bit input. 0x8000's negated value is 0x7fff.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8001 <= swOut <= 0x0000 7fff.
 *
 *   KEYWORDS: negate, negative, invert
 *
 *************************************************************************/

int16_t negate(int16_t var1)
{
  int16_t swOut;

  if (var1 == SW_MIN)
    swOut = SW_MAX;
  else
    swOut = -var1;
  return (swOut);
}

/***************************************************************************
 *
 *   FUNCTION NAME: norm_l
 *
 *   PURPOSE:
 *
 *     Get normalize shift count:
 *
 *     A 32 bit number is input (possiblly unnormalized).  Output
 *     the positive (or zero) shift count required to normalize the
 *     input.
 *
 *   INPUTS:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0 <= swOut <= 31
 *
 *
 *
 *   IMPLEMENTATION:
 *
 *     Get normalize shift count:
 *
 *     A 32 bit number is input (possiblly unnormalized).  Output
 *     the positive (or zero) shift count required to normalize the
 *     input.
 *
 *     If zero in input, return 0 as the shift count.
 *
 *     For non-zero numbers, count the number of left shift
 *     required to get the number to fall into the range:
 *
 *     0x4000 0000 >= normlzd number >= 0x7fff ffff (positive number)
 *     or
 *     0x8000 0000 <= normlzd number < 0xc000 0000 (negative number)
 *
 *     Return the number of shifts.
 *
 *     This instruction corresponds exactly to the Full-Rate "norm"
 *     instruction.
 *
 *   KEYWORDS: norm, normalization
 *
 *************************************************************************/

int16_t norm_l(int32_t L_var1)
{

  int16_t swShiftCnt;

  if (L_var1 != 0)
  {
    if (!(L_var1 & LW_SIGN))
    {

      /* positive input */
      for (swShiftCnt = 0; !(L_var1 <= LW_MAX && L_var1 >= 0x40000000L);
           swShiftCnt++)
      {
        L_var1 = L_var1 << 1;
      }

    }
    else
    {
      /* negative input */
      int32_t lwMin = (int32_t)LW_MIN;
      int32_t upperLimit = (int32_t)0xc0000000L;
      for (swShiftCnt = 0; !(L_var1 >= lwMin && L_var1 < upperLimit ); swShiftCnt++)
      {
        L_var1 = L_var1 << 1;
      }
    }
  }
  else
  {
    swShiftCnt = 0;
  }
  return (swShiftCnt);
}

/***************************************************************************
 *
 *   FUNCTION NAME: norm_s
 *
 *   PURPOSE:
 *
 *     Get normalize shift count:
 *
 *     A 16 bit number is input (possiblly unnormalized).  Output
 *     the positive (or zero) shift count required to normalize the
 *     input.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0 <= swOut <= 15
 *
 *
 *
 *   IMPLEMENTATION:
 *
 *     Get normalize shift count:
 *
 *     A 16 bit number is input (possiblly unnormalized).  Output
 *     the positive (or zero) shift count required to normalize the
 *     input.
 *
 *     If zero in input, return 0 as the shift count.
 *
 *     For non-zero numbers, count the number of left shift
 *     required to get the number to fall into the range:
 *
 *     0x4000 >= normlzd number >= 0x7fff (positive number)
 *     or
 *     0x8000 <= normlzd number <  0xc000 (negative number)
 *
 *     Return the number of shifts.
 *
 *     This instruction corresponds exactly to the Full-Rate "norm"
 *     instruction.
 *
 *   KEYWORDS: norm, normalization
 *
 *************************************************************************/

int16_t norm_s(int16_t var1)
{

  short  swShiftCnt;
  int32_t L_var1;

  L_var1 = L_deposit_h(var1);
  swShiftCnt = norm_l(L_var1);
  return (swShiftCnt);
}

/***************************************************************************
 *
 *   FUNCTION NAME: round
 *
 *   PURPOSE:
 *
 *     Round the 32 bit int32_t into a 16 bit int16_t with saturation.
 *
 *   INPUTS:
 *
 *     L_var1
 *                     32 bit long signed integer (int32_t) whose value
 *                     falls in the range
 *                     0x8000 0000 <= L_var1 <= 0x7fff ffff.
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     Perform a two's complement round on the input int32_t with
 *     saturation.
 *
 *     This is equivalent to adding 0x0000 8000 to the input.  The
 *     result may overflow due to the add.  If so, the result is
 *     saturated.  The 32 bit rounded number is then shifted down
 *     16 bits and returned as a int16_t.
 *
 *
 *   KEYWORDS: round
 *
 *************************************************************************/

int16_t round(int32_t L_var1)
{
  int32_t L_Prod;

  L_Prod = L_add(L_var1, 0x00008000L); /* round MSP */
  return (extract_h(L_Prod));
}

/***************************************************************************
 *
 *   FUNCTION NAME: shift_r
 *
 *   PURPOSE:
 *
 *     Shift and round.  Perform a shift right. After shifting, use
 *     the last bit shifted out of the LSB to round the result up
 *     or down.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *
 *   IMPLEMENTATION:
 *
 *     Shift and round.  Perform a shift right. After shifting, use
 *     the last bit shifted out of the LSB to round the result up
 *     or down.
 *
 *     If var2 is positive perform a arithmetic left shift
 *     with saturation (see shl() above).
 *
 *     If var2 is zero simply return var1.
 *
 *     If var2 is negative perform a arithmetic right shift (shr)
 *     of var1 by (-var2)+1.  Add the LS bit of the result to var1
 *     shifted right (shr) by -var2.
 *
 *     Note that there is no constraint on var2, so if var2 is
 *     -0xffff 8000 then -var2 is 0x0000 8000, not 0x0000 7fff.
 *     This is the reason the shl function is used.
 *
 *
 *   KEYWORDS:
 *
 *************************************************************************/

int16_t shift_r(int16_t var1, int16_t var2)
{
  int16_t swOut,
         swRnd;

  if (var2 >= 0)
    swOut = shl(var1, var2);
  else
  {

    /* right shift */

    if (var2 < -15)
    {

      swOut = 0;

    }
    else
    {

      swRnd = shl(var1, var2 + 1) & 0x1;
      swOut = add(shl(var1, var2), swRnd);

    }
  }
  return (swOut);
}

/***************************************************************************
 *
 *   FUNCTION NAME: shl
 *
 *   PURPOSE:
 *
 *     Arithmetically shift the input left by var2.
 *
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     If Arithmetically shift the input left by var2.  If var2 is
 *     negative then an arithmetic shift right (shr) of var1 by
 *     -var2 is performed.  See description of shr for details.
 *     When an arithmetic shift left is performed the var2 LS bits
 *     are zero filled.
 *
 *     The only exception is if the left shift causes an overflow
 *     or underflow.  In this case the LS bits are not modified.
 *     The number returned is 0x8000 in the case of an underflow or
 *     0x7fff in the case of an overflow.
 *
 *     The shl is equivalent to the Full-Rate GSM "<< n" operation.
 *     Note that ANSI-C does not guarantee operation of the C ">>"
 *     or "<<" operator for negative numbers - it is not specified
 *     whether this shift is an arithmetic or logical shift.
 *
 *   KEYWORDS: asl, arithmetic shift left, shift
 *
 *************************************************************************/

int16_t shl(int16_t var1, int16_t var2)
{
  int16_t swOut;
  int32_t L_Out;

  if (var2 == 0 || var1 == 0)
  {
    swOut = var1;
  }
  else if (var2 < 0)
  {

    /* perform a right shift */
    /*-----------------------*/

    if (var2 <= -15)
    {
      if (var1 < 0)
        swOut = (int16_t) 0xffff;
      else
        swOut = 0x0;
    }
    else
      swOut = shr(var1, -var2);

  }
  else
  {
    /* var2 > 0 */
    if (var2 >= 15)
    {
      /* saturate */
      if (var1 > 0)
        swOut = SW_MAX;
      else
        swOut = SW_MIN;
    }
    else
    {

      L_Out = (int32_t) var1 *(1 << var2);

      swOut = (int16_t) L_Out;       /* copy low portion to swOut, overflow
                                        * could have hpnd */
      if (swOut != L_Out)
      {
        /* overflow  */
        if (var1 > 0)
          swOut = SW_MAX;              /* saturate */
        else
          swOut = SW_MIN;              /* saturate */
      }
    }
  }
  return (swOut);
}

/***************************************************************************
 *
 *   FUNCTION NAME: shr
 *
 *   PURPOSE:
 *
 *     Arithmetic shift right (or left).
 *     Arithmetically shift the input right by var2.   If var2 is
 *     negative then an arithmetic shift left (shl) of var1 by
 *     -var2 is performed.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     Arithmetically shift the input right by var2.  This
 *     operation maintains the sign of the input number. If var2 is
 *     negative then an arithmetic shift left (shl) of var1 by
 *     -var2 is performed.  See description of shl for details.
 *
 *     Equivalent to the Full-Rate GSM ">> n" operation.  Note that
 *     ANSI-C does not guarantee operation of the C ">>" or "<<"
 *     operator for negative numbers.
 *
 *   KEYWORDS: shift, arithmetic shift right,
 *
 *************************************************************************/

int16_t shr(int16_t var1, int16_t var2)
{

  int16_t swMask,
         swOut;

  if (var2 == 0 || var1 == 0)
    swOut = var1;

  else if (var2 < 0)
  {
    /* perform an arithmetic left shift */
    /*----------------------------------*/
    if (var2 <= -15)
    {
      /* saturate */
      if (var1 > 0)
        swOut = SW_MAX;
      else
        swOut = SW_MIN;
    }
    else
      swOut = shl(var1, -var2);
  }

  else
  {

    /* positive shift count */
    /*----------------------*/

    if (var2 >= 15)
    {
      if (var1 < 0)
        swOut = (int16_t) 0xffff;
      else
        swOut = 0x0;
    }
    else
    {
      /* take care of sign extension */
      /*-----------------------------*/

      swMask = 0;
      if (var1 < 0)
      {
        swMask = ~swMask << (16 - var2);
      }

      var1 >>= var2;
      swOut = swMask | var1;

    }
  }
  return (swOut);
}

/***************************************************************************
 *
 *   FUNCTION NAME: sub
 *
 *   PURPOSE:
 *
 *     Perform the subtraction of the two 16 bit input variable with
 *     saturation.
 *
 *   INPUTS:
 *
 *     var1
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var1 <= 0x0000 7fff.
 *     var2
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range 0xffff 8000 <= var2 <= 0x0000 7fff.
 *
 *   OUTPUTS:
 *
 *     none
 *
 *   RETURN VALUE:
 *
 *     swOut
 *                     16 bit short signed integer (int16_t) whose value
 *                     falls in the range
 *                     0xffff 8000 <= swOut <= 0x0000 7fff.
 *
 *   IMPLEMENTATION:
 *
 *     Perform the subtraction of the two 16 bit input variable with
 *     saturation.
 *
 *     swOut = var1 - var2
 *
 *     swOut is set to 0x7fff if the operation results in an
 *     overflow.  swOut is set to 0x8000 if the operation results
 *     in an underflow.
 *
 *   KEYWORDS: sub, subtraction
 *
 *************************************************************************/
int16_t sub(int16_t var1, int16_t var2)
{
  int32_t L_diff;
  int16_t swOut;

  L_diff = (int32_t) var1 - var2;
  swOut = saturate(L_diff);

  return (swOut);
}

// From err_conc.c

#define MIN_MUTE_LEVEL -45

/*****************************************************************************
 *
 *   FUNCTION NAME: para_conceal_speech_decoder
 *
 *     This subroutine performs concealment on parameter level. If the
 *     badframe flag (swErrorFlag[0]) has been set in the channel decoder
 *     parameter repetition is performed.
 *     If the average frame energy R0 shows an abnormal increase between two
 *     subsequent frames the badframe flag is also set and parameter
 *     repetition is performed.
 *     If R0 shows an important increase muting is permitted in the signal
 *     concealment unit. There the level of the synthesized speech signal is
 *     controlled and corrected if necessary.
 *
 *     In table "psrR0RepeatThreshold[]" the maximum allowed
 *     increase of R0 for badframe setting is stored. The table
 *     is controlled by the value of R0 of the last frame.
 *     If e.g. the previous R0 is 10 the allowed maximum increase
 *     is 9 (psrR0RepeatThreshold[10]).
 *     The figures in psrR0RepeatThreshold[] have been determined
 *     by measuring the R0 statistics of an error free speech
 *     signal. In approximately 95 % of the frames the increase of
 *     R0 is less than the defined figures for error free speech.
 *     If the level increase is higher than the determined limit
 *     then the badframe flag is set.
 *
 *     In table "psrR0MuteThreshold[]" the maximum allowed
 *     increase of R0 for muting is stored.
 *     The table is controlled by the value of R0 of the last frame
 *     If e.g. the previous R0 is 10 the allowed maximum increase
 *     is 7 (psrR0MuteThreshold[10]).
 *     The figures in psrR0MuteThreshold[] have been determined
 *     by measuring the R0 statistics of an error free speech
 *     signal. In approximately 85 % of the frames the increase of
 *     R0 is less than the defined figures for error free speech.
 *     If the level increase is higher than the determined limit
 *     then muting is allowed.
 *
 *     Input:     pswErrorFlag[0]   badframe flag from channel decoder
 *                pswErrorFlag[1]   unreliable frame flag from channel decoder
 *                pswSpeechPara[]   unconcealed speech parameters
 *     Output:    pswSpeechPara[]   concealed speech parameters
 *                swMutePermit      flag, indicating whether muting is
 *                                  permitted
 *
 *     Constants: psrR0RepeatThreshold[32]  maximum allowed R0 difference
 *                                          before frame is repeated
 *                psrR0MuteThreshold[32]    maximum allowed R0 difference
 *                                          before muting is permitted
 *
 *
 ****************************************************************************/

void Codec::para_conceal_speech_decoder(int16_t pswErrorFlag[],
                        int16_t pswSpeechPara[], int16_t *pswMutePermit)
{

/*_________________________________________________________________________
 |                                                                         |
 |                           Local Static Variables                        |
 |_________________________________________________________________________|
*/
  static const int16_t psrR0RepeatThreshold[32] =
  {15, 15, 15, 12, 12, 12, 12, 11,
    10, 10, 9, 9, 9, 9, 8, 8,
    7, 6, 5, 5, 5, 4, 4, 3,
  2, 2, 2, 2, 2, 2, 10, 10};
  static const int16_t psrR0MuteThreshold[32] =
  {14, 12, 11, 9, 9, 9, 9, 7,
    7, 7, 7, 7, 6, 6, 6, 5,
    5, 4, 3, 3, 3, 3, 3, 2,
  1, 1, 1, 1, 1, 1, 10, 10};

/*_________________________________________________________________________
 |                                                                         |
 |                            Automatic Variables                          |
 |_________________________________________________________________________|
*/
  int16_t swLastLag,
         swR0,
         swLag,
         r0_diff,
         i;


/*_________________________________________________________________________
 |                                                                         |
 |                              Executable Code                            |
 |_________________________________________________________________________|
*/

  /* Initialise mute permission flag */
  /* ------------------------------- */
  *pswMutePermit = 0;

  /* Determine R0-difference to last frame */
  /* ------------------------------------- */
  r0_diff = sub(pswSpeechPara[0], lastR0);

  /* If no badframe has been declared, but the frame is unreliable then */
  /* check whether there is an abnormal increase of R0                  */
  /* ------------------------------------------------------------------ */
  if ((pswErrorFlag[0] == 0) && (pswErrorFlag[1] > 0))
  {

    /* Check if difference exceeds the maximum allowed threshold. */
    /* If yes, set badframe flag                                  */
    /* ---------------------------------------------------------- */
    if (sub(r0_diff, psrR0RepeatThreshold[lastR0]) >= 0)
    {
      pswErrorFlag[0] = 1;
    }
    else
    {
      /* Allow muting if R0 >= 30 */
      /* ------------------------ */
      if (sub(pswSpeechPara[0], 30) >= 0)
        *pswMutePermit = 1;
    }
  }

  /* If no badframe has been declared, but the frame is unreliable then */
  /* check whether there is an important increase of R0                 */
  /* ------------------------------------------------------------------ */
  if ((pswErrorFlag[1] > 0) && (pswErrorFlag[0] == 0))
  {

    /* Check if difference exceeds a threshold.                   */
    /* If yes, allow muting in the signal concealment unit        */
    /* ---------------------------------------------------------- */
    if (sub(r0_diff, psrR0MuteThreshold[lastR0]) >= 0)
    {
      *pswMutePermit = 1;
    }
  }


  /* Perform parameter repetition, if necessary (badframe handling) */
  /* -------------------------------------------------------------- */

  if (pswErrorFlag[0] > 0)
  {
    swState = add(swState, 1);         /* update the bad frame
                                        * masking state */
    if (sub(swState, 6) > 0)
      swState = 6;
  }
  else
  {
    if (sub(swState, 6) < 0)
      swState = 0;
    else if (swLastFlag == 0)
      swState = 0;
  }

  swLastFlag = pswErrorFlag[0];

  /* if the decoded frame is good, save it */
  /* ------------------------------------- */
  if (swState == 0)
  {
    for (i = 0; i < 18; i++)
      pswLastGood[i] = pswSpeechPara[i];
  }

  /* if the frame is bad, attenuate and repeat last good frame */
  /* --------------------------------------------------------- */
  else
  {
    if ((sub(swState, 3) >= 0) && (sub(swState, 5) <= 0))
    {
      swR0 = sub(pswLastGood[0], 2);   /* attenuate by 4 dB */
      if (swR0 < 0)
        swR0 = 0;
      pswLastGood[0] = swR0;
    }

    if (sub(swState, 6) >= 0)          /* mute */
      pswLastGood[0] = 0;

    /* If the last good frame is unvoiced, use its energy, voicing mode, lpc
     * coefficients, and soft interpolation.   For gsp0, use only the gsp0
     * value from the last good subframe.  If the current bad frame is
     * unvoiced, use the current codewords.  If not, use the codewords from
     * the last good frame.               */
    /* -------------------------------------------------------------- */
    if (pswLastGood[5] == 0)
    {                                  /* unvoiced good frame */
      if (pswSpeechPara[5] == 0)
      {                                /* unvoiced bad frame */
        for (i = 0; i < 5; i++)
          pswSpeechPara[i] = pswLastGood[i];
        for (i = 0; i < 4; i++)
          pswSpeechPara[3 * i + 8] = pswLastGood[17];
      }
      else
      {                                /* voiced bad frame */
        for (i = 0; i < 18; i++)
          pswSpeechPara[i] = pswLastGood[i];
        for (i = 0; i < 3; i++)
          pswSpeechPara[3 * i + 8] = pswLastGood[17];
      }
    }

    /* If the last good frame is voiced, the long term predictor lag at the
     * last subframe is used for all subsequent subframes. Use the last good
     * frame's energy, voicing mode, lpc coefficients, and soft
     * interpolation.  For gsp0 in all subframes, use the gsp0 value from the
     * last good subframe.  If the current bad frame is voiced, use the
     * current codewords.  If not, use the codewords from the last good
     * frame.                              */
    /* ---------------------------------------------------------------- */
    else
    {                                  /* voiced good frame */
      swLastLag = pswLastGood[6];      /* frame lag */
      for (i = 0; i < 3; i++)
      {                                /* each delta lag */
        swLag = sub(pswLastGood[3 * i + 9], 0x8);       /* biased around 0 */
        swLag = add(swLag, swLastLag); /* reconstruct pitch */
        if (sub(swLag, 0x00ff) > 0)
        {                              /* limit, as needed */
          swLastLag = 0x00ff;
        }
        else if (swLag < 0)
        {
          swLastLag = 0;
        }
        else
          swLastLag = swLag;
      }
      pswLastGood[6] = swLastLag;      /* saved frame lag */
      pswLastGood[9] = 0x8;            /* saved delta lags */
      pswLastGood[12] = 0x8;
      pswLastGood[15] = 0x8;

      if (pswSpeechPara[5] != 0)
      {                                /* voiced bad frame */
        for (i = 0; i < 6; i++)
          pswSpeechPara[i] = pswLastGood[i];
        for (i = 0; i < 4; i++)
          pswSpeechPara[3 * i + 6] = pswLastGood[3 * i + 6];
        for (i = 0; i < 4; i++)
          pswSpeechPara[3 * i + 8] = pswLastGood[17];
      }
      else
      {                                /* unvoiced bad frame */
        for (i = 0; i < 18; i++)
          pswSpeechPara[i] = pswLastGood[i];
        for (i = 0; i < 3; i++)
          pswSpeechPara[3 * i + 8] = pswLastGood[17];
      }
    }

  }                                    /* end of bad frame */


  /* Update last value of R0 */
  /* ----------------------- */
  lastR0 = pswSpeechPara[0];

}


/****************************************************************************
 *
 *   FUNCTION NAME: level_estimator
 *
 *     This subroutine determines the mean level and the maximum level
 *     of the last four speech sub-frames. These parameters are the basis
 *     for the level estimation in signal_conceal_sub().
 *
 *     Input:     swUpdate      = 0: the levels are determined
 *                              = 1: the memory of the level estimator
 *                                   is updated
 *                pswDecodedSpeechFrame[]  synthesized speech signal
 *
 *     Output:    swLevelMean   mean level of the last 4 sub-frames
 *                swLevelMax    maximum level of the last 4 sub-frames
 *
 ***************************************************************************/

void Codec::level_estimator(int16_t update, int16_t *pswLevelMean,
                              int16_t *pswLevelMax,
                              int16_t pswDecodedSpeechFrame[])
{

/*_________________________________________________________________________
 |                                                                         |
 |                            Automatic Variables                          |
 |_________________________________________________________________________|
*/
  int16_t i,
         tmp,
         swLevelSub;
  int32_t L_sum;

/*_________________________________________________________________________
 |                                                                         |
 |                              Executable Code                            |
 |_________________________________________________________________________|
*/

  if (update == 0)
  {

    /* Determine mean level of the last 4 sub-frames: */
    /* ---------------------------------------------- */
    for (i = 0, L_sum = 0; i < 4; ++i)
    {
      L_sum = L_add(L_sum, plSubfrEnergyMem[i]);
    }
    *pswLevelMean = level_calc(1, &L_sum);

    /* Determine maximum level of the last 4 sub-frames: */
    /* ------------------------------------------------- */
    *pswLevelMax = -72;
    for (i = 0; i < 4; ++i)
    {
      if (sub(swLevelMem[i], *pswLevelMax) > 0)
        *pswLevelMax = swLevelMem[i];
    }

  }
  else
  {
    /* Determine the energy of the synthesized speech signal: */
    /* ------------------------------------------------------ */
    for (i = 0, L_sum = 0; i < S_LEN; ++i)
    {
      tmp = shr(pswDecodedSpeechFrame[i], 3);
      L_sum = L_mac(L_sum, tmp, tmp);
    }
    swLevelSub = level_calc(0, &L_sum);

    /* Update memories of level estimator: */
    /* ----------------------------------- */
    for (i = 0; i < 3; ++i)
      plSubfrEnergyMem[i] = plSubfrEnergyMem[i + 1];
    plSubfrEnergyMem[3] = L_sum;

    for (i = 0; i < 3; ++i)
      swLevelMem[i] = swLevelMem[i + 1];
    swLevelMem[3] = swLevelSub;
  }
}


/*****************************************************************************
 *
 *   FUNCTION NAME: signal_conceal_sub
 *
 *     This subroutine performs concealment on subframe signal level.
 *     A test synthesis is performed and the level of the synthesized speech
 *     signal is compared to the estimated level. Depending on the control
 *     flag "swMutePermit" a muting factor is determined.
 *     If muting is permitted (swMutePermit=1) and the actual sub-frame level
 *     exceeds the maximum level of the last four sub-frames "swLevelMax" plus
 *     an allowed increase "psrLevelMaxIncrease[]" then the synthesized speech
 *     signal together with the signal memories is muted.
 *     In table "psrLevelMaxIncrease[]" the maximum allowed increase
 *     of the maximum sub-frame level is stored. The table is controled by the
 *     mean level "swMeanLevel".
 *     If e.g. the level is in the range between -30 and -35 db
 *     the allowed maximum increase is 4 db (psrLevelMaxIncrease[6]).
 *     The figures in psrLevelMaxIncrease[] have been determined
 *     by measuring the level statistics of error free synthesized speech.
 *
 *     Input:     pswPPFExcit[]            excitation signal
 *                pswSynthFiltState[]      state of LPC synthesis filter
 *                ppswSynthAs[]            LPC coefficients
 *                pswLtpStateOut[]         state of long term predictor
 *                pswPPreState[]           state of pitch prefilter
 *                swLevelMean              mean level
 *                swLevelMax               maximum level
 *                swUFI                    unreliable frame flag
 *                swMuteFlagOld            last muting flag
 *                pswMuteFlag              actual muting flag
 *                swMutePermit             mute permission
 *
 *     Output:    pswPPFExcit[]            muted excitation signal
 *                pswSynthFiltState[]      muted state of LPC synthesis filter
 *                pswLtpStateOut[]         muted state of long term predictor
 *                pswPPreState[]           muted state of pitch prefilter
 *
 *     Constants: psrConceal[0:15]         muting factors
 *                psrLevelMaxIncrease[0:7] maximum allowed level increase
 *
 *
 ****************************************************************************/

void Codec::signal_conceal_sub(int16_t pswPPFExcit[],
                     int16_t ppswSynthAs[], int16_t pswSynthFiltState[],
                       int16_t pswLtpStateOut[], int16_t pswPPreState[],
                                 int16_t swLevelMean, int16_t swLevelMax,
                                 int16_t swUFI, int16_t swMuteFlagOld,
                             int16_t *pswMuteFlag, int16_t swMutePermit)
{

/*_________________________________________________________________________
 |                                                                         |
 |                           Local Static Variables                        |
 |_________________________________________________________________________|
*/
  static const int16_t psrConceal[15] = {29205, 27571, 24573, 21900,
  19519, 17396, 15504, 13818, 12315, 10976, 9783, 8719, 7771, 6925, 6172};
  static const int16_t psrLevelMaxIncrease[16] =
  {0, 0, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16};

/*_________________________________________________________________________
 |                                                                         |
 |                            Automatic Variables                          |
 |_________________________________________________________________________|
*/
  int16_t swMute,
         swLevelSub,
         i,
         swIndex;
  int16_t swTmp,
         pswStateTmp[10],
         swOutTmp[40],
         swPermitMuteSub;
  int32_t L_sum;


/*_________________________________________________________________________
 |                                                                         |
 |                              Executable Code                            |
 |_________________________________________________________________________|
*/

  /* Test synthesis filter: */
  /* ---------------------- */
  for (i = 0; i < 10; ++i)
    pswStateTmp[i] = pswSynthFiltState[i];

  lpcIir(pswPPFExcit, ppswSynthAs, pswStateTmp, swOutTmp);


  /* Determine level in db of synthesized signal: */
  /* -------------------------------------------- */
  L_sum = 0;
  for (i = 0; i < S_LEN; ++i)
  {
    swTmp = shr(swOutTmp[i], 2);
    L_sum = L_mac(L_sum, swTmp, swTmp);
  }
  swLevelSub = level_calc(0, &L_sum);


  /* Determine index to table, specifying the allowed level increase: */
  /* level [ 0 ..  -5] --> swIndex = 0       */
  /* level [-5 .. -10] --> swIndex = 1  etc. */
  /*---------------------------------------------*/
  swIndex = mult(negate(swLevelMean), 1638);
  if (sub(swIndex, 15) > 0)
    swIndex = 15;

  /* Muting is permitted, if it is signalled from the parameter concealment */
  /* unit or if muting has been performed in the last frame                */
  /*-----------------------------------------------------------------------*/
  swPermitMuteSub = swMutePermit;
  if (swMuteFlagOld > 0)
    swPermitMuteSub = 1;

  if (swPermitMuteSub > 0)
  {
    /* Muting is not permitted if the sub-frame level is less than */
    /* MIN_MUTE_LEVEL                                              */
    /* ------------------------------------------------------------ */
    if (sub(swLevelSub, MIN_MUTE_LEVEL) <= 0)
      swPermitMuteSub = 0;

    /* Muting is not permitted if the sub-frame level is less than */
    /* the maximum level of the last 4 sub-frames plus the allowed */
    /* increase                                                    */
    /* ------------------------------------------------------------ */
    swMute = sub(swLevelSub, add(swLevelMax, psrLevelMaxIncrease[swIndex]));
    if (swMute <= 0)
      swPermitMuteSub = 0;
  }


  /* Perform muting, if allowed */
  /* -------------------------- */
  if (swPermitMuteSub > 0)
  {

    if (sub(swMute, (int16_t) 15) > 0)
      swMute = 15;

    /* Keep information that muting occured for next frame */
    /* --------------------------------------------------- */
    if (swUFI > 0)
      *pswMuteFlag = 1;


    /* Mute excitation signal: */
    /* ----------------------- */
    for (i = 0; i < 10; ++i)
      pswSynthFiltState[i] =
              mult_r(pswSynthFiltState[i], psrConceal[swMute - 1]);
    for (i = 0; i < S_LEN; ++i)
      pswPPFExcit[i] = mult_r(pswPPFExcit[i], psrConceal[swMute - 1]);

    /* Mute pitch memory: */
    /* ------------------ */
    for (i = 0; i < S_LEN; ++i)
      pswLtpStateOut[i] =
              mult_r(pswLtpStateOut[i], psrConceal[swMute - 1]);


    /* Mute pitch prefilter memory: */
    /* ---------------------------- */
    for (i = 0; i < S_LEN; ++i)
      pswPPreState[i] = mult_r(pswPPreState[i], psrConceal[swMute - 1]);
  }
}


/****************************************************************************
 *
 *   FUNCTION NAME: level_calc
 *
 *     This subroutine calculates the level (db) from the energy
 *     of a speech sub-frame (swInd=0) or a speech frame (swInd=1):
 *     The level of a speech subframe is:
 *       swLevel =  10 * lg(EN/(40.*4096*4096))
 *               =   3 * ld(EN) - 88.27
 *               = (3*4*ld(EN) - 353)/4
 *               = (3*(4*POS(MSB(EN)) + 2*BIT(MSB-1) + BIT(MSB-2)) - 353)/4
 *
 *     Input:     pl_en      energy of the speech subframe or frame
 *                           The energy is multiplied by 2 because of the
 *                           MAC routines !!
 *                swInd      = 0: EN is the energy of one subframe
 *                           = 1: EN is the energy of one frame
 *
 *     Output:    swLevel    level in db
 *
 *
 ***************************************************************************/

int16_t Codec::level_calc(int16_t swInd, int32_t *pl_en)
{

/*_________________________________________________________________________
 |                                                                         |
 |                            Automatic Variables                          |
 |_________________________________________________________________________|
*/
  int16_t swPos,
         swLevel;
  int32_t L_tmp;

/*_________________________________________________________________________
 |                                                                         |
 |                              Executable Code                            |
 |_________________________________________________________________________|
*/

  if (*pl_en != 0)
    swPos = sub((int16_t) 29, norm_l(*pl_en));
  else
    swPos = 0;

  /* Determine the term: 4*POS(MSB(EN)): */
  /* ----------------------------------- */
  swLevel = shl(swPos, 2);

  /* Determine the term: 2*BIT(MSB-1): */
  /* --------------------------------- */
  if (swPos >= 0)
  {
    L_tmp = L_shl((int32_t) 1, swPos);
    if ((*pl_en & L_tmp) != 0)
      swLevel += 2;
  }

  /* Determine the term: BIT(MSB-2): */
  /* ------------------------------- */
  if (--swPos >= 0)
  {
    L_tmp = L_shl((int32_t) 1, swPos);
    if ((*pl_en & L_tmp) != 0)
      ++swLevel;
  }

  /* Multiply by 3: */
  /* -------------- */
  swLevel += shl(swLevel, 1);
  swLevel -= (swInd == 0) ? 353 : 377;
  swLevel = mult_r(swLevel, 0X2000);   /* >> 2 */

  if (sub(swLevel, -72) < 0)
  {
    swLevel = -72;
    *pl_en = (swInd == 0) ? 80 : 320;
  }

  return (swLevel);
}

// From sp_dec.c
  static short aToRc(int16_t swAshift, int16_t pswAin[],
                            int16_t pswRc[]);


  static void lookupVq(int16_t pswVqCodeWds[], int16_t pswRCOut[]);


#define  P_INT_MACS   10
#define  ASCALE       0x0800
#define  ASHIFT       4
#define  DELTA_LEVELS 16
#define  GSP0_SCALE   1
#define  C_BITS_V     9                /* number of bits in any voiced VSELP
                                        * codeword */
#define  C_BITS_UV    7                /* number of bits in a unvoiced VSELP
                                        * codeword */
#define  MAXBITS      C_BITS_V         /* max number of bits in any VSELP
                                        * codeword */
#define  SQRT_ONEHALF 0x5a82           /* the 0.5 ** 0.5 */
#define  LPC_ROUND    0x00000800L      /* 0x8000 >> ASHIFT */
#define  AFSHIFT      2                /* number of right shifts to be
                                        * applied to the autocorrelation
                                        * sequence in aFlatRcDp     */

  void Codec::aFlatRcDp(int32_t *pL_R, int16_t *pswRc)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t pL_pjNewSpace[NP];
    int32_t pL_pjOldSpace[NP];
    int32_t pL_vjNewSpace[2 * NP - 1];
    int32_t pL_vjOldSpace[2 * NP - 1];

    int32_t *pL_pjOld;
    int32_t *pL_pjNew;
    int32_t *pL_vjOld;
    int32_t *pL_vjNew;
    int32_t *pL_swap;

    int32_t L_temp;
    int32_t L_sum;
    int16_t swRc,
           swRcSq,
           swTemp,
           swTemp1,
           swAbsTemp1,
           swTemp2;
    int    i,
           j;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    pL_pjOld = pL_pjOldSpace;
    pL_pjNew = pL_pjNewSpace;
    pL_vjOld = pL_vjOldSpace + NP - 1;
    pL_vjNew = pL_vjNewSpace + NP - 1;


    /* Extract the 0-th reflection coefficient */
    /*-----------------------------------------*/

    swTemp1 = round(pL_R[1]);
    swTemp2 = round(pL_R[0]);
    swAbsTemp1 = abs_s(swTemp1);
    if (swTemp2 <= 0 || sub(swAbsTemp1, swTemp2) >= 0)
    {
      j = 0;
      for (i = j; i < NP; i++)
      {
        pswRc[i] = 0;
      }
      return;
    }

    swRc = divide_s(swAbsTemp1, swTemp2);/* return division result */

    if (sub(swTemp1, swAbsTemp1) == 0)
      swRc = negate(swRc);               /* negate reflection Rc[j] */

    pswRc[0] = swRc;                     /* copy into the output Rc array */

    for (i = 0; i <= NP; i++)
    {
      pL_R[i] = L_shr(pL_R[i], AFSHIFT);
    }

    /* Initialize the pjOld and vjOld recursion arrays */
    /*-------------------------------------------------*/

    for (i = 0; i < NP; i++)
    {
      pL_pjOld[i] = pL_R[i];
      pL_vjOld[i] = pL_R[i + 1];
    }
    for (i = -1; i > -NP; i--)
      pL_vjOld[i] = pL_R[-(i + 1)];


    /* Compute the square of the j=0 reflection coefficient */
    /*------------------------------------------------------*/

    swRcSq = mult_r(swRc, swRc);

    /* Update pjNew and vjNew arrays for lattice stage j=1 */
    /*-----------------------------------------------------*/

    /* Updating pjNew: */
    /*-------------------*/

    for (i = 0; i <= NP - 2; i++)
    {
      L_temp = L_mpy_ls(pL_vjOld[i], swRc);
      L_sum = L_add(L_temp, pL_pjOld[i]);
      L_temp = L_mpy_ls(pL_pjOld[i], swRcSq);
      L_sum = L_add(L_temp, L_sum);
      L_temp = L_mpy_ls(pL_vjOld[-i], swRc);
      pL_pjNew[i] = L_add(L_sum, L_temp);
    }

    /* Updating vjNew: */
    /*-------------------*/

    for (i = -NP + 2; i <= NP - 2; i++)
    {
      L_temp = L_mpy_ls(pL_vjOld[-i - 1], swRcSq);
      L_sum = L_add(L_temp, pL_vjOld[i + 1]);
      L_temp = L_mpy_ls(pL_pjOld[(((i + 1) >= 0) ? i + 1 : -(i + 1))], swRc);
      L_temp = L_shl(L_temp, 1);
      pL_vjNew[i] = L_add(L_temp, L_sum);
    }



    j = 0;

    /* Compute reflection coefficients Rc[1],...,Rc[9] */
    /*-------------------------------------------------*/

    for (j = 1; j < NP; j++)
    {

      /* Swap pjNew and pjOld buffers */
      /*------------------------------*/

      pL_swap = pL_pjNew;
      pL_pjNew = pL_pjOld;
      pL_pjOld = pL_swap;

      /* Swap vjNew and vjOld buffers */
      /*------------------------------*/

      pL_swap = pL_vjNew;
      pL_vjNew = pL_vjOld;
      pL_vjOld = pL_swap;

      /* Compute the j-th reflection coefficient */
      /*-----------------------------------------*/

      swTemp = norm_l(pL_pjOld[0]);      /* get shift count */
      swTemp1 = round(L_shl(pL_vjOld[0], swTemp));        /* normalize num.  */
      swTemp2 = round(L_shl(pL_pjOld[0], swTemp));        /* normalize den.  */

      /* Test for invalid divide conditions: a) devisor < 0 b) abs(divident) >
       * abs(devisor) If either of these conditions is true, zero out
       * reflection coefficients for i=j,...,NP-1 and return. */

      swAbsTemp1 = abs_s(swTemp1);
      if (swTemp2 <= 0 || sub(swAbsTemp1, swTemp2) >= 0)
      {
        i = j;
        for (i = j; i < NP; i++)
        {
          pswRc[i] = 0;
        }
        return;
      }

      swRc = divide_s(swAbsTemp1, swTemp2);       /* return division result */
      if (sub(swTemp1, swAbsTemp1) == 0)
        swRc = negate(swRc);             /* negate reflection Rc[j] */
      swRcSq = mult_r(swRc, swRc);       /* compute Rc^2 */
      pswRc[j] = swRc;                   /* copy Rc[j] to output array */

      /* Update pjNew and vjNew arrays for the next lattice stage if j < NP-1 */
      /*---------------------------------------------------------------------*/

      /* Updating pjNew: */
      /*-----------------*/

      for (i = 0; i <= NP - j - 2; i++)
      {
        L_temp = L_mpy_ls(pL_vjOld[i], swRc);
        L_sum = L_add(L_temp, pL_pjOld[i]);
        L_temp = L_mpy_ls(pL_pjOld[i], swRcSq);
        L_sum = L_add(L_temp, L_sum);
        L_temp = L_mpy_ls(pL_vjOld[-i], swRc);
        pL_pjNew[i] = L_add(L_sum, L_temp);
      }

      /* Updating vjNew: */
      /*-----------------*/

      for (i = -NP + j + 2; i <= NP - j - 2; i++)
      {
        L_temp = L_mpy_ls(pL_vjOld[-i - 1], swRcSq);
        L_sum = L_add(L_temp, pL_vjOld[i + 1]);
        L_temp = L_mpy_ls(pL_pjOld[(((i + 1) >= 0) ? i + 1 : -(i + 1))], swRc);
        L_temp = L_shl(L_temp, 1);
        pL_vjNew[i] = L_add(L_temp, L_sum);
      }
    }
    return;
  }


  static short aToRc(int16_t swAshift, int16_t pswAin[],
                            int16_t pswRc[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Constants                                    |
   |_________________________________________________________________________|
  */

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t pswTmpSpace[NP],
           pswASpace[NP],
           swNormShift,
           swActShift,
           swNormProd,
           swRcOverE,
           swDiv,
          *pswSwap,
          *pswTmp,
          *pswA;

    int32_t L_temp;

    short int siUnstableFlt,
           i,
           j;                            /* Loop control variables */

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Initialize starting addresses for temporary buffers */
    /*-----------------------------------------------------*/

    pswA = pswASpace;
    pswTmp = pswTmpSpace;

    /* Copy the direct form filter coefficients to a temporary array */
    /*---------------------------------------------------------------*/

    for (i = 0; i < NP; i++)
    {
      pswA[i] = pswAin[i];
    }

    /* Initialize the flag for filter stability check */
    /*------------------------------------------------*/

    siUnstableFlt = 0;

    /* Start computation of the reflection coefficients, Rc[9],...,Rc[1] */
    /*-------------------------------------------------------------------*/

    for (i = NP - 1; i >= 1; i--)
    {

      pswRc[i] = shl(pswA[i], swAshift); /* write Rc[i] to output array */

      /* Check the stability of i-th reflection coefficient */
      /*----------------------------------------------------*/

      siUnstableFlt = siUnstableFlt | isSwLimit(pswRc[i]);

      /* Precompute intermediate variables for needed for the computation */
      /* of direct form filter of order i-1                               */
      /*------------------------------------------------------------------*/

      if (sub(pswRc[i], SW_MIN) == 0)
      {
        siUnstableFlt = 1;
        swRcOverE = 0;
        swDiv = 0;
        swActShift = 2;
      }
      else
      {
        L_temp = LW_MAX;                 /* Load ~1.0 into accum */
        L_temp = L_msu(L_temp, pswRc[i], pswRc[i]);       /* 1.-Rc[i]*Rc[i]  */
        swNormShift = norm_l(L_temp);
        L_temp = L_shl(L_temp, swNormShift);
        swNormProd = extract_h(L_temp);
        swActShift = add(2, swNormShift);
        swDiv = divide_s(0x2000, swNormProd);
        swRcOverE = mult_r(pswRc[i], swDiv);
      }
      /* Check stability   */
      /*---------------------*/
      siUnstableFlt = siUnstableFlt | isSwLimit(swRcOverE);

      /* Compute direct form filter coefficients corresponding to */
      /* a direct form filter of order i-1                        */
      /*----------------------------------------------------------*/

      for (j = 0; j <= i - 1; j++)
      {
        L_temp = L_mult(pswA[j], swDiv);
        L_temp = L_msu(L_temp, pswA[i - j - 1], swRcOverE);
        L_temp = L_shl(L_temp, swActShift);
        pswTmp[j] = round(L_temp);
        siUnstableFlt = siUnstableFlt | isSwLimit(pswTmp[j]);
      }

      /* Swap swA and swTmp buffers */
      /*----------------------------*/

      pswSwap = pswA;
      pswA = pswTmp;
      pswTmp = pswSwap;
    }

    /* Compute reflection coefficient Rc[0] */
    /*--------------------------------------*/

    pswRc[0] = shl(pswA[0], swAshift);   /* write Rc[0] to output array */

    /* Check the stability of 0-th reflection coefficient */
    /*----------------------------------------------------*/

    siUnstableFlt = siUnstableFlt | isSwLimit(pswRc[0]);

    return (siUnstableFlt);
  }

  void Codec::a_sst(int16_t swAshift, int16_t swAscale,
                           int16_t pswDirectFormCoefIn[],
                           int16_t pswDirectFormCoefOut[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                           Local Static Variables                        |
   |_________________________________________________________________________|
  */

    static int16_tRom psrSST[NP + 1] = {0x7FFF,
      0x7F5C, 0x7D76, 0x7A5B, 0x7622, 0x70EC,
      0x6ADD, 0x641F, 0x5CDD, 0x5546, 0x4D86,
    };

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t pL_CorrTemp[NP + 1];

    int16_t pswRCNum[NP],
           pswRCDenom[NP];

    short int siLoopCnt;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* convert direct form coefs to reflection coefs */
    /* --------------------------------------------- */

    aToRc(swAshift, pswDirectFormCoefIn, pswRCDenom);

    /* convert to autocorrelation coefficients */
    /* --------------------------------------- */

    rcToCorrDpL(swAshift, swAscale, pswRCDenom, pL_CorrTemp);

    /* do spectral smoothing technique */
    /* ------------------------------- */

    for (siLoopCnt = 1; siLoopCnt <= NP; siLoopCnt++)
    {
      pL_CorrTemp[siLoopCnt] = L_mpy_ls(pL_CorrTemp[siLoopCnt],
                                        psrSST[siLoopCnt]);
    }

    /* Compute the reflection coefficients via AFLAT */
    /*-----------------------------------------------*/

    aFlatRcDp(pL_CorrTemp, pswRCNum);


    /* Convert reflection coefficients to direct form filter coefficients */
    /*-------------------------------------------------------------------*/

    rcToADp(swAscale, pswRCNum, pswDirectFormCoefOut);
  }


  int16_t Codec::agcGain(int16_t pswStateCurr[],
                          struct NormSw snsInSigEnergy, int16_t swEngyRShft)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_OutEnergy,
           L_AgcGain;

    struct NormSw snsOutEnergy,
           snsAgc;

    int16_t swAgcOut,
           swAgcShftCnt;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Calculate the energy in the output vector divided by 2 */
    /*--------------------------------------------------------*/

    snsOutEnergy.sh = g_corr1s(pswStateCurr, swEngyRShft, &L_OutEnergy);

    /* reduce energy by a factor of 2 */
    snsOutEnergy.sh = add(snsOutEnergy.sh, 1);

    /* if waveform has nonzero energy, find AGC gain */
    /*-----------------------------------------------*/

    if (L_OutEnergy == 0)
    {
      swAgcOut = 0;
    }
    else
    {

      snsOutEnergy.man = round(L_OutEnergy);

      /* divide input energy by 2 */
      snsInSigEnergy.man = shr(snsInSigEnergy.man, 1);


      /* Calculate AGC gain squared */
      /*----------------------------*/

      snsAgc.man = divide_s(snsInSigEnergy.man, snsOutEnergy.man);
      swAgcShftCnt = norm_s(snsAgc.man);
      snsAgc.man = shl(snsAgc.man, swAgcShftCnt);

      /* find shift count for G^2 */
      /*--------------------------*/

      snsAgc.sh = add(sub(snsInSigEnergy.sh, snsOutEnergy.sh),
                      swAgcShftCnt);
      L_AgcGain = L_deposit_h(snsAgc.man);


      /* Calculate AGC gain */
      /*--------------------*/

      snsAgc.man = sqroot(L_AgcGain);


      /* check if 1/2 sqrt(G^2) >= 1.0                      */
      /* This is equivalent to checking if shiftCnt/2+1 < 0 */
      /*----------------------------------------------------*/

      if (add(snsAgc.sh, 2) < 0)
      {
        swAgcOut = SW_MAX;
      }
      else
      {

        if (0x1 & snsAgc.sh)
        {
          snsAgc.man = mult(snsAgc.man, SQRT_ONEHALF);
        }

        snsAgc.sh = shr(snsAgc.sh, 1);   /* shiftCnt/2 */
        snsAgc.sh = add(snsAgc.sh, 1);   /* shiftCnt/2 + 1 */

        if (snsAgc.sh > 0)
        {
          snsAgc.man = shr(snsAgc.man, snsAgc.sh);
        }
        swAgcOut = snsAgc.man;
      }
    }

    return (swAgcOut);
  }

  void Codec::b_con(int16_t swCodeWord, short siNumBits,
                      int16_t pswVectOut[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    short int siLoopCnt;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    for (siLoopCnt = 0; siLoopCnt < siNumBits; siLoopCnt++)
    {

      if (swCodeWord & 1)                /* temp accumulator get 0.5 */
        pswVectOut[siLoopCnt] = (int16_t) 0x4000;
      else                               /* temp accumulator gets -0.5 */
        pswVectOut[siLoopCnt] = (int16_t) 0xc000;

      swCodeWord = shr(swCodeWord, 1);
    }
  }

  void Codec::fp_ex(int16_t swOrigLagIn,
                      int16_t pswLTPState[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Temp;
    int16_t swIntLag,
           swRemain,
           swRunningLag;
    short int siSampsSoFar,
           siSampsThisPass,
           i,
           j;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Loop: execute until all samples in the vector have been looked up */
    /*-------------------------------------------------------------------*/

    swRunningLag = swOrigLagIn;
    siSampsSoFar = 0;
    while (siSampsSoFar < S_LEN)
    {

      /* Get integer lag and remainder.  These are used in addressing */
      /* the LTP state and the interpolating filter, respectively     */
      /*--------------------------------------------------------------*/

      get_ipjj(swRunningLag, &swIntLag, &swRemain);


      /* Get the number of samples to look up in this pass */
      /*---------------------------------------------------*/

      if (sub(swIntLag, S_LEN) < 0)
        siSampsThisPass = swIntLag - siSampsSoFar;
      else
        siSampsThisPass = S_LEN - siSampsSoFar;

      /* Look up samples by interpolating (fractional lag), or copying */
      /* (integer lag).                                                */
      /*---------------------------------------------------------------*/

      if (swRemain == 0)
      {

        /* Integer lag: copy samples from history */
        /*----------------------------------------*/

        for (i = siSampsSoFar; i < siSampsSoFar + siSampsThisPass; i++)
          pswLTPState[i] = pswLTPState[i - swIntLag];
      }
      else
      {

        /* Fractional lag: interpolate to get samples */
        /*--------------------------------------------*/

        for (i = siSampsSoFar; i < siSampsSoFar + siSampsThisPass; i++)
        {

          /* first tap with rounding offset */
          /*--------------------------------*/
          L_Temp = L_mac((long) 32768,
                         pswLTPState[i - swIntLag - P_INT_MACS / 2],
                         ppsrPVecIntFilt[0][swRemain]);

          for (j = 1; j < P_INT_MACS - 1; j++)
          {

            L_Temp = L_mac(L_Temp,
                           pswLTPState[i - swIntLag - P_INT_MACS / 2 + j],
                           ppsrPVecIntFilt[j][swRemain]);

          }

          pswLTPState[i] = extract_h(L_mac(L_Temp,
                               pswLTPState[i - swIntLag + P_INT_MACS / 2 - 1],
                                  ppsrPVecIntFilt[P_INT_MACS - 1][swRemain]));
        }
      }

      /* Done with this pass: update loop controls */
      /*-------------------------------------------*/

      siSampsSoFar += siSampsThisPass;
      swRunningLag = add(swRunningLag, swOrigLagIn);
    }
  }


  int16_t Codec::g_corr1(int16_t *pswIn, int32_t *pL_out)
  {


  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_sum;
    int16_t swEngyLShft;
    int    i;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */


    /* Calculate energy in subframe vector (40 samples) */
    /*--------------------------------------------------*/

    L_sum = L_mult(pswIn[0], pswIn[0]);
    for (i = 1; i < S_LEN; i++)
    {
      L_sum = L_mac(L_sum, pswIn[i], pswIn[i]);
    }



    if (L_sum != 0)
    {

      /* Normalize the energy in the output int32_t */
      /*---------------------------------------------*/

      swEngyLShft = norm_l(L_sum);
      *pL_out = L_shl(L_sum, swEngyLShft);        /* normalize output
                                                   * int32_t */
    }
    else
    {

      /* Special case: energy is zero */
      /*------------------------------*/

      *pL_out = L_sum;
      swEngyLShft = 0;
    }

    return (swEngyLShft);
  }


  int16_t Codec::g_corr1s(int16_t pswIn[], int16_t swEngyRShft,
                            int32_t *pL_out)
  {


  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_sum;
    int16_t swTemp,
           swEngyLShft;
    int16_t swInputRShft;

    int    i;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */


    /* Calculate energy in subframe vector (40 samples) */
    /*--------------------------------------------------*/

    if (sub(swEngyRShft, 1) <= 0)
    {

      /* use the energy shift factor, although it is an odd shift count */
      /*----------------------------------------------------------------*/

      swTemp = shr(pswIn[0], swEngyRShft);
      L_sum = L_mult(pswIn[0], swTemp);
      for (i = 1; i < S_LEN; i++)
      {
        swTemp = shr(pswIn[i], swEngyRShft);
        L_sum = L_mac(L_sum, pswIn[i], swTemp);
      }

    }
    else
    {

      /* convert energy shift factor to an input shift factor */
      /*------------------------------------------------------*/

      swInputRShft = shift_r(swEngyRShft, -1);
      swEngyRShft = shl(swInputRShft, 1);

      swTemp = shr(pswIn[0], swInputRShft);
      L_sum = L_mult(swTemp, swTemp);
      for (i = 1; i < S_LEN; i++)
      {
        swTemp = shr(pswIn[i], swInputRShft);
        L_sum = L_mac(L_sum, swTemp, swTemp);
      }
    }

    if (L_sum != 0)
    {

      /* Normalize the energy in the output int32_t */
      /*---------------------------------------------*/

      swTemp = norm_l(L_sum);
      *pL_out = L_shl(L_sum, swTemp);    /* normalize output int32_t */
      swEngyLShft = sub(swTemp, swEngyRShft);
    }
    else
    {

      /* Special case: energy is zero */
      /*------------------------------*/

      *pL_out = L_sum;
      swEngyLShft = 0;
    }

    return (swEngyLShft);
  }

  void Codec::getSfrmLpc(short int siSoftInterpolation,
                           int16_t swPrevR0, int16_t swNewR0,
            /* last frm */ int16_t pswPrevFrmKs[], int16_t pswPrevFrmAs[],
                           int16_t pswPrevFrmPFNum[],
                           int16_t pswPrevFrmPFDenom[],

              /* this frm */ int16_t pswNewFrmKs[], int16_t pswNewFrmAs[],
                           int16_t pswNewFrmPFNum[],
                           int16_t pswNewFrmPFDenom[],

                     /* output */ struct NormSw *psnsSqrtRs,
                           int16_t *ppswSynthAs[], int16_t *ppswPFNumAs[],
                           int16_t *ppswPFDenomAs[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                           Local Static Variables                        |
   |_________________________________________________________________________|
  */


  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    short int siSfrm,
           siStable,
           i;

    int32_t L_Temp1,
           L_Temp2;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    if (siSoftInterpolation)
    {
      /* yes, interpolating */
      /* ------------------ */

      siSfrm = 0;

      siStable = interpolateCheck(pswPrevFrmKs, pswPrevFrmAs,
                                  pswPrevFrmAs, pswNewFrmAs,
                                  psrOldCont[siSfrm], psrNewCont[siSfrm],
                                  swPrevR0,
                                  &psnsSqrtRs[siSfrm],
                                  ppswSynthAs[siSfrm]);
      if (siStable)
      {

        /* interpolate between direct form coefficient sets */
        /* for both numerator and denominator coefficients  */
        /* assume output will be stable                     */
        /* ------------------------------------------------ */

        for (i = 0; i < NP; i++)
        {
          L_Temp1 = L_mult(pswNewFrmPFNum[i], psrNewCont[siSfrm]);
          ppswPFNumAs[siSfrm][i] = mac_r(L_Temp1, pswPrevFrmPFNum[i],
                                         psrOldCont[siSfrm]);
          L_Temp2 = L_mult(pswNewFrmPFDenom[i], psrNewCont[siSfrm]);
          ppswPFDenomAs[siSfrm][i] = mac_r(L_Temp2, pswPrevFrmPFDenom[i],
                                           psrOldCont[siSfrm]);
        }
      }
      else
      {
        /* this subframe is unstable */
        /* ------------------------- */
        for (i = 0; i < NP; i++)
        {
          ppswPFNumAs[siSfrm][i] = pswPrevFrmPFNum[i];
          ppswPFDenomAs[siSfrm][i] = pswPrevFrmPFDenom[i];
        }
      }
      for (siSfrm = 1; siSfrm < N_SUB - 1; siSfrm++)
      {

        siStable = interpolateCheck(pswNewFrmKs, pswNewFrmAs,
                                    pswPrevFrmAs, pswNewFrmAs,
                                    psrOldCont[siSfrm], psrNewCont[siSfrm],
                                    swNewR0,
                                    &psnsSqrtRs[siSfrm],
                                    ppswSynthAs[siSfrm]);
        if (siStable)
        {

          /* interpolate between direct form coefficient sets */
          /* for both numerator and denominator coefficients  */
          /* assume output will be stable                     */
          /* ------------------------------------------------ */

          for (i = 0; i < NP; i++)
          {
            L_Temp1 = L_mult(pswNewFrmPFNum[i], psrNewCont[siSfrm]);
            ppswPFNumAs[siSfrm][i] = mac_r(L_Temp1, pswPrevFrmPFNum[i],
                                           psrOldCont[siSfrm]);
            L_Temp2 = L_mult(pswNewFrmPFDenom[i], psrNewCont[siSfrm]);
            ppswPFDenomAs[siSfrm][i] = mac_r(L_Temp2, pswPrevFrmPFDenom[i],
                                             psrOldCont[siSfrm]);
          }
        }
        else
        {
          /* this subframe has unstable filter coeffs, would like to
           * interpolate but can not  */
          /* -------------------------------------- */
          for (i = 0; i < NP; i++)
          {
            ppswPFNumAs[siSfrm][i] = pswNewFrmPFNum[i];
            ppswPFDenomAs[siSfrm][i] = pswNewFrmPFDenom[i];
          }
        }
      }
      /* the last subframe never interpolate */
      /* ----------------------------------- */
      siSfrm = 3;
      for (i = 0; i < NP; i++)
      {
        ppswPFNumAs[siSfrm][i] = pswNewFrmPFNum[i];
        ppswPFDenomAs[siSfrm][i] = pswNewFrmPFDenom[i];
        ppswSynthAs[siSfrm][i] = pswNewFrmAs[i];
      }

      res_eng(pswNewFrmKs, swNewR0, &psnsSqrtRs[siSfrm]);

    }
    /* SoftInterpolation == 0  - no interpolation */
    /* ------------------------------------------ */
    else
    {
      siSfrm = 0;
      for (i = 0; i < NP; i++)
      {
        ppswPFNumAs[siSfrm][i] = pswPrevFrmPFNum[i];
        ppswPFDenomAs[siSfrm][i] = pswPrevFrmPFDenom[i];
        ppswSynthAs[siSfrm][i] = pswPrevFrmAs[i];
      }

      res_eng(pswPrevFrmKs, swPrevR0, &psnsSqrtRs[siSfrm]);

      /* for subframe 1 and all subsequent sfrms, use result from new frm */
      /* ---------------------------------------------------------------- */


      res_eng(pswNewFrmKs, swNewR0, &psnsSqrtRs[1]);

      for (siSfrm = 1; siSfrm < N_SUB; siSfrm++)
      {


        psnsSqrtRs[siSfrm].man = psnsSqrtRs[1].man;
        psnsSqrtRs[siSfrm].sh = psnsSqrtRs[1].sh;

        for (i = 0; i < NP; i++)
        {
          ppswPFNumAs[siSfrm][i] = pswNewFrmPFNum[i];
          ppswPFDenomAs[siSfrm][i] = pswNewFrmPFDenom[i];
          ppswSynthAs[siSfrm][i] = pswNewFrmAs[i];
        }
      }
    }
  }

  void Codec::get_ipjj(int16_t swLagIn,
                         int16_t *pswIp, int16_t *pswJj)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                              Local Constants                            |
   |_________________________________________________________________________|
  */

  #define  OS_FCTR_INV  (int16_t)0x1555/* SW_MAX/OS_FCTR */

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Temp;

    int16_t swTemp,
           swTempIp,
           swTempJj;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* calculate ip */
    /* ------------ */

    L_Temp = L_mult(OS_FCTR_INV, swLagIn);        /* lag/OS_FCTR */
    swTempIp = extract_h(L_Temp);

    /* calculate jj */
    /* ------------ */

    swTemp = extract_l(L_Temp);          /* loose ip */
    swTemp = shr(swTemp, 1);             /* isolate jj fraction */
    swTemp = swTemp & SW_MAX;
    L_Temp = L_mult(swTemp, OS_FCTR);    /* ((lag/OS_FCTR)-ip))*(OS_FCTR) */
    swTemp = round(L_Temp);              /* round and pick-off jj */
    if (sub(swTemp, OS_FCTR) == 0)
    {                                    /* if 'overflow ' */
      swTempJj = 0;                      /* set remainder,jj to 0 */
      swTempIp = add(swTempIp, 1);       /* 'carry' overflow into ip */
    }
    else
    {
      swTempJj = swTemp;                 /* read-off remainder,jj */
    }

    /* return ip and jj */
    /* ---------------- */

    *pswIp = swTempIp;
    *pswJj = swTempJj;
  }

  short int Codec::interpolateCheck(int16_t pswRefKs[],
                                    int16_t pswRefCoefsA[],
                           int16_t pswOldCoefsA[], int16_t pswNewCoefsA[],
                                    int16_t swOldPer, int16_t swNewPer,
                                    int16_t swRq,
                                    struct NormSw *psnsSqrtRsOut,
                                    int16_t pswCoefOutA[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t pswRcTemp[NP];

    int32_t L_Temp;

    short int siInterp_flg,
           i;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Interpolation loop, NP is order of LPC filter */
    /* --------------------------------------------- */

    for (i = 0; i < NP; i++)
    {
      L_Temp = L_mult(pswNewCoefsA[i], swNewPer);
      pswCoefOutA[i] = mac_r(L_Temp, pswOldCoefsA[i], swOldPer);
    }

    /* Convert to reflection coefficients and check stability */
    /* ------------------------------------------------------ */

    if (aToRc(ASHIFT, pswCoefOutA, pswRcTemp) != 0)
    {

      /* Unstable, use uninterpolated parameters and compute RS update the
       * state with the frame data closest to this subfrm */
      /* --------------------------------------------------------- */

      res_eng(pswRefKs, swRq, psnsSqrtRsOut);

      for (i = 0; i < NP; i++)
      {
        pswCoefOutA[i] = pswRefCoefsA[i];
      }
      siInterp_flg = 0;
    }
    else
    {

      /* Stable, compute RS */
      /* ------------------ */
      res_eng(pswRcTemp, swRq, psnsSqrtRsOut);

      /* Set temporary subframe interpolation flag */
      /* ----------------------------------------- */
      siInterp_flg = 1;
    }

    /* Return subframe interpolation flag */
    /* ---------------------------------- */
    return (siInterp_flg);
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: lagDecode
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to decode the lag received from the
   *     speech encoder into a full resolution lag for the speech decoder
   *
   *   INPUTS:
   *
   *     swDeltaLag
   *
   *                     lag received from channel decoder
   *
   *     giSfrmCnt
   *
   *                     current sub-frame count
   *
   *     swLastLag
   *
   *                     previous lag to un-delta this sub-frame's lag
   *
   *     psrLagTbl[0:255]
   *
   *                     table used to look up full resolution lag
   *
   *   OUTPUTS:
   *
   *     swLastLag
   *
   *                     new previous lag for next sub-frame
   *
   *   RETURN VALUE:
   *
   *     swLag
   *
   *                     decoded full resolution lag
   *
   *   DESCRIPTION:
   *
   *     If first subframe, use lag as index to look up table directly.
   *
   *     If it is one of the other subframes, the codeword represents a
   *     delta offset.  The previously decoded lag is used as a starting
   *     point for decoding the current lag.
   *
   *   REFERENCES: Sub-clause 4.2.1 of GSM Recomendation 06.20
   *
   *   KEYWORDS: deltalags, lookup lag
   *
   *************************************************************************/

  int16_t Codec::lagDecode(int16_t swDeltaLag)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                              Local Constants                            |
   |_________________________________________________________________________|
  */

  #define  DELTA_LEVELS_D2  DELTA_LEVELS/2
  #define  MAX_LAG          0x00ff
  #define  MIN_LAG          0x0000

  /*_________________________________________________________________________
   |                                                                         |
   |                           Local Static Variables                        |
   |_________________________________________________________________________|
  */



  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swLag;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* first sub-frame */
    /* --------------- */

    if (giSfrmCnt == 0)
    {
      swLastLag = swDeltaLag;
    }

    /* remaining sub-frames */
    /* -------------------- */

    else
    {

      /* get lag biased around 0 */
      /* ----------------------- */

      swLag = sub(swDeltaLag, DELTA_LEVELS_D2);

      /* get real lag relative to last */
      /* ----------------------------- */

      swLag = add(swLag, swLastLag);

      /* clip to max or min */
      /* ------------------ */

      if (sub(swLag, MAX_LAG) > 0)
      {
        swLastLag = MAX_LAG;
      }
      else if (sub(swLag, MIN_LAG) < 0)
      {
        swLastLag = MIN_LAG;
      }
      else
      {
        swLastLag = swLag;
      }
    }

    /* return lag after look up */
    /* ------------------------ */

    swLag = psrLagTbl[swLastLag];
    return (swLag);
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: lookupVq
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to recover the reflection coeffs from
   *     the received LPC codewords.
   *
   *   INPUTS:
   *
   *     pswVqCodeWds[0:2]
   *
   *                         the codewords for each of the segments
   *
   *   OUTPUTS:
   *
   *     pswRCOut[0:NP-1]
   *
   *                        the decoded reflection coefficients
   *
   *   RETURN VALUE:
   *
   *     none.
   *
   *   DESCRIPTION:
   *
   *     For each segment do the following:
   *       setup the retrieval pointers to the correct vector
   *       get that vector
   *
   *   REFERENCES: Sub-clause 4.2.3 of GSM Recomendation 06.20
   *
   *   KEYWORDS: vq, vectorquantizer, lpc
   *
   *************************************************************************/

  static void lookupVq(int16_t pswVqCodeWds[], int16_t pswRCOut[])
  {
  /*_________________________________________________________________________
   |                                                                         |
   |                              Local Constants                            |
   |_________________________________________________________________________|
  */

  #define  LSP_MASK  0x00ff

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    short int siSeg,
           siIndex,
           siVector,
           siVector1,
           siVector2,
           siWordPtr;

    int16_tRom *psrQTable;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* for each segment */
    /* ---------------- */

    for (siSeg = 0; siSeg < QUANT_NUM_OF_TABLES; siSeg++)
    {

      siVector = pswVqCodeWds[siSeg];
      siIndex = psvqIndex[siSeg].l;

      if (sub(siSeg, 2) == 0)
      {                                  /* segment 3 */

        /* set table */
        /* --------- */

        psrQTable = psrQuant3;

        /* set offset into table */
        /* ---------------------- */

        siWordPtr = add(siVector, siVector);

        /* look up coeffs */
        /* -------------- */

        siVector1 = psrQTable[siWordPtr];
        siVector2 = psrQTable[siWordPtr + 1];

        pswRCOut[siIndex - 1] = psrSQuant[shr(siVector1, 8) & LSP_MASK];
        pswRCOut[siIndex] = psrSQuant[siVector1 & LSP_MASK];
        pswRCOut[siIndex + 1] = psrSQuant[shr(siVector2, 8) & LSP_MASK];
        pswRCOut[siIndex + 2] = psrSQuant[siVector2 & LSP_MASK];
      }
      else
      {                                  /* segments 1 and 2 */

        /* set tables */
        /* ---------- */

        if (siSeg == 0)
        {
          psrQTable = psrQuant1;
        }
        else
        {
          psrQTable = psrQuant2;

        }

        /* set offset into table */
        /* --------------------- */

        siWordPtr = add(siVector, siVector);
        siWordPtr = add(siWordPtr, siVector);
        siWordPtr = shr(siWordPtr, 1);

        /* look up coeffs */
        /* -------------- */

        siVector1 = psrQTable[siWordPtr];
        siVector2 = psrQTable[siWordPtr + 1];

        if ((siVector & 0x0001) == 0)
        {
          pswRCOut[siIndex - 1] = psrSQuant[shr(siVector1, 8) & LSP_MASK];
          pswRCOut[siIndex] = psrSQuant[siVector1 & LSP_MASK];
          pswRCOut[siIndex + 1] = psrSQuant[shr(siVector2, 8) & LSP_MASK];
        }
        else
        {
          pswRCOut[siIndex - 1] = psrSQuant[siVector1 & LSP_MASK];
          pswRCOut[siIndex] = psrSQuant[shr(siVector2, 8) & LSP_MASK];
          pswRCOut[siIndex + 1] = psrSQuant[siVector2 & LSP_MASK];
        }
      }
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: lpcFir
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to perform direct form fir filtering
   *     assuming a NP order filter and given state, coefficients, and input.
   *
   *   INPUTS:
   *
   *     NP
   *                     order of the lpc filter
   *
   *     S_LEN
   *                     number of samples to filter
   *
   *     pswInput[0:S_LEN-1]
   *
   *                     input array of points to be filtered.
   *                     pswInput[0] is the oldest point (first to be filtered)
   *                     pswInput[siLen-1] is the last point filtered (newest)
   *
   *     pswCoef[0:NP-1]
   *
   *                     array of direct form coefficients
   *                     pswCoef[0] = coeff for delay n = -1
   *                     pswCoef[NP-1] = coeff for delay n = -NP
   *
   *     ASHIFT
   *                     number of shifts input A's have been shifted down by
   *
   *     LPC_ROUND
   *                     rounding constant
   *
   *     pswState[0:NP-1]
   *
   *                     array of the filter state following form of pswCoef
   *                     pswState[0] = state of filter for delay n = -1
   *                     pswState[NP-1] = state of filter for delay n = -NP
   *
   *   OUTPUTS:
   *
   *     pswState[0:NP-1]
   *
   *                     updated filter state, ready to filter
   *                     pswInput[siLen], i.e. the next point
   *
   *     pswFiltOut[0:S_LEN-1]
   *
   *                     the filtered output
   *                     same format as pswInput, pswFiltOut[0] is oldest point
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     because of the default sign of the coefficients the
   *     formula for the filter is :
   *     i=0, i < S_LEN
   *        out[i] = rounded(state[i]*coef[0])
   *        j=1, j < NP
   *           out[i] += state[j]*coef[j] (state is taken from either input
   *                                       state[] or input in[] arrays)
   *        rescale(out[i])
   *        out[i] += in[i]
   *     update final state array using in[]
   *
   *   REFERENCES: Sub-clause 4.1.7 and 4.2.4 of GSM Recomendation 06.20
   *
   *   KEYWORDS: lpc, directform, fir, lpcFir, inversefilter, lpcFilt
   *   KEYWORDS: dirForm, dir_mod, dir_clr, dir_neg, dir_set, i_dir_mod
   *
   *************************************************************************/

  void Codec::lpcFir(int16_t pswInput[], int16_t pswCoef[],
                       int16_t pswState[], int16_t pswFiltOut[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Sum;
    short int siStage,
           siSmp;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* filter 1st sample */
    /* ----------------- */

    /* sum past state outputs */
    /* ---------------------- */
    /* 0th coef, with rounding */
    L_Sum = L_mac(LPC_ROUND, pswState[0], pswCoef[0]);

    for (siStage = 1; siStage < NP; siStage++)
    {                                    /* remaining coefs */
      L_Sum = L_mac(L_Sum, pswState[siStage], pswCoef[siStage]);
    }

    /* add input to partial output */
    /* --------------------------- */

    L_Sum = L_shl(L_Sum, ASHIFT);
    L_Sum = L_msu(L_Sum, pswInput[0], 0x8000);

    /* save 1st output sample */
    /* ---------------------- */

    pswFiltOut[0] = extract_h(L_Sum);

    /* filter remaining samples */
    /* ------------------------ */

    for (siSmp = 1; siSmp < S_LEN; siSmp++)
    {

      /* sum past outputs */
      /* ---------------- */
      /* 0th coef, with rounding */
      L_Sum = L_mac(LPC_ROUND, pswInput[siSmp - 1], pswCoef[0]);
      /* remaining coefs */
      for (siStage = 1; ((0 < (siSmp - siStage)) && siStage < NP); siStage++)
      {
        L_Sum = L_mac(L_Sum, pswInput[siSmp - siStage - 1], pswCoef[siStage]);
      }

      /* sum past states, if any */
      /* ----------------------- */

      for (siStage = siSmp; siStage < NP; siStage++)
      {
        L_Sum = L_mac(L_Sum, pswState[siStage - siSmp], pswCoef[siStage]);
      }

      /* add input to partial output */
      /* --------------------------- */

      L_Sum = L_shl(L_Sum, ASHIFT);
      L_Sum = L_msu(L_Sum, pswInput[siSmp], 0x8000);

      /* save current output sample */
      /* -------------------------- */

      pswFiltOut[siSmp] = extract_h(L_Sum);
    }

    /* save final state */
    /* ---------------- */

    for (siStage = 0; siStage < NP; siStage++)
    {
      pswState[siStage] = pswInput[S_LEN - siStage - 1];
    }

  }

  /***************************************************************************
   *
   *   FUNCTION NAME: lpcIir
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to perform direct form IIR filtering
   *     assuming a NP order filter and given state, coefficients, and input
   *
   *   INPUTS:
   *
   *     NP
   *                     order of the lpc filter
   *
   *     S_LEN
   *                     number of samples to filter
   *
   *     pswInput[0:S_LEN-1]
   *
   *                     input array of points to be filtered
   *                     pswInput[0] is the oldest point (first to be filtered)
   *                     pswInput[siLen-1] is the last point filtered (newest)
   *
   *     pswCoef[0:NP-1]
   *                     array of direct form coefficients
   *                     pswCoef[0] = coeff for delay n = -1
   *                     pswCoef[NP-1] = coeff for delay n = -NP
   *
   *     ASHIFT
   *                     number of shifts input A's have been shifted down by
   *
   *     LPC_ROUND
   *                     rounding constant
   *
   *     pswState[0:NP-1]
   *
   *                     array of the filter state following form of pswCoef
   *                     pswState[0] = state of filter for delay n = -1
   *                     pswState[NP-1] = state of filter for delay n = -NP
   *
   *   OUTPUTS:
   *
   *     pswState[0:NP-1]
   *
   *                     updated filter state, ready to filter
   *                     pswInput[siLen], i.e. the next point
   *
   *     pswFiltOut[0:S_LEN-1]
   *
   *                     the filtered output
   *                     same format as pswInput, pswFiltOut[0] is oldest point
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     because of the default sign of the coefficients the
   *     formula for the filter is :
   *     i=0, i < S_LEN
   *        out[i] = rounded(state[i]*coef[0])
   *        j=1, j < NP
   *           out[i] -= state[j]*coef[j] (state is taken from either input
   *                                       state[] or prior out[] arrays)
   *        rescale(out[i])
   *        out[i] += in[i]
   *     update final state array using out[]
   *
   *   REFERENCES: Sub-clause 4.1.7 and 4.2.4 of GSM Recomendation 06.20
   *
   *   KEYWORDS: lpc, directform, iir, synthesisfilter, lpcFilt
   *   KEYWORDS: dirForm, dir_mod, dir_clr, dir_neg, dir_set
   *
   *************************************************************************/

  void Codec::lpcIir(int16_t pswInput[], int16_t pswCoef[],
                       int16_t pswState[], int16_t pswFiltOut[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Sum;
    short int siStage,
           siSmp;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* filter 1st sample */
    /* ----------------- */

    /* sum past state outputs */
    /* ---------------------- */
    /* 0th coef, with rounding */
    L_Sum = L_msu(LPC_ROUND, pswState[0], pswCoef[0]);

    for (siStage = 1; siStage < NP; siStage++)
    {                                    /* remaining coefs */
      L_Sum = L_msu(L_Sum, pswState[siStage], pswCoef[siStage]);
    }

    /* add input to partial output */
    /* --------------------------- */

    L_Sum = L_shl(L_Sum, ASHIFT);
    L_Sum = L_msu(L_Sum, pswInput[0], 0x8000);

    /* save 1st output sample */
    /* ---------------------- */

    pswFiltOut[0] = extract_h(L_Sum);

    /* filter remaining samples */
    /* ------------------------ */

    for (siSmp = 1; siSmp < S_LEN; siSmp++)
    {

      /* sum past outputs */
      /* ---------------- */
      /* 0th coef, with rounding */
      L_Sum = L_msu(LPC_ROUND, pswFiltOut[siSmp - 1], pswCoef[0]);
      /* remaining coefs */
      for (siStage = 1; ((0 < (siSmp - siStage)) && siStage < NP); siStage++)
      {
        L_Sum = L_msu(L_Sum, pswFiltOut[siSmp - siStage - 1],
                      pswCoef[siStage]);
      }

      /* sum past states, if any */
      /* ----------------------- */

      for (siStage = siSmp; siStage < NP; siStage++)
      {
        L_Sum = L_msu(L_Sum, pswState[siStage - siSmp], pswCoef[siStage]);
      }

      /* add input to partial output */
      /* --------------------------- */

      L_Sum = L_shl(L_Sum, ASHIFT);
      L_Sum = L_msu(L_Sum, pswInput[siSmp], 0x8000);

      /* save current output sample */
      /* -------------------------- */

      pswFiltOut[siSmp] = extract_h(L_Sum);
    }

    /* save final state */
    /* ---------------- */

    for (siStage = 0; siStage < NP; siStage++)
    {
      pswState[siStage] = pswFiltOut[S_LEN - siStage - 1];
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: lpcIrZsIir
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to calculate the impulse response
   *     via direct form IIR filtering with zero state assuming a NP order
   *     filter and given coefficients
   *
   *   INPUTS:
   *
   *     NP
   *                     order of the lpc filter
   *
   *     S_LEN
   *                     number of samples to filter
   *
   *     pswCoef[0:NP-1]
   *                     array of direct form coefficients
   *                     pswCoef[0] = coeff for delay n = -1
   *                     pswCoef[NP-1] = coeff for delay n = -NP
   *
   *     ASHIFT
   *                     number of shifts input A's have been shifted down by
   *
   *     LPC_ROUND
   *                     rounding constant
   *
   *   OUTPUTS:
   *
   *     pswFiltOut[0:S_LEN-1]
   *
   *                     the filtered output
   *                     same format as pswInput, pswFiltOut[0] is oldest point
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     This routine is called by getNWCoefs().
   *
   *     Because of the default sign of the coefficients the
   *     formula for the filter is :
   *     i=0, i < S_LEN
   *        out[i] = rounded(state[i]*coef[0])
   *        j=1, j < NP
   *           out[i] -= state[j]*coef[j] (state taken from prior output[])
   *        rescale(out[i])
   *
   *   REFERENCES: Sub-clause 4.1.8 of GSM Recomendation 06.20
   *
   *   KEYWORDS: lpc, directform, iir, synthesisfilter, lpcFilt
   *   KEYWORDS: dirForm, dir_mod, dir_clr, dir_neg, dir_set
   *
   *************************************************************************/

  void Codec::lpcIrZsIir(int16_t pswCoef[], int16_t pswFiltOut[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Sum;
    short int siStage,
           siSmp;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* output 1st sample */
    /* ----------------- */

    pswFiltOut[0] = 0x0400;

    /* filter remaining samples */
    /* ------------------------ */

    for (siSmp = 1; siSmp < S_LEN; siSmp++)
    {

      /* sum past outputs */
      /* ---------------- */
      /* 0th coef, with rounding */
      L_Sum = L_msu(LPC_ROUND, pswFiltOut[siSmp - 1], pswCoef[0]);
      /* remaining coefs */
      for (siStage = 1; ((0 < (siSmp - siStage)) && siStage < NP); siStage++)
      {
        L_Sum = L_msu(L_Sum, pswFiltOut[siSmp - siStage - 1],
                      pswCoef[siStage]);
      }

      /* scale output */
      /* ------------ */

      L_Sum = L_shl(L_Sum, ASHIFT);

      /* save current output sample */
      /* -------------------------- */

      pswFiltOut[siSmp] = extract_h(L_Sum);
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: lpcZiIir
   *
   *   PURPOSE:
   *     The purpose of this function is to perform direct form iir filtering
   *     with zero input assuming a NP order filter, and given state and
   *     coefficients
   *
   *   INPUTS:
   *
   *     NP
   *                     order of the lpc filter
   *
   *     S_LEN
   *                     number of samples to filter MUST be <= MAX_ZIS
   *
   *     pswCoef[0:NP-1]
   *
   *                     array of direct form coefficients.
   *                     pswCoef[0] = coeff for delay n = -1
   *                     pswCoef[NP-1] = coeff for delay n = -NP
   *
   *     ASHIFT
   *                     number of shifts input A's have been shifted down by
   *
   *     LPC_ROUND
   *                     rounding constant
   *
   *     pswState[0:NP-1]
   *
   *                     array of the filter state following form of pswCoef
   *                     pswState[0] = state of filter for delay n = -1
   *                     pswState[NP-1] = state of filter for delay n = -NP
   *
   *   OUTPUTS:
   *
   *     pswFiltOut[0:S_LEN-1]
   *
   *                     the filtered output
   *                     same format as pswIn, pswFiltOut[0] is oldest point
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     The routine is called from sfrmAnalysis, and is used to let the
   *     LPC filters ring out.
   *
   *     because of the default sign of the coefficients the
   *     formula for the filter is :
   *     i=0, i < S_LEN
   *        out[i] = rounded(state[i]*coef[0])
   *        j=1, j < NP
   *           out[i] -= state[j]*coef[j] (state is taken from either input
   *                                       state[] or prior output[] arrays)
   *        rescale(out[i])
   *
   *   REFERENCES: Sub-clause 4.1.7 of GSM Recomendation 06.20
   *
   *   KEYWORDS: lpc, directform, iir, synthesisfilter, lpcFilt
   *   KEYWORDS: dirForm, dir_mod, dir_clr, dir_neg, dir_set
   *
   *************************************************************************/

  void Codec::lpcZiIir(int16_t pswCoef[], int16_t pswState[],
                         int16_t pswFiltOut[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Sum;
    short int siStage,
           siSmp;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* filter 1st sample */
    /* ----------------- */

    /* sum past state outputs */
    /* ---------------------- */
    /* 0th coef, with rounding */
    L_Sum = L_msu(LPC_ROUND, pswState[0], pswCoef[0]);

    for (siStage = 1; siStage < NP; siStage++)
    {                                    /* remaining coefs */
      L_Sum = L_msu(L_Sum, pswState[siStage], pswCoef[siStage]);
    }

    /* scale output */
    /* ------------ */

    L_Sum = L_shl(L_Sum, ASHIFT);

    /* save 1st output sample */
    /* ---------------------- */

    pswFiltOut[0] = extract_h(L_Sum);

    /* filter remaining samples */
    /* ------------------------ */

    for (siSmp = 1; siSmp < S_LEN; siSmp++)
    {

      /* sum past outputs */
      /* ---------------- */
      /* 0th coef, with rounding */
      L_Sum = L_msu(LPC_ROUND, pswFiltOut[siSmp - 1], pswCoef[0]);
      /* remaining coefs */
      for (siStage = 1; ((0 < (siSmp - siStage)) && siStage < NP); siStage++)
      {
        L_Sum = L_msu(L_Sum, pswFiltOut[siSmp - siStage - 1],
                      pswCoef[siStage]);
      }

      /* sum past states, if any */
      /* ----------------------- */

      for (siStage = siSmp; siStage < NP; siStage++)
      {
        L_Sum = L_msu(L_Sum, pswState[siStage - siSmp], pswCoef[siStage]);
      }

      /* scale output */
      /* ------------ */

      L_Sum = L_shl(L_Sum, ASHIFT);

      /* save current output sample */
      /* -------------------------- */

      pswFiltOut[siSmp] = extract_h(L_Sum);
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: lpcZsFir
   *
   *   PURPOSE:
   *     The purpose of this function is to perform direct form fir filtering
   *     with zero state, assuming a NP order filter and given coefficients
   *     and non-zero input.
   *
   *   INPUTS:
   *
   *     NP
   *                     order of the lpc filter
   *
   *     S_LEN
   *                     number of samples to filter
   *
   *     pswInput[0:S_LEN-1]
   *
   *                     input array of points to be filtered.
   *                     pswInput[0] is the oldest point (first to be filtered)
   *                     pswInput[siLen-1] is the last point filtered (newest)
   *
   *     pswCoef[0:NP-1]
   *
   *                     array of direct form coefficients
   *                     pswCoef[0] = coeff for delay n = -1
   *                     pswCoef[NP-1] = coeff for delay n = -NP
   *
   *     ASHIFT
   *                     number of shifts input A's have been shifted down by
   *
   *     LPC_ROUND
   *                     rounding constant
   *
   *   OUTPUTS:
   *
   *     pswFiltOut[0:S_LEN-1]
   *
   *                     the filtered output
   *                     same format as pswInput, pswFiltOut[0] is oldest point
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     This routine is used in getNWCoefs().  See section 4.1.7.
   *
   *     because of the default sign of the coefficients the
   *     formula for the filter is :
   *     i=0, i < S_LEN
   *        out[i] = rounded(state[i]*coef[0])
   *        j=1, j < NP
   *           out[i] += state[j]*coef[j] (state taken from in[])
   *        rescale(out[i])
   *        out[i] += in[i]
   *
   *   REFERENCES: Sub-clause 4.1.7 of GSM Recomendation 06.20
   *
   *   KEYWORDS: lpc, directform, fir, lpcFir, inversefilter, lpcFilt
   *   KEYWORDS: dirForm, dir_mod, dir_clr, dir_neg, dir_set, i_dir_mod
   *
   *************************************************************************/

  void Codec::lpcZsFir(int16_t pswInput[], int16_t pswCoef[],
                         int16_t pswFiltOut[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Sum;
    short int siStage,
           siSmp;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* output 1st sample */
    /* ----------------- */

    pswFiltOut[0] = pswInput[0];

    /* filter remaining samples */
    /* ------------------------ */

    for (siSmp = 1; siSmp < S_LEN; siSmp++)
    {

      /* sum past outputs */
      /* ---------------- */
      /* 0th coef, with rounding */
      L_Sum = L_mac(LPC_ROUND, pswInput[siSmp - 1], pswCoef[0]);
      /* remaining coefs */
      for (siStage = 1; ((0 < (siSmp - siStage)) && siStage < NP); siStage++)
      {
        L_Sum = L_mac(L_Sum, pswInput[siSmp - siStage - 1],
                      pswCoef[siStage]);
      }

      /* add input to partial output */
      /* --------------------------- */

      L_Sum = L_shl(L_Sum, ASHIFT);
      L_Sum = L_msu(L_Sum, pswInput[siSmp], 0x8000);

      /* save current output sample */
      /* -------------------------- */

      pswFiltOut[siSmp] = extract_h(L_Sum);
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: lpcZsIir
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to perform direct form IIR filtering
   *     with zero state, assuming a NP order filter and given coefficients
   *     and non-zero input.
   *
   *   INPUTS:
   *
   *     NP
   *                     order of the lpc filter
   *
   *     S_LEN
   *                     number of samples to filter
   *
   *     pswInput[0:S_LEN-1]
   *
   *                     input array of points to be filtered
   *                     pswInput[0] is the oldest point (first to be filtered)
   *                     pswInput[siLen-1] is the last point filtered (newest)
   *
   *     pswCoef[0:NP-1]
   *                     array of direct form coefficients
   *                     pswCoef[0] = coeff for delay n = -1
   *                     pswCoef[NP-1] = coeff for delay n = -NP
   *
   *     ASHIFT
   *                     number of shifts input A's have been shifted down by
   *
   *     LPC_ROUND
   *                     rounding constant
   *
   *   OUTPUTS:
   *
   *     pswFiltOut[0:S_LEN-1]
   *
   *                     the filtered output
   *                     same format as pswInput, pswFiltOut[0] is oldest point
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     This routine is used in the subframe analysis process.  It is
   *     called by sfrmAnalysis() and fnClosedLoop().  It is this function
   *     which performs the weighting of the excitation vectors.
   *
   *     because of the default sign of the coefficients the
   *     formula for the filter is :
   *     i=0, i < S_LEN
   *        out[i] = rounded(state[i]*coef[0])
   *        j=1, j < NP
   *           out[i] -= state[j]*coef[j] (state taken from prior out[])
   *        rescale(out[i])
   *        out[i] += in[i]
   *
   *   REFERENCES: Sub-clause 4.1.8.5 of GSM Recomendation 06.20
   *
   *   KEYWORDS: lpc, directform, iir, synthesisfilter, lpcFilt
   *   KEYWORDS: dirForm, dir_mod, dir_clr, dir_neg, dir_set
   *
   *************************************************************************/

  void Codec::lpcZsIir(int16_t pswInput[], int16_t pswCoef[],
                         int16_t pswFiltOut[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Sum;
    short int siStage,
           siSmp;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* output 1st sample */
    /* ----------------- */

    pswFiltOut[0] = pswInput[0];

    /* filter remaining samples */
    /* ------------------------ */

    for (siSmp = 1; siSmp < S_LEN; siSmp++)
    {

      /* sum past outputs */
      /* ---------------- */
      /* 0th coef, with rounding */
      L_Sum = L_msu(LPC_ROUND, pswFiltOut[siSmp - 1], pswCoef[0]);
      /* remaining coefs */
      for (siStage = 1; ((0 < (siSmp - siStage)) && siStage < NP); siStage++)
      {
        L_Sum = L_msu(L_Sum, pswFiltOut[siSmp - siStage - 1],
                      pswCoef[siStage]);
      }

      /* add input to partial output */
      /* --------------------------- */

      L_Sum = L_shl(L_Sum, ASHIFT);
      L_Sum = L_msu(L_Sum, pswInput[siSmp], 0x8000);

      /* save current output sample */
      /* -------------------------- */

      pswFiltOut[siSmp] = extract_h(L_Sum);
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: lpcZsIirP
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to perform direct form iir filtering
   *     with zero state, assuming a NP order filter and given coefficients
   *     and input
   *
   *   INPUTS:
   *
   *     NP
   *                     order of the lpc filter
   *
   *     S_LEN
   *                     number of samples to filter
   *
   *     pswCommonIO[0:S_LEN-1]
   *
   *                     input array of points to be filtered
   *                     pswCommonIO[0] is oldest point (first to be filtered)
   *                     pswCommonIO[siLen-1] is last point filtered (newest)
   *
   *     pswCoef[0:NP-1]
   *                     array of direct form coefficients
   *                     pswCoef[0] = coeff for delay n = -1
   *                     pswCoef[NP-1] = coeff for delay n = -NP
   *
   *     ASHIFT
   *                     number of shifts input A's have been shifted down by
   *
   *     LPC_ROUND
   *                     rounding constant
   *
   *   OUTPUTS:
   *
   *     pswCommonIO[0:S_LEN-1]
   *
   *                     the filtered output
   *                     pswCommonIO[0] is oldest point
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     This function is called by geNWCoefs().  See section 4.1.7.
   *
   *     because of the default sign of the coefficients the
   *     formula for the filter is :
   *     i=0, i < S_LEN
   *        out[i] = rounded(state[i]*coef[0])
   *        j=1, j < NP
   *           out[i] += state[j]*coef[j] (state taken from prior out[])
   *        rescale(out[i])
   *        out[i] += in[i]
   *
   *   REFERENCES: Sub-clause 4.1.7 of GSM Recomendation 06.20
   *
   *   KEYWORDS: lpc, directform, iir, synthesisfilter, lpcFilt
   *   KEYWORDS: dirForm, dir_mod, dir_clr, dir_neg, dir_set
   *
   *************************************************************************/

  void Codec::lpcZsIirP(int16_t pswCommonIO[], int16_t pswCoef[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Sum;
    short int siStage,
           siSmp;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* filter remaining samples */
    /* ------------------------ */

    for (siSmp = 1; siSmp < S_LEN; siSmp++)
    {

      /* sum past outputs */
      /* ---------------- */
      /* 0th coef, with rounding */
      L_Sum = L_mac(LPC_ROUND, pswCommonIO[siSmp - 1], pswCoef[0]);
      /* remaining coefs */
      for (siStage = 1; ((0 < (siSmp - siStage)) && siStage < NP); siStage++)
      {
        L_Sum = L_mac(L_Sum, pswCommonIO[siSmp - siStage - 1],
                      pswCoef[siStage]);
      }

      /* add input to partial output */
      /* --------------------------- */

      L_Sum = L_shl(L_Sum, ASHIFT);
      L_Sum = L_msu(L_Sum, pswCommonIO[siSmp], 0x8000);

      /* save current output sample */
      /* -------------------------- */

      pswCommonIO[siSmp] = extract_h(L_Sum);
    }
  }

  /**************************************************************************
   *
   *   FUNCTION NAME: pitchPreFilt
   *
   *   PURPOSE:
   *
   *     Performs pitch pre-filter on excitation in speech decoder.
   *
   *   INPUTS:
   *
   *     pswExcite[0:39]
   *
   *                     Synthetic residual signal to be filtered, a subframe-
   *                     length vector.
   *
   *     ppsrPVecIntFilt[0:9][0:5] ([tap][phase])
   *
   *                     Interpolation filter coefficients.
   *
   *     ppsrSqtrP0[0:2][0:31] ([voicing level-1][gain code])
   *
   *                     Sqrt(P0) look-up table, used to determine pitch
   *                     pre-filtering coefficient.
   *
   *     swRxGsp0
   *
   *                     Coded value from gain quantizer, used to look up
   *                     sqrt(P0).
   *
   *     swRxLag
   *
   *                     Full-resolution lag value (fractional lag *
   *                     oversampling factor), used to index pitch pre-filter
   *                     state.
   *
   *     swUvCode
   *
   *                     Coded voicing level, used to distinguish between
   *                     voiced and unvoiced conditions, and to look up
   *                     sqrt(P0).
   *
   *     swSemiBeta
   *
   *                     The gain applied to the adaptive codebook excitation
   *                     (long-term predictor excitation) limited to a maximum
   *                     of 1.0, used to determine the pitch pre-filter
   *                     coefficient.
   *
   *     snsSqrtRs
   *
   *                     The estimate of the energy in the residual, used only
   *                     for scaling.
   *
   *   OUTPUTS:
   *
   *     pswExciteOut[0:39]
   *
   *                     The output pitch pre-filtered excitation.
   *
   *     pswPPreState[0:44]
   *
   *                     Contains the state of the pitch pre-filter
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     If the voicing mode for the frame is unvoiced, then the pitch pre-
   *     filter state is updated with the input excitation, and the input
   *     excitation is copied to the output.
   *
   *     If voiced: first the energy in the input excitation is calculated.
   *     Then, the coefficient of the pitch pre-filter is obtained:
   *
   *     PpfCoef = POST_EPSILON * min(beta, sqrt(P0)).
   *
   *     Then, the pitch pre-filter is performed:
   *
   *     ex_p(n) = ex(n)  +  PpfCoef * ex_p(n-L)
   *
   *     The ex_p(n-L) sample is interpolated from the surrounding samples,
   *     even for integer values of L.
   *
   *     Note: The coefficients of the interpolating filter are multiplied
   *     by PpfCoef, rather multiplying ex_p(n_L) after interpolation.
   *
   *     Finally, the energy in the output excitation is calculated, and
   *     automatic gain control is applied to the output signal so that
   *     its energy matches the original.
   *
   *     The pitch pre-filter is described in section 4.2.2.
   *
   *   REFERENCES: Sub-clause 4.2.2 of GSM Recomendation 06.20
   *
   *   KEYWORDS: prefilter, pitch, pitchprefilter, excitation, residual
   *
   *************************************************************************/

  void Codec::pitchPreFilt(int16_t pswExcite[],
                                  int16_t swRxGsp0,
                                  int16_t swRxLag, int16_t swUvCode,
                                int16_t swSemiBeta, struct NormSw snsSqrtRs,
                                  int16_t pswExciteOut[],
                                  int16_t pswPPreState[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                              Local Constants                            |
   |_________________________________________________________________________|
  */

  #define  POST_EPSILON  0x2666

  /*_________________________________________________________________________
   |                                                                         |
   |                           Local Static Variables                        |
   |_________________________________________________________________________|
  */


  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_1,
           L_OrigEnergy;

    int16_t swScale,
           swSqrtP0,
           swIntLag,
           swRemain,
           swEnergy,
           pswInterpCoefs[P_INT_MACS];

    short int i,
           j;

    struct NormSw snsOrigEnergy;

    int16_t *pswPPreCurr = &pswPPreState[LTP_LEN];

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Initialization */
    /*----------------*/

    swEnergy = 0;

    /* Check voicing level */
    /*---------------------*/

    if (swUvCode == 0)
    {

      /* Unvoiced: perform one subframe of delay on state, copy input to */
      /* state, copy input to output (if not same)                       */
      /*-----------------------------------------------------------------*/

      for (i = 0; i < LTP_LEN - S_LEN; i++)
        pswPPreState[i] = pswPPreState[i + S_LEN];

      for (i = 0; i < S_LEN; i++)
        pswPPreState[i + LTP_LEN - S_LEN] = pswExcite[i];

      if (pswExciteOut != pswExcite)
      {

        for (i = 0; i < S_LEN; i++)
          pswExciteOut[i] = pswExcite[i];
      }
    }
    else
    {

      /* Voiced: calculate energy in input, filter, calculate energy in */
      /* output, scale                                                  */
      /*----------------------------------------------------------------*/

      /* Get energy in input excitation vector */
      /*---------------------------------------*/

      swEnergy = add(negate(shl(snsSqrtRs.sh, 1)), 3);

      if (swEnergy > 0)
      {

        /* High-energy residual: scale input vector during energy */
        /* calculation.  The shift count + 1 of the energy of the */
        /* residual estimate is used as an estimate of the shift  */
        /* count needed for the excitation energy                 */
        /*--------------------------------------------------------*/


        snsOrigEnergy.sh = g_corr1s(pswExcite, swEnergy, &L_OrigEnergy);
        snsOrigEnergy.man = round(L_OrigEnergy);

      }
      else
      {

        /* set shift count to zero for AGC later */
        /*---------------------------------------*/

        swEnergy = 0;

        /* Lower-energy residual: no overflow protection needed */
        /*------------------------------------------------------*/

        L_OrigEnergy = 0;
        for (i = 0; i < S_LEN; i++)
        {

          L_OrigEnergy = L_mac(L_OrigEnergy, pswExcite[i], pswExcite[i]);
        }

        snsOrigEnergy.sh = norm_l(L_OrigEnergy);
        snsOrigEnergy.man = round(L_shl(L_OrigEnergy, snsOrigEnergy.sh));
      }

      /* Determine pitch pre-filter coefficient, and scale the appropriate */
      /* phase of the interpolating filter by it                           */
      /*-------------------------------------------------------------------*/

      swSqrtP0 = ppsrSqrtP0[swUvCode - 1][swRxGsp0];

      if (sub(swSqrtP0, swSemiBeta) > 0)
        swScale = swSemiBeta;
      else
        swScale = swSqrtP0;

      swScale = mult_r(POST_EPSILON, swScale);

      get_ipjj(swRxLag, &swIntLag, &swRemain);

      for (i = 0; i < P_INT_MACS; i++)
        pswInterpCoefs[i] = mult_r(ppsrPVecIntFilt[i][swRemain], swScale);

      /* Perform filter */
      /*----------------*/

      for (i = 0; i < S_LEN; i++)
      {

        L_1 = L_deposit_h(pswExcite[i]);

        for (j = 0; j < P_INT_MACS - 1; j++)
        {

          L_1 = L_mac(L_1, pswPPreCurr[i - swIntLag - P_INT_MACS / 2 + j],
                      pswInterpCoefs[j]);
        }

        pswPPreCurr[i] = mac_r(L_1,
                               pswPPreCurr[i - swIntLag + P_INT_MACS / 2 - 1],
                               pswInterpCoefs[P_INT_MACS - 1]);
      }

      /* Get energy in filtered vector, determine automatic-gain-control */
      /* scale factor                                                    */
      /*-----------------------------------------------------------------*/

      swScale = agcGain(pswPPreCurr, snsOrigEnergy, swEnergy);

      /* Scale filtered vector by AGC, put out.  NOTE: AGC scale returned */
      /* by routine above is divided by two, hence the shift below        */
      /*------------------------------------------------------------------*/

      for (i = 0; i < S_LEN; i++)
      {

        L_1 = L_mult(pswPPreCurr[i], swScale);
        L_1 = L_shl(L_1, 1);
        pswExciteOut[i] = round(L_1);
      }

      /* Update pitch pre-filter state */
      /*-------------------------------*/

      for (i = 0; i < LTP_LEN; i++)
        pswPPreState[i] = pswPPreState[i + S_LEN];
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: r0BasedEnergyShft
   *
   *   PURPOSE:
   *
   *     Given an R0 voicing level, find the number of shifts to be
   *     performed on the energy to ensure that the subframe energy does
   *     not overflow.  example if energy can maximally take the value
   *     4.0, then 2 shifts are required.
   *
   *   INPUTS:
   *
   *     swR0Index
   *                     R0 codeword (0-0x1f)
   *
   *   OUTPUTS:
   *
   *     none
   *
   *   RETURN VALUE:
   *
   *     swShiftDownSignal
   *
   *                    number of right shifts to apply to energy (0..6)
   *
   *   DESCRIPTION:
   *
   *     Based on the R0, the average frame energy, we can get an
   *     upper bound on the energy any one subframe can take on.
   *     Using this upper bound we can calculate what right shift is
   *     needed to ensure an unsaturated output out of a subframe
   *     energy calculation (g_corr).
   *
   *   REFERENCES: Sub-clause 4.1.9 and 4.2.1 of GSM Recomendation 06.20
   *
   *   KEYWORDS: spectral postfilter
   *
   *************************************************************************/

  int16_t Codec::r0BasedEnergyShft(int16_t swR0Index)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swShiftDownSignal;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    if (sub(swR0Index, 26) <= 0)
    {
      if (sub(swR0Index, 23) <= 0)
      {
        if (sub(swR0Index, 21) <= 0)
          swShiftDownSignal = 0;         /* r0  [0,  21] */
        else
          swShiftDownSignal = 1;         /* r0  [22, 23] */
      }
      else
      {
        if (sub(swR0Index, 24) <= 0)
          swShiftDownSignal = 2;         /* r0  [23, 24] */
        else
          swShiftDownSignal = 3;         /* r0  [24, 26] */
      }
    }
    else
    {                                    /* r0 index > 26 */
      if (sub(swR0Index, 28) <= 0)
      {
        swShiftDownSignal = 4;           /* r0  [26, 28] */
      }
      else
      {
        if (sub(swR0Index, 29) <= 0)
          swShiftDownSignal = 5;         /* r0  [28, 29] */
        else
          swShiftDownSignal = 6;         /* r0  [29, 31] */
      }
    }
    if (sub(swR0Index, 18) > 0)
      swShiftDownSignal = add(swShiftDownSignal, 2);

    return (swShiftDownSignal);
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: rcToADp
   *
   *   PURPOSE:
   *
   *     This subroutine computes a vector of direct form LPC filter
   *     coefficients, given an input vector of reflection coefficients.
   *     Double precision is used internally, but 16 bit direct form
   *     filter coefficients are returned.
   *
   *   INPUTS:
   *
   *     NP
   *                     order of the LPC filter (global constant)
   *
   *     swAscale
   *                     The multiplier which scales down the direct form
   *                     filter coefficients.
   *
   *     pswRc[0:NP-1]
   *                     The input vector of reflection coefficients.
   *
   *   OUTPUTS:
   *
   *     pswA[0:NP-1]
   *                     Array containing the scaled down direct form LPC
   *                     filter coefficients.
   *
   *   RETURN VALUE:
   *
   *     siLimit
   *                     1 if limiting occured in computation, 0 otherwise.
   *
   *   DESCRIPTION:
   *
   *     This function performs the conversion from reflection coefficients
   *     to direct form LPC filter coefficients. The direct form coefficients
   *     are scaled by multiplication by swAscale. NP, the filter order is 10.
   *     The a's and rc's each have NP elements in them. Double precision
   *     calculations are used internally.
   *
   *        The equations are:
   *        for i = 0 to NP-1{
   *
   *          a(i)(i) = rc(i)              (scaling by swAscale occurs here)
   *
   *          for j = 0 to i-1
   *             a(i)(j) = a(i-1)(j) + rc(i)*a(i-1)(i-j-1)
   *       }
   *
   *     See page 443, of
   *     "Digital Processing of Speech Signals" by L.R. Rabiner and R.W.
   *     Schafer; Prentice-Hall; Englewood Cliffs, NJ (USA).  1978.
   *
   *   REFERENCES: Sub-clause 4.1.7 and 4.2.3 of GSM Recomendation 06.20
   *
   *  KEYWORDS: reflectioncoefficients, parcors, conversion, rctoadp, ks, as
   *  KEYWORDS: parcorcoefficients, lpc, flat, vectorquantization
   *
   *************************************************************************/

  short  Codec::rcToADp(int16_t swAscale, int16_t pswRc[],
                        int16_t pswA[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t pL_ASpace[NP],
           pL_tmpSpace[NP],
           L_temp,
          *pL_A,
          *pL_tmp,
          *pL_swap;

    short int i,
           j,                            /* loop counters */
           siLimit;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Initialize starting addresses for temporary buffers */
    /*-----------------------------------------------------*/

    pL_A = pL_ASpace;
    pL_tmp = pL_tmpSpace;

    /* Initialize the flag for checking if limiting has occured */
    /*----------------------------------------------------------*/

    siLimit = 0;

    /* Compute direct form filter coefficients, pswA[0],...,pswA[9] */
    /*-------------------------------------------------------------------*/

    for (i = 0; i < NP; i++)
    {

      pL_tmp[i] = L_mult(swAscale, pswRc[i]);
      for (j = 0; j <= i - 1; j++)
      {
        L_temp = L_mpy_ls(pL_A[i - j - 1], pswRc[i]);
        pL_tmp[j] = L_add(L_temp, pL_A[j]);
        siLimit |= isLwLimit(pL_tmp[j]);
      }
      if (i != NP - 1)
      {
        /* Swap swA and swTmp buffers */

        pL_swap = pL_tmp;
        pL_tmp = pL_A;
        pL_A = pL_swap;
      }
    }

    for (i = 0; i < NP; i++)
    {
      pswA[i] = round(pL_tmp[i]);
      siLimit |= isSwLimit(pswA[i]);
    }
    return (siLimit);
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: rcToCorrDpL
   *
   *   PURPOSE:
   *
   *     This subroutine computes an autocorrelation vector, given a vector
   *     of reflection coefficients as an input. Double precision calculations
   *     are used internally, and a double precision (int32_t)
   *     autocorrelation sequence is returned.
   *
   *   INPUTS:
   *
   *     NP
   *                     LPC filter order passed in as a global constant.
   *
   *     swAshift
   *                     Number of right shifts to be applied to the
   *                     direct form filter coefficients being computed
   *                     as an intermediate step to generating the
   *                     autocorrelation sequence.
   *
   *     swAscale
   *                     A multiplicative scale factor corresponding to
   *                     swAshift; i.e. swAscale = 2 ^(-swAshift).
   *
   *     pswRc[0:NP-1]
   *                     An input vector of reflection coefficients.
   *
   *   OUTPUTS:
   *
   *     pL_R[0:NP]
   *                     An output int32_t array containing the
   *                     autocorrelation vector where
   *                     pL_R[0] = 0x7fffffff; (i.e., ~1.0).
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     The algorithm used for computing the correlation sequence is
   *     described on page 232 of the book "Linear Prediction of Speech",
   *     by J.D.  Markel and A.H. Gray, Jr.; Springer-Verlag, Berlin,
   *     Heidelberg, New York, 1976.
   *
   *   REFERENCES: Sub_Clause 4.1.4 and 4.2.1  of GSM Recomendation 06.20
   *
   *   KEYWORDS: normalized autocorrelation, reflection coefficients
   *   KEYWORDS: conversion
   *
   **************************************************************************/

  void Codec::rcToCorrDpL(int16_t swAshift, int16_t swAscale,
                            int16_t pswRc[], int32_t pL_R[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t pL_ASpace[NP],
           pL_tmpSpace[NP],
           L_temp,
           L_sum,
          *pL_A,
          *pL_tmp,
          *pL_swap;

    short int i,
           j;                            /* loop control variables */

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Set R[0] = 0x7fffffff, (i.e., R[0] = 1.0) */
    /*-------------------------------------------*/

    pL_R[0] = LW_MAX;

    /* Assign an address onto each of the two temporary buffers */
    /*----------------------------------------------------------*/

    pL_A = pL_ASpace;
    pL_tmp = pL_tmpSpace;

    /* Compute correlations R[1],...,R[10] */
    /*------------------------------------*/

    for (i = 0; i < NP; i++)
    {

      /* Compute, as an intermediate step, the filter coefficients for */
      /* for an i-th order direct form filter (pL_tmp[j],j=0,i)        */
      /*---------------------------------------------------------------*/

      pL_tmp[i] = L_mult(swAscale, pswRc[i]);
      for (j = 0; j <= i - 1; j++)
      {
        L_temp = L_mpy_ls(pL_A[i - j - 1], pswRc[i]);
        pL_tmp[j] = L_add(L_temp, pL_A[j]);
      }

      /* Swap pL_A and pL_tmp buffers */
      /*------------------------------*/

      pL_swap = pL_A;
      pL_A = pL_tmp;
      pL_tmp = pL_swap;

      /* Given the direct form filter coefficients for an i-th order filter  */
      /* and the autocorrelation vector computed up to and including stage i */
      /* compute the autocorrelation coefficient R[i+1]                      */
      /*---------------------------------------------------------------------*/

      L_temp = L_mpy_ll(pL_A[0], pL_R[i]);
      L_sum = L_negate(L_temp);

      for (j = 1; j <= i; j++)
      {
        L_temp = L_mpy_ll(pL_A[j], pL_R[i - j]);
        L_sum = L_sub(L_sum, L_temp);
      }
      pL_R[i + 1] = L_shl(L_sum, swAshift);

    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: res_eng
   *
   *   PURPOSE:
   *
   *     Calculates square root of subframe residual energy estimate:
   *
   *                     sqrt( R(0)(1-k1**2)...(1-k10**2) )
   *
   *   INPUTS:
   *
   *     pswReflecCoefIn[0:9]
   *
   *                     Array of reflection coeffcients.
   *
   *     swRq
   *
   *                     Subframe energy = sqrt(frame_energy * S_LEN/2**S_SH)
   *                     (quantized).
   *
   *   OUTPUTS:
   *
   *     psnsSqrtRsOut
   *
   *                     (Pointer to) the output residual energy estimate.
   *
   *   RETURN VALUE:
   *
   *     The shift count of the normalized residual energy estimate, as int.
   *
   *   DESCRIPTION:
   *
   *     First, the canonic product of the (1-ki**2) terms is calculated
   *     (normalizations are done to maintain precision).  Also, a factor of
   *     2**S_SH is applied to the product to offset this same factor in the
   *     quantized square root of the subframe energy.
   *
   *     Then the product is square-rooted, and multiplied by the quantized
   *     square root of the subframe energy.  This combined product is put
   *     out as a normalized fraction and shift count (mantissa and exponent).
   *
   *   REFERENCES: Sub-clause 4.1.7 and 4.2.1 of GSM Recomendation 06.20
   *
   *   KEYWORDS: residualenergy, res_eng, rs
   *
   *************************************************************************/

  void Codec::res_eng(int16_t pswReflecCoefIn[], int16_t swRq,
                        struct NormSw *psnsSqrtRsOut)
  {
  /*_________________________________________________________________________
   |                                                                         |
   |                              Local Constants                            |
   |_________________________________________________________________________|
  */

  #define  S_SH          6               /* ceiling(log2(S_LEN)) */
  #define  MINUS_S_SH    -S_SH


  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Product,
           L_Shift,
           L_SqrtResEng;

    int16_t swPartialProduct,
           swPartialProductShift,
           swTerm,
           swShift,
           swSqrtPP,
           swSqrtPPShift,
           swSqrtResEng,
           swSqrtResEngShift;

    short int i;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Form canonic product, maintain precision and shift count */
    /*----------------------------------------------------------*/

    /* (Start off with unity product (actually -1), and shift offset) */
    /*----------------------------------------------------------------*/
    swPartialProduct = SW_MIN;
    swPartialProductShift = MINUS_S_SH;

    for (i = 0; i < NP; i++)
    {

      /* Get next (-1 + k**2) term, form partial canonic product */
      /*---------------------------------------------------------*/


      swTerm = mac_r(LW_MIN, pswReflecCoefIn[i], pswReflecCoefIn[i]);

      L_Product = L_mult(swTerm, swPartialProduct);

      /* Normalize partial product, round */
      /*----------------------------------*/

      swShift = norm_s(extract_h(L_Product));
      swPartialProduct = round(L_shl(L_Product, swShift));
      swPartialProductShift = add(swPartialProductShift, swShift);
    }

    /* Correct sign of product, take square root */
    /*-------------------------------------------*/

    swPartialProduct = abs_s(swPartialProduct);

    swSqrtPP = sqroot(L_deposit_h(swPartialProduct));

    L_Shift = L_shr(L_deposit_h(swPartialProductShift), 1);

    swSqrtPPShift = extract_h(L_Shift);

    if (extract_l(L_Shift) != 0)
    {

      /* Odd exponent: shr above needs to be compensated by multiplying */
      /* mantissa by sqrt(0.5)                                          */
      /*----------------------------------------------------------------*/

      swSqrtPP = mult_r(swSqrtPP, SQRT_ONEHALF);
    }

    /* Form final product, the residual energy estimate, and do final */
    /* normalization                                                  */
    /*----------------------------------------------------------------*/

    L_SqrtResEng = L_mult(swRq, swSqrtPP);

    swShift = norm_l(L_SqrtResEng);
    swSqrtResEng = round(L_shl(L_SqrtResEng, swShift));
    swSqrtResEngShift = add(swSqrtPPShift, swShift);

    /* Return */
    /*--------*/
    psnsSqrtRsOut->man = swSqrtResEng;
    psnsSqrtRsOut->sh = swSqrtResEngShift;

    return;
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: rs_rr
   *
   *   PURPOSE:
   *
   *     Calculates sqrt(RS/R(x,x)) using floating point format,
   *     where RS is the approximate residual energy in a given
   *     subframe and R(x,x) is the power in each long term
   *     predictor vector or in each codevector.
   *     Used in the joint optimization of the gain and the long
   *     term predictor coefficient.
   *
   *   INPUTS:
   *
   *     pswExcitation[0:39] - excitation signal array
   *     snsSqrtRs - structure sqrt(RS) normalized with mantissa and shift
   *
   *   OUTPUTS:
   *
   *     snsSqrtRsRr - structure sqrt(RS/R(x,x)) with mantissa and shift
   *
   *   RETURN VALUE:
   *
   *     None
   *
   *   DESCRIPTION:
   *
   *     Implemented as sqrt(RS)/sqrt(R(x,x)) where both sqrts
   *     are stored normalized (0.5<=x<1.0) and the associated
   *     shift.  See section 4.1.11.1 for details
   *
   *   REFERENCES: Sub-clause 4.1.11.1 and 4.2.1 of GSM
   *               Recomendation 06.20
   *
   *   KEYWORDS: rs_rr, sqroot
   *
   *************************************************************************/

  void Codec::rs_rr(int16_t pswExcitation[], struct NormSw snsSqrtRs,
                      struct NormSw *snsSqrtRsRr)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */
    int32_t L_Temp;
    int16_t swTemp,
           swTemp2,
           swEnergy,
           swNormShift,
           swShift;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    swEnergy = sub(shl(snsSqrtRs.sh, 1), 3);      /* shift*2 + margin ==
                                                   * energy. */


    if (swEnergy < 0)
    {

      /* High-energy residual: scale input vector during energy */
      /* calculation. The shift count of the energy of the      */
      /* residual estimate is used as an estimate of the shift  */
      /* count needed for the excitation energy                 */
      /*--------------------------------------------------------*/

      swNormShift = g_corr1s(pswExcitation, negate(swEnergy), &L_Temp);

    }
    else
    {

      /* Lower-energy residual: no overflow protection needed */
      /*------------------------------------------------------*/

      swNormShift = g_corr1(pswExcitation, &L_Temp);
    }

    /* Compute single precision square root of energy sqrt(R(x,x)) */
    /* ----------------------------------------------------------- */
    swTemp = sqroot(L_Temp);

    /* If odd no. of shifts compensate by sqrt(0.5) */
    /* -------------------------------------------- */
    if (swNormShift & 1)
    {

      /* Decrement no. of shifts in accordance with sqrt(0.5) */
      /* ---------------------------------------------------- */
      swNormShift = sub(swNormShift, 1);

      /* sqrt(R(x,x) = sqrt(R(x,x)) * sqrt(0.5) */
      /* -------------------------------------- */
      L_Temp = L_mult(0x5a82, swTemp);
    }
    else
    {
      L_Temp = L_deposit_h(swTemp);
    }

    /* Normalize again and update shifts */
    /* --------------------------------- */
    swShift = norm_l(L_Temp);
    swNormShift = add(shr(swNormShift, 1), swShift);
    L_Temp = L_shl(L_Temp, swShift);

    /* Shift sqrt(RS) to make sure less than divisor */
    /* --------------------------------------------- */
    swTemp = shr(snsSqrtRs.man, 1);

    /* Divide sqrt(RS)/sqrt(R(x,x)) */
    /* ---------------------------- */
    swTemp2 = divide_s(swTemp, round(L_Temp));

    /* Calculate shift for division, compensate for shift before division */
    /* ------------------------------------------------------------------ */
    swNormShift = sub(snsSqrtRs.sh, swNormShift);
    swNormShift = sub(swNormShift, 1);

    /* Normalize and get no. of shifts */
    /* ------------------------------- */
    swShift = norm_s(swTemp2);
    snsSqrtRsRr->sh = add(swNormShift, swShift);
    snsSqrtRsRr->man = shl(swTemp2, swShift);

  }

  /***************************************************************************
   *
   *   FUNCTION NAME: rs_rrNs
   *
   *   PURPOSE:
   *
   *     Calculates sqrt(RS/R(x,x)) using floating point format,
   *     where RS is the approximate residual energy in a given
   *     subframe and R(x,x) is the power in each long term
   *     predictor vector or in each codevector.
   *
   *     Used in the joint optimization of the gain and the long
   *     term predictor coefficient.
   *
   *   INPUTS:
   *
   *     pswExcitation[0:39] - excitation signal array
   *     snsSqrtRs - structure sqrt(RS) normalized with mantissa and shift
   *
   *   OUTPUTS:
   *
   *     snsSqrtRsRr - structure sqrt(RS/R(x,x)) with mantissa and shift
   *
   *   RETURN VALUE:
   *
   *     None
   *
   *   DESCRIPTION:
   *
   *     Implemented as sqrt(RS)/sqrt(R(x,x)) where both sqrts
   *     are stored normalized (0.5<=x<1.0) and the associated
   *     shift.
   *
   *   REFERENCES: Sub-clause 4.1.11.1 and 4.2.1 of GSM
   *               Recomendation 06.20
   *
   *   KEYWORDS: rs_rr, sqroot
   *
   *************************************************************************/

  void Codec::rs_rrNs(int16_t pswExcitation[], struct NormSw snsSqrtRs,
                        struct NormSw *snsSqrtRsRr)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */
    int32_t L_Temp;
    int16_t swTemp,
           swTemp2,
           swNormShift,
           swShift;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Lower-energy residual: no overflow protection needed */
    /*------------------------------------------------------*/

    swNormShift = g_corr1(pswExcitation, &L_Temp);


    /* Compute single precision square root of energy sqrt(R(x,x)) */
    /* ----------------------------------------------------------- */
    swTemp = sqroot(L_Temp);

    /* If odd no. of shifts compensate by sqrt(0.5) */
    /* -------------------------------------------- */
    if (swNormShift & 1)
    {

      /* Decrement no. of shifts in accordance with sqrt(0.5) */
      /* ---------------------------------------------------- */
      swNormShift = sub(swNormShift, 1);

      /* sqrt(R(x,x) = sqrt(R(x,x)) * sqrt(0.5) */
      /* -------------------------------------- */
      L_Temp = L_mult(0x5a82, swTemp);
    }
    else
    {
      L_Temp = L_deposit_h(swTemp);
    }

    /* Normalize again and update shifts */
    /* --------------------------------- */

    swShift = norm_l(L_Temp);
    swNormShift = add(shr(swNormShift, 1), swShift);
    L_Temp = L_shl(L_Temp, swShift);

    /* Shift sqrt(RS) to make sure less than divisor */
    /* --------------------------------------------- */
    swTemp = shr(snsSqrtRs.man, 1);

    /* Divide sqrt(RS)/sqrt(R(x,x)) */
    /* ---------------------------- */
    swTemp2 = divide_s(swTemp, round(L_Temp));

    /* Calculate shift for division, compensate for shift before division */
    /* ------------------------------------------------------------------ */
    swNormShift = sub(snsSqrtRs.sh, swNormShift);
    swNormShift = sub(swNormShift, 1);

    /* Normalize and get no. of shifts */
    /* ------------------------------- */
    swShift = norm_s(swTemp2);
    snsSqrtRsRr->sh = add(swNormShift, swShift);
    snsSqrtRsRr->man = shl(swTemp2, swShift);

  }


  /***************************************************************************
   *
   *   FUNCTION NAME: scaleExcite
   *
   *   PURPOSE:
   *
   *     Scale an arbitrary excitation vector (codevector or
   *     pitch vector)
   *
   *   INPUTS:
   *
   *     pswVect[0:39] - the unscaled vector.
   *     iGsp0Scale - an positive offset to compensate for the fact
   *                  that GSP0 table is scaled down.
   *     swErrTerm - rather than a gain being passed in, (beta, gamma)
   *                 it is calculated from this error term - either
   *                 Gsp0[][][0] error term A or Gsp0[][][1] error
   *                 term B. Beta is calculated from error term A,
   *                 gamma from error term B.
   *     snsRS - the RS_xx appropriate to pswVect.
   *
   *   OUTPUTS:
   *
   *      pswScldVect[0:39] - the output, scaled excitation vector.
   *
   *   RETURN VALUE:
   *
   *     swGain - One of two things.  Either a clamped value of 0x7fff if the
   *              gain's shift was > 0 or the rounded vector gain otherwise.
   *
   *   DESCRIPTION:
   *
   *     If gain > 1.0 then
   *         (do not shift gain up yet)
   *         partially scale vector element THEN shift and round save
   *      else
   *         shift gain correctly
   *         scale vector element and round save
   *         update state array
   *
   *   REFERENCES: Sub-clause 4.1.10.2 and 4.2.1 of GSM
   *               Recomendation 06.20
   *
   *   KEYWORDS: excite_vl, sc_ex, excitevl, scaleexcite, codevector, p_vec,
   *   KEYWORDS: x_vec, pitchvector, gain, gsp0
   *
   *************************************************************************/

  int16_t Codec::scaleExcite(int16_t pswVect[],
                               int16_t swErrTerm, struct NormSw snsRS,
                               int16_t pswScldVect[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */
    int32_t L_GainUs,
           L_scaled,
           L_Round_off;
    int16_t swGain,
           swGainUs,
           swGainShift,
           i,
           swGainUsShft;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */


    L_GainUs = L_mult(swErrTerm, snsRS.man);
    swGainUsShft = norm_s(extract_h(L_GainUs));
    L_GainUs = L_shl(L_GainUs, swGainUsShft);

    swGainShift = add(swGainUsShft, snsRS.sh);
    swGainShift = sub(swGainShift, GSP0_SCALE);


    /* gain > 1.0 */
    /* ---------- */

    if (swGainShift < 0)
    {
      swGainUs = round(L_GainUs);

      L_Round_off = L_shl((long) 32768, swGainShift);

      for (i = 0; i < S_LEN; i++)
      {
        L_scaled = L_mac(L_Round_off, swGainUs, pswVect[i]);
        L_scaled = L_shr(L_scaled, swGainShift);
        pswScldVect[i] = extract_h(L_scaled);
      }

      if (swGainShift == 0)
        swGain = swGainUs;
      else
        swGain = 0x7fff;
    }

    /* gain < 1.0 */
    /* ---------- */

    else
    {

      /* shift down or not at all */
      /* ------------------------ */
      if (swGainShift > 0)
        L_GainUs = L_shr(L_GainUs, swGainShift);

      /* the rounded actual vector gain */
      /* ------------------------------ */
      swGain = round(L_GainUs);

      /* now scale the vector (with rounding) */
      /* ------------------------------------ */

      for (i = 0; i < S_LEN; i++)
      {
        L_scaled = L_mac((long) 32768, swGain, pswVect[i]);
        pswScldVect[i] = extract_h(L_scaled);
      }
    }
    return (swGain);
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: spectralPostFilter
   *
   *   PURPOSE:
   *
   *     Perform spectral post filter on the output of the
   *     synthesis filter.
   *
   *
   *   INPUT:
   *
   *      S_LEN         a global constant
   *
   *      pswSPFIn[0:S_LEN-1]
   *
   *                    input to the routine. Unmodified
   *                    pswSPFIn[0] is the oldest point (first to be filtered),
   *                    pswSPFIn[iLen-1] is the last pointer filtered,
   *                    the newest.
   *
   *      pswNumCoef[0:NP-1],pswDenomCoef[0:NP-1]
   *
   *                    numerator and denominator
   *                    direct form coeffs used by postfilter.
   *                    Exactly like lpc coefficients in format.  Shifted down
   *                    by iAShift to ensure that they are < 1.0.
   *
   *      gpswPostFiltStateNum[0:NP-1], gpswPostFiltStateDenom[0:NP-1]
   *
   *                    array of the filter state.
   *                    Same format as coefficients: *praState = state of
   *                    filter for delay n = -1 praState[NP] = state of
   *                    filter for delay n = -NP These numbers are not
   *                    shifted at all.  These states are static to this
   *                    file.
   *
   *   OUTPUT:
   *
   *      gpswPostFiltStateNum[0:NP-1], gpswPostFiltStateDenom[0:NP-1]
   *
   *                      See above for description.  These are updated.
   *
   *      pswSPFOut[0:S_LEN-1]
   *
   *                    same format as pswSPFIn,
   *                    *pswSPFOut is oldest point. The filtered output.
   *                    Note this routine can handle pswSPFOut = pswSPFIn.
   *                    output can be the same as input.  i.e. in place
   *                    calculation.
   *
   *   RETURN:
   *
   *      none
   *
   *   DESCRIPTION:
   *
   *      find energy in input,
   *      perform the numerator fir
   *      perform the denominator iir
   *      perform the post emphasis
   *      find energy in signal,
   *      perform the agc using energy in and energy in signam after
   *      post emphasis signal
   *
   *      The spectral postfilter is described in section 4.2.4.
   *
   *   REFERENCES: Sub-clause 4.2.4 of GSM Recomendation 06.20
   *
   *   KEYWORDS: postfilter, emphasis, postemphasis, brightness,
   *   KEYWORDS: numerator, deminator, filtering, lpc,
   *
   *************************************************************************/

  void Codec::spectralPostFilter(int16_t pswSPFIn[],
                                        int16_t pswNumCoef[],
                              int16_t pswDenomCoef[], int16_t pswSPFOut[])
  {
  /*_________________________________________________________________________
   |                                                                         |
   |                              Local Constants                            |
   |_________________________________________________________________________|
  */

  #define  AGC_COEF       (int16_t)0x19a        /* (1.0 - POST_AGC_COEF)
                                                   * 1.0-.9875 */
  #define  POST_EMPHASIS  (int16_t)0x199a       /* 0.2 */

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    short int i;

    int32_t L_OrigEnergy,
           L_runningGain,
           L_Output;

    int16_t swAgcGain,
           swRunningGain,
           swTemp;

    struct NormSw snsOrigEnergy;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* calculate the energy in the input and save it */
    /*-----------------------------------------------*/

    snsOrigEnergy.sh = g_corr1s(pswSPFIn, swEngyRShift, &L_OrigEnergy);
    snsOrigEnergy.man = round(L_OrigEnergy);

    /* numerator of the postfilter */
    /*-----------------------------*/

    lpcFir(pswSPFIn, pswNumCoef, gpswPostFiltStateNum, pswSPFOut);

    /* denominator of the postfilter */
    /*-------------------------------*/

    lpcIir(pswSPFOut, pswDenomCoef, gpswPostFiltStateDenom, pswSPFOut);

    /* postemphasis section of postfilter */
    /*------------------------------------*/

    for (i = 0; i < S_LEN; i++)
    {
      swTemp = msu_r(L_deposit_h(pswSPFOut[i]), swPostEmphasisState,
                     POST_EMPHASIS);
      swPostEmphasisState = pswSPFOut[i];
      pswSPFOut[i] = swTemp;
    }

    swAgcGain = agcGain(pswSPFOut, snsOrigEnergy, swEngyRShift);

    /* scale the speech vector */
    /*-----------------------------*/

    swRunningGain = gswPostFiltAgcGain;
    L_runningGain = L_deposit_h(gswPostFiltAgcGain);
    for (i = 0; i < S_LEN; i++)
    {
      L_runningGain = L_msu(L_runningGain, swRunningGain, AGC_COEF);
      L_runningGain = L_mac(L_runningGain, swAgcGain, AGC_COEF);
      swRunningGain = extract_h(L_runningGain);

      /* now scale input with gain */

      L_Output = L_mult(swRunningGain, pswSPFOut[i]);
      pswSPFOut[i] = extract_h(L_shl(L_Output, 2));
    }
    gswPostFiltAgcGain = swRunningGain;

  }

  /***************************************************************************
   *
   *   FUNCTION NAME: speechDecoder
   *
   *   PURPOSE:
   *     The purpose of this function is to call all speech decoder
   *     subroutines.  This is the top level routine for the speech
   *     decoder.
   *
   *   INPUTS:
   *
   *     pswParameters[0:21]
   *
   *        pointer to this frame's parameters.  See below for input
   *        data format.
   *
   *   OUTPUTS:
   *
   *     pswDecodedSpeechFrame[0:159]
   *
   *        this frame's decoded 16 bit linear pcm frame
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     The sequence of events in the decoder, and therefore this routine
   *     follow a simple plan.  First, the frame based parameters are
   *     decoded.  Second, on a subframe basis, the subframe based
   *     parameters are decoded and the excitation is generated.  Third,
   *     on a subframe basis, the combined and scaled excitation is
   *     passed through the synthesis filter, and then the pitch and
   *     spectral postfilters.
   *
   *     The in-line comments for the routine speechDecoder, are very
   *     detailed.  Here in a more consolidated form, are the main
   *     points.
   *
   *     The R0 parameter is decoded using the lookup table
   *     psrR0DecTbl[].  The LPC codewords are looked up using lookupVQ().
   *     The decoded parameters are reflection coefficients
   *     (pswFrmKs[]).
   *
   *     The decoder does not use reflection coefficients directly.
   *     Instead it converts them to direct form coeficients.  This is
   *     done using rcToADp().  If this conversion results in invalid
   *     results, the previous frames parameters are used.
   *
   *     The direct form coeficients are used to derive the spectal
   *     postfilter's numerator and denominator coeficients.  The
   *     denominators coefficients are widened, and the numerators
   *     coefficients are a spectrally smoothed version of the
   *     denominator.  The smoothing is done with a_sst().
   *
   *     The frame based LPC coefficients are either used directly as the
   *     subframe coefficients, or are derived through interpolation.
   *     The subframe based coeffiecients are calculated in getSfrmLpc().
   *
   *     Based on voicing mode, the decoder will construct and scale the
   *     excitation in one of two ways.  For the voiced mode the lag is
   *     decoded using lagDecode().  The fractional pitch LTP lookup is
   *     done by the function fp_ex().  In both voiced and unvoiced
   *     mode, the VSELP codewords are decoded into excitation vectors
   *     using b_con() and v_con().
   *
   *     rs_rr(), rs_rrNs(), and scaleExcite() are used to calculate
   *     the gamma's, codevector gains, as well as beta, the LTP vector
   *     gain.  Description of this can be found in section 4.1.11.  Once
   *     the vectors have been scaled and combined, the excitation is
   *     stored in the LTP history.
   *
   *     The excitation, pswExcite[], passes through the pitch pre-filter
   *     (pitchPreFilt()).  Then the harmonically enhanced excitation
   *     passes through the synthesis filter, lpcIir(), and finally the
   *     reconstructed speech passes through the spectral post-filter
   *     (spectalPostFilter()).  The final output speech is passed back in
   *     the array pswDecodedSpeechFrame[].
   *
   *   INPUT DATA FORMAT:
   *
   *      The format/content of the input parameters is the so called
   *      bit alloc format.
   *
   *     voiced mode bit alloc format:
   *     -----------------------------
   *     index    number of bits  parameter name
   *     0        5               R0
   *     1        11              k1Tok3
   *     2        9               k4Tok6
   *     3        8               k7Tok10
   *     4        1               softInterpolation
   *     5        2               voicingDecision
   *     6        8               frameLag
   *     7        9               code_1
   *     8        5               gsp0_1
   *     9        4               deltaLag_2
   *     10       9               code_2
   *     11       5               gsp0_2
   *     12       4               deltaLag_3
   *     13       9               code_3
   *     14       5               gsp0_3
   *     15       4               deltaLag_4
   *     16       9               code_4
   *     17       5               gsp0_4
   *
   *     18       1               BFI
   *     19       1               UFI
   *     20       2               SID
   *     21       1               TAF
   *
   *
   *     unvoiced mode bit alloc format:
   *     -------------------------------
   *
   *     index    number of bits  parameter name
   *     0        5               R0
   *     1        11              k1Tok3
   *     2        9               k4Tok6
   *     3        8               k7Tok10
   *     4        1               softInterpolation
   *     5        2               voicingDecision
   *     6        7               code1_1
   *     7        7               code2_1
   *     8        5               gsp0_1
   *     9        7               code1_2
   *     10       7               code2_2
   *     11       5               gsp0_2
   *     12       7               code1_3
   *     13       7               code2_3
   *     14       5               gsp0_3
   *     15       7               code1_4
   *     16       7               code2_4
   *     17       5               gsp0_4
   *
   *     18       1               BFI
   *     19       1               UFI
   *     20       2               SID
   *     21       1               TAF
   *
   *
   *   REFERENCES: Sub_Clause 4.2 of GSM Recomendation 06.20
   *
   *   KEYWORDS: synthesis, speechdecoder, decoding
   *   KEYWORDS: codewords, lag, codevectors, gsp0
   *
   *************************************************************************/

  void   Codec::speechDecoder(int16_t pswParameters[],
                              int16_t pswDecodedSpeechFrame[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    short int i,
           j,
           siLagCode,
           siGsp0Code,
           psiVselpCw[2],
           siVselpCw,
           siNumBits,
           siCodeBook;

    int16_t pswFrmKs[NP],
           pswFrmAs[NP],
           pswFrmPFNum[NP],
           pswFrmPFDenom[NP],
           pswPVec[S_LEN],
           ppswVselpEx[2][S_LEN],
           pswExcite[S_LEN],
           pswPPFExcit[S_LEN],
           pswSynthFiltOut[S_LEN],
           swR0Index,
           swLag,
           swSemiBeta,
           pswBitArray[MAXBITS];

    struct NormSw psnsSqrtRs[N_SUB],
           snsRs00,
           snsRs11,
           snsRs22;


    int16_t swMutePermit,
           swLevelMean,
           swLevelMax,                   /* error concealment */
           swMuteFlag;                   /* error concealment */


    int16_t swTAF,
           swSID,
           swBfiDtx;                     /* DTX mode */
    int16_t swFrameType;               /* DTX mode */

    int32_t L_RxDTXGs;                  /* DTX mode */

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* -------------------------------------------------------------------- */
    /* do bad frame handling (error concealment) and comfort noise          */
    /* insertion                                                            */
    /* -------------------------------------------------------------------- */


    /* This flag indicates whether muting is performed in the actual frame */
    /* ------------------------------------------------------------------- */
    swMuteFlag = 0;


    /* This flag indicates whether muting is allowed in the actual frame */
    /* ----------------------------------------------------------------- */
    swMutePermit = 0;


    /* frame classification */
    /* -------------------- */

    swSID = pswParameters[20];
    swTAF = pswParameters[21];

    swBfiDtx = pswParameters[18] | pswParameters[19];     /* BFI | UFI */

    if ((swSID == 2) && (swBfiDtx == 0))
      swFrameType = VALIDSID;
    else if ((swSID == 0) && (swBfiDtx == 0))
      swFrameType = GOODSPEECH;
    else if ((swSID == 0) && (swBfiDtx != 0))
      swFrameType = UNUSABLE;
    else
      swFrameType = INVALIDSID;


    /* update of decoder state */
    /* ----------------------- */

    if (swDecoMode == SPEECH)
    {
      /* speech decoding mode */
      /* -------------------- */

      if (swFrameType == VALIDSID)
        swDecoMode = CNIFIRSTSID;
      else if (swFrameType == INVALIDSID)
        swDecoMode = CNIFIRSTSID;
      else if (swFrameType == UNUSABLE)
        swDecoMode = SPEECH;
      else if (swFrameType == GOODSPEECH)
        swDecoMode = SPEECH;
    }
    else
    {
      /* comfort noise insertion mode */
      /* ---------------------------- */

      if (swFrameType == VALIDSID)
        swDecoMode = CNICONT;
      else if (swFrameType == INVALIDSID)
        swDecoMode = CNICONT;
      else if (swFrameType == UNUSABLE)
        swDecoMode = CNIBFI;
      else if (swFrameType == GOODSPEECH)
        swDecoMode = SPEECH;
    }


    if (swDecoMode == SPEECH)
    {
      /* speech decoding mode */
      /* -------------------- */

      /* Perform parameter concealment, depending on BFI (pswParameters[18]) */
      /* or UFI (pswParameters[19])                                          */
      /* ------------------------------------------------------------------- */
      para_conceal_speech_decoder(&pswParameters[18],
                                  pswParameters, &swMutePermit);


      /* copy the frame rate parameters */
      /* ------------------------------ */

      swR0Index = pswParameters[0];      /* R0 Index */
      pswVq[0] = pswParameters[1];       /* LPC1 */
      pswVq[1] = pswParameters[2];       /* LPC2 */
      pswVq[2] = pswParameters[3];       /* LPC3 */
      swSi = pswParameters[4];           /* INT_LPC */
      swVoicingMode = pswParameters[5];  /* MODE */


      /* lookup R0 and VQ parameters */
      /* --------------------------- */

      swR0Dec = psrR0DecTbl[swR0Index * 2];       /* R0 */
      lookupVq(pswVq, pswFrmKs);


      /* save this frames GS values */
      /* -------------------------- */

      for (i = 0; i < N_SUB; i++)
      {
        pL_RxGsHist[swRxGsHistPtr] =
                ppLr_gsTable[swVoicingMode][pswParameters[(i * 3) + 8]];
        swRxGsHistPtr++;
        if (swRxGsHistPtr > ((OVERHANG - 1) * N_SUB) - 1)
          swRxGsHistPtr = 0;
      }


      /* DTX variables */
      /* ------------- */

      swDtxBfiCnt = 0;
      swDtxMuting = 0;
      swRxDTXState = CNINTPER - 1;

    }
    else
    {
      /* comfort noise insertion mode */
      /*----------------------------- */

      /* copy the frame rate parameters */
      /* ------------------------------ */

      swR0Index = pswParameters[0];      /* R0 Index */
      pswVq[0] = pswParameters[1];       /* LPC1 */
      pswVq[1] = pswParameters[2];       /* LPC2 */
      pswVq[2] = pswParameters[3];       /* LPC3 */
      swSi = 1;                          /* INT_LPC */
      swVoicingMode = 0;                 /* MODE */


      /* bad frame handling in comfort noise insertion mode */
      /* -------------------------------------------------- */

      if (swDecoMode == CNIFIRSTSID)     /* first SID frame */
      {
        swDtxBfiCnt = 0;
        swDtxMuting = 0;
        swRxDTXState = CNINTPER - 1;

        if (swFrameType == VALIDSID)     /* valid SID frame */
        {
          swR0NewCN = psrR0DecTbl[swR0Index * 2];
          lookupVq(pswVq, pswFrmKs);
        }
        else if (swFrameType == INVALIDSID)       /* invalid SID frame */
        {
          swR0NewCN = psrR0DecTbl[swOldR0IndexDec * 2];
          swR0Index = swOldR0IndexDec;
          for (i = 0; i < NP; i++)
            pswFrmKs[i] = pswOldFrmKsDec[i];
        }

      }
      else if (swDecoMode == CNICONT)    /* SID frame detected, but */
      {                                  /* not the first SID       */
        swDtxBfiCnt = 0;
        swDtxMuting = 0;

        if (swFrameType == VALIDSID)     /* valid SID frame */
        {
          swRxDTXState = 0;
          swR0NewCN = psrR0DecTbl[swR0Index * 2];
          lookupVq(pswVq, pswFrmKs);
        }
        else if (swFrameType == INVALIDSID)       /* invalid SID frame */
        {
          if (swRxDTXState < (CNINTPER - 1))
            swRxDTXState = add(swRxDTXState, 1);
          swR0Index = swOldR0IndexDec;
        }

      }
      else if (swDecoMode == CNIBFI)     /* bad frame received in */
      {                                  /* CNI mode              */
        if (swRxDTXState < (CNINTPER - 1))
          swRxDTXState = add(swRxDTXState, 1);
        swR0Index = swOldR0IndexDec;

        if (swDtxMuting == 1)
        {
          swOldR0IndexDec = sub(swOldR0IndexDec, 2);      /* attenuate
                                                           * by 4 dB */
          if (swOldR0IndexDec < 0)
            swOldR0IndexDec = 0;

          swR0Index = swOldR0IndexDec;

          swR0NewCN = psrR0DecTbl[swOldR0IndexDec * 2];   /* R0 */

        }

        swDtxBfiCnt = add(swDtxBfiCnt, 1);
        if ((swTAF == 1) && (swDtxBfiCnt >= (2 * CNINTPER + 1)))  /* 25 */
          swDtxMuting = 1;

      }


      if (swDecoMode == CNIFIRSTSID)
      {

        /* the first SID frame is received */
        /* ------------------------------- */

        /* initialize the decoders pn-generator */
        /* ------------------------------------ */

        L_RxPNSeed = PN_INIT_SEED;


        /* using the stored rx history, generate averaged GS */
        /* ------------------------------------------------- */

        avgGsHistQntz(pL_RxGsHist, &L_RxDTXGs);
        swRxDtxGsIndex = gsQuant(L_RxDTXGs, 0);

      }


      /* Replace the "transmitted" subframe parameters with */
      /* synthetic ones                                     */
      /* -------------------------------------------------- */

      for (i = 0; i < 4; i++)
      {
        /* initialize the GSP0 parameter */
        pswParameters[(i * 3) + 8] = swRxDtxGsIndex;

        /* CODE1 */
        pswParameters[(i * 3) + 6] = getPnBits(7, &L_RxPNSeed);
        /* CODE2 */
        pswParameters[(i * 3) + 7] = getPnBits(7, &L_RxPNSeed);
      }


      /* Interpolation of CN parameters */
      /* ------------------------------ */

      rxInterpR0Lpc(pswOldFrmKsDec, pswFrmKs, swRxDTXState,
                    swDecoMode, swFrameType);

    }


    /* ------------------- */
    /* do frame processing */
    /* ------------------- */

    /* generate the direct form coefs */
    /* ------------------------------ */

    if (!rcToADp(ASCALE, pswFrmKs, pswFrmAs))
    {

      /* widen direct form coefficients using the widening coefs */
      /* ------------------------------------------------------- */

      for (i = 0; i < NP; i++)
      {
        pswFrmPFDenom[i] = mult_r(pswFrmAs[i], psrSPFDenomWidenCf[i]);
      }

      a_sst(ASHIFT, ASCALE, pswFrmPFDenom, pswFrmPFNum);
    }
    else
    {


      for (i = 0; i < NP; i++)
      {
        pswFrmKs[i] = pswOldFrmKsDec[i];
        pswFrmAs[i] = pswOldFrmAsDec[i];
        pswFrmPFDenom[i] = pswOldFrmPFDenom[i];
        pswFrmPFNum[i] = pswOldFrmPFNum[i];
      }
    }

    /* interpolate, or otherwise get sfrm reflection coefs */
    /* --------------------------------------------------- */

    getSfrmLpc(swSi, swOldR0Dec, swR0Dec, pswOldFrmKsDec, pswOldFrmAsDec,
               pswOldFrmPFNum, pswOldFrmPFDenom, pswFrmKs, pswFrmAs,
               pswFrmPFNum, pswFrmPFDenom, psnsSqrtRs, ppswSynthAs,
               ppswPFNumAs, ppswPFDenomAs);

    /* calculate shift for spectral postfilter */
    /* --------------------------------------- */

    swEngyRShift = r0BasedEnergyShft(swR0Index);


    /* ----------------------- */
    /* do sub-frame processing */
    /* ----------------------- */

    for (giSfrmCnt = 0; giSfrmCnt < 4; giSfrmCnt++)
    {

      /* copy this sub-frame's parameters */
      /* -------------------------------- */

      if (sub(swVoicingMode, 0) == 0)
      {                                  /* unvoiced */
        psiVselpCw[0] = pswParameters[(giSfrmCnt * 3) + 6];       /* CODE_1 */
        psiVselpCw[1] = pswParameters[(giSfrmCnt * 3) + 7];       /* CODE_2 */
        siGsp0Code = pswParameters[(giSfrmCnt * 3) + 8];  /* GSP0 */
      }
      else
      {                                  /* voiced */
        siLagCode = pswParameters[(giSfrmCnt * 3) + 6];   /* LAG */
        psiVselpCw[0] = pswParameters[(giSfrmCnt * 3) + 7];       /* CODE */
        siGsp0Code = pswParameters[(giSfrmCnt * 3) + 8];  /* GSP0 */
      }

      /* for voiced mode, reconstruct the pitch vector */
      /* --------------------------------------------- */

      if (swVoicingMode)
      {

        /* convert delta lag to lag and convert to fractional lag */
        /* ------------------------------------------------------ */

        swLag = lagDecode(siLagCode);

        /* state followed by out */
        /* --------------------- */

        fp_ex(swLag, pswLtpStateOut);

        /* extract a piece of pswLtpStateOut into newly named vector pswPVec */
        /* ----------------------------------------------------------------- */

        for (i = 0; i < S_LEN; i++)
        {
          pswPVec[i] = pswLtpStateOut[i];
        }
      }

      /* for unvoiced, do not reconstruct a pitch vector */
      /* ----------------------------------------------- */

      else
      {
        swLag = 0;                       /* indicates invalid lag
                                          * and unvoiced */
      }

      /* now work on the VSELP codebook excitation output */
      /* x_vec, x_a_vec here named ppswVselpEx[0] and [1] */
      /* ------------------------------------------------ */

      if (swVoicingMode)
      {                                  /* voiced */

        siNumBits = C_BITS_V;
        siVselpCw = psiVselpCw[0];

        b_con(siVselpCw, siNumBits, pswBitArray);

        v_con(pppsrVcdCodeVec[0][0], ppswVselpEx[0], pswBitArray, siNumBits);
      }

      else
      {                                  /* unvoiced */

        siNumBits = C_BITS_UV;

        for (siCodeBook = 0; siCodeBook < 2; siCodeBook++)
        {

          siVselpCw = psiVselpCw[siCodeBook];

          b_con(siVselpCw, siNumBits, (int16_t *) pswBitArray);

          v_con(pppsrUvCodeVec[siCodeBook][0], ppswVselpEx[siCodeBook],
                pswBitArray, siNumBits);
        }
      }

      /* all excitation vectors have been created: ppswVselpEx and pswPVec  */
      /* if voiced compute rs00 and rs11; if unvoiced cmpute rs11 and rs22  */
      /* ------------------------------------------------------------------ */

      if (swLag)
      {
        rs_rr(pswPVec, psnsSqrtRs[giSfrmCnt], &snsRs00);
      }

      rs_rrNs(ppswVselpEx[0], psnsSqrtRs[giSfrmCnt], &snsRs11);

      if (!swVoicingMode)
      {
        rs_rrNs(ppswVselpEx[1], psnsSqrtRs[giSfrmCnt], &snsRs22);
      }

      /* now implement synthesis - combine the excitations */
      /* ------------------------------------------------- */

      if (swVoicingMode)
      {                                  /* voiced */

        /* scale pitch and codebook excitations and get beta */
        /* ------------------------------------------------- */
        swSemiBeta = scaleExcite(pswPVec,
                                 pppsrGsp0[swVoicingMode][siGsp0Code][0],
                                 snsRs00, pswPVec);
        scaleExcite(ppswVselpEx[0],
                    pppsrGsp0[swVoicingMode][siGsp0Code][1],
                    snsRs11, ppswVselpEx[0]);

        /* combine the two scaled excitations */
        /* ---------------------------------- */
        for (i = 0; i < S_LEN; i++)
        {
          pswExcite[i] = add(pswPVec[i], ppswVselpEx[0][i]);
        }
      }
      else
      {                                  /* unvoiced */

        /* scale codebook excitations and set beta to 0 as not applicable */
        /* -------------------------------------------------------------- */
        swSemiBeta = 0;
        scaleExcite(ppswVselpEx[0],
                    pppsrGsp0[swVoicingMode][siGsp0Code][0],
                    snsRs11, ppswVselpEx[0]);
        scaleExcite(ppswVselpEx[1],
                    pppsrGsp0[swVoicingMode][siGsp0Code][1],
                    snsRs22, ppswVselpEx[1]);

        /* combine the two scaled excitations */
        /* ---------------------------------- */
        for (i = 0; i < S_LEN; i++)
        {
          pswExcite[i] = add(ppswVselpEx[1][i], ppswVselpEx[0][i]);
        }
      }

      /* now update the pitch state using the combined/scaled excitation */
      /* --------------------------------------------------------------- */

      for (i = 0; i < LTP_LEN; i++)
      {
        pswLtpStateBaseDec[i] = pswLtpStateBaseDec[i + S_LEN];
      }

      /* add the current sub-frames data to the state */
      /* -------------------------------------------- */

      for (i = -S_LEN, j = 0; j < S_LEN; i++, j++)
      {
        pswLtpStateOut[i] = pswExcite[j];/* add new frame at t = -S_LEN */
      }

      /* given the excitation perform pitch prefiltering */
      /* ----------------------------------------------- */

      pitchPreFilt(pswExcite, siGsp0Code, swLag,
                   swVoicingMode, swSemiBeta, psnsSqrtRs[giSfrmCnt],
                   pswPPFExcit, pswPPreState);


      /* Concealment on subframe signal level: */
      /* ------------------------------------- */
      level_estimator(0, &swLevelMean, &swLevelMax,
                      &pswDecodedSpeechFrame[giSfrmCnt * S_LEN]);

      signal_conceal_sub(pswPPFExcit, ppswSynthAs[giSfrmCnt], pswSynthFiltState,
                      &pswLtpStateOut[-S_LEN], &pswPPreState[LTP_LEN - S_LEN],
                         swLevelMean, swLevelMax,
                         pswParameters[19], swMuteFlagOld,
                         &swMuteFlag, swMutePermit);


      /* synthesize the speech through the synthesis filter */
      /* -------------------------------------------------- */

      lpcIir(pswPPFExcit, ppswSynthAs[giSfrmCnt], pswSynthFiltState,
             pswSynthFiltOut);

      /* pass reconstructed speech through adaptive spectral postfilter */
      /* -------------------------------------------------------------- */

      spectralPostFilter(pswSynthFiltOut, ppswPFNumAs[giSfrmCnt],
                         ppswPFDenomAs[giSfrmCnt],
                         &pswDecodedSpeechFrame[giSfrmCnt * S_LEN]);

      level_estimator(1, &swLevelMean, &swLevelMax,
                      &pswDecodedSpeechFrame[giSfrmCnt * S_LEN]);

    }

    /* Save muting information for next frame */
    /* -------------------------------------- */
    swMuteFlagOld = swMuteFlag;

    /* end of frame processing - save this frame's frame energy,  */
    /* reflection coefs, direct form coefs, and post filter coefs */
    /* ---------------------------------------------------------- */

    swOldR0Dec = swR0Dec;
    swOldR0IndexDec = swR0Index;         /* DTX mode */

    for (i = 0; i < NP; i++)
    {
      pswOldFrmKsDec[i] = pswFrmKs[i];
      pswOldFrmAsDec[i] = pswFrmAs[i];
      pswOldFrmPFNum[i] = pswFrmPFNum[i];
      pswOldFrmPFDenom[i] = pswFrmPFDenom[i];
    }
  }


  /***************************************************************************
   *
   *   FUNCTION NAME: sqroot
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to perform a single precision square
   *     root function on a int32_t
   *
   *   INPUTS:
   *
   *     L_SqrtIn
   *                     input to square root function
   *
   *   OUTPUTS:
   *
   *     none
   *
   *   RETURN VALUE:
   *
   *     swSqrtOut
   *                     output to square root function
   *
   *   DESCRIPTION:
   *
   *      Input assumed to be normalized
   *
   *      The algorithm is based around a six term Taylor expansion :
   *
   *        y^0.5 = (1+x)^0.5
   *             ~= 1 + (x/2) - 0.5*((x/2)^2) + 0.5*((x/2)^3)
   *                - 0.625*((x/2)^4) + 0.875*((x/2)^5)
   *
   *      Max error less than 0.08 % for normalized input ( 0.5 <= x < 1 )
   *
   *   REFERENCES: Sub-clause 4.1.4.1, 4.1.7, 4.1.11.1, 4.2.1,
   *               4.2.2, 4.2.3 and 4.2.4 of GSM Recomendation 06.20
   *
   *   KEYWORDS: sqrt, squareroot, sqrt016
   *
   *************************************************************************/

  int16_t Codec::sqroot(int32_t L_SqrtIn)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                              Local Constants                            |
   |_________________________________________________________________________|
  */

  #define    PLUS_HALF          0x40000000L       /* 0.5 */
  #define    MINUS_ONE          0x80000000L       /* -1 */
  #define    TERM5_MULTIPLER    0x5000   /* 0.625 */
  #define    TERM6_MULTIPLER    0x7000   /* 0.875 */

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Temp0,
           L_Temp1;

    int16_t swTemp,
           swTemp2,
           swTemp3,
           swTemp4,
           swSqrtOut;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* determine 2nd term x/2 = (y-1)/2 */
    /* -------------------------------- */

    L_Temp1 = L_shr(L_SqrtIn, 1);        /* L_Temp1 = y/2 */
    L_Temp1 = L_sub(L_Temp1, PLUS_HALF); /* L_Temp1 = (y-1)/2 */
    swTemp = extract_h(L_Temp1);         /* swTemp = x/2 */

    /* add contribution of 2nd term */
    /* ---------------------------- */

    L_Temp1 = L_sub(L_Temp1, MINUS_ONE); /* L_Temp1 = 1 + x/2 */

    /* determine 3rd term */
    /* ------------------ */

    L_Temp0 = L_msu(0L, swTemp, swTemp); /* L_Temp0 = -(x/2)^2 */
    swTemp2 = extract_h(L_Temp0);        /* swTemp2 = -(x/2)^2 */
    L_Temp0 = L_shr(L_Temp0, 1);         /* L_Temp0 = -0.5*(x/2)^2 */

    /* add contribution of 3rd term */
    /* ---------------------------- */

    L_Temp0 = L_add(L_Temp1, L_Temp0);   /* L_Temp0 = 1 + x/2 - 0.5*(x/2)^2 */

    /* determine 4rd term */
    /* ------------------ */

    L_Temp1 = L_msu(0L, swTemp, swTemp2);/* L_Temp1 = (x/2)^3 */
    swTemp3 = extract_h(L_Temp1);        /* swTemp3 = (x/2)^3 */
    L_Temp1 = L_shr(L_Temp1, 1);         /* L_Temp1 = 0.5*(x/2)^3 */

    /* add contribution of 4rd term */
    /* ---------------------------- */

    /* L_Temp1 = 1 + x/2 - 0.5*(x/2)^2 + 0.5*(x/2)^3 */

    L_Temp1 = L_add(L_Temp0, L_Temp1);

    /* determine partial 5th term */
    /* -------------------------- */

    L_Temp0 = L_mult(swTemp, swTemp3);   /* L_Temp0 = (x/2)^4 */
    swTemp4 = round(L_Temp0);            /* swTemp4 = (x/2)^4 */

    /* determine partial 6th term */
    /* -------------------------- */

    L_Temp0 = L_msu(0L, swTemp2, swTemp3);        /* L_Temp0 = (x/2)^5 */
    swTemp2 = round(L_Temp0);            /* swTemp2 = (x/2)^5 */

    /* determine 5th term and add its contribution */
    /* ------------------------------------------- */

    /* L_Temp0 = -0.625*(x/2)^4 */

    L_Temp0 = L_msu(0L, TERM5_MULTIPLER, swTemp4);

    /* L_Temp1 = 1 + x/2 - 0.5*(x/2)^2 + 0.5*(x/2)^3 - 0.625*(x/2)^4 */

    L_Temp1 = L_add(L_Temp0, L_Temp1);

    /* determine 6th term and add its contribution */
    /* ------------------------------------------- */

    /* swSqrtOut = 1 + x/2 - 0.5*(x/2)^2 + 0.5*(x/2)^3 */
    /* - 0.625*(x/2)^4 + 0.875*(x/2)^5     */

    swSqrtOut = mac_r(L_Temp1, TERM6_MULTIPLER, swTemp2);

    /* return output */
    /* ------------- */

    return (swSqrtOut);
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: v_con
   *
   *   PURPOSE:
   *
   *     This subroutine constructs a codebook excitation
   *     vector from basis vectors
   *
   *   INPUTS:
   *
   *     pswBVects[0:siNumBVctrs*S_LEN-1]
   *
   *                     Array containing a set of basis vectors.
   *
   *     pswBitArray[0:siNumBVctrs-1]
   *
   *                     Bit array dictating the polarity of the
   *                     basis vectors in the output vector.
   *                     Each element of the bit array is either -0.5 or +0.5
   *
   *     siNumBVctrs
   *                     Number of bits in codeword
   *
   *   OUTPUTS:
   *
   *     pswOutVect[0:39]
   *
   *                     Array containing the contructed output vector
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *     The array pswBitArray is used to multiply each of the siNumBVctrs
   *     basis vectors.  The input pswBitArray[] is an array whose
   *     elements are +/-0.5.  These multiply the VSELP basis vectors and
   *     when summed produce a VSELP codevector.  b_con() is the function
   *     used to translate a VSELP codeword into pswBitArray[].
   *
   *
   *   REFERENCES: Sub-clause 4.1.10 and 4.2.1 of GSM Recomendation 06.20
   *
   *   KEYWORDS: v_con, codeword, reconstruct, basis vector, excitation
   *
   *************************************************************************/

  void Codec::v_con(int16_t pswBVects[], int16_t pswOutVect[],
                      int16_t pswBitArray[], short int siNumBVctrs)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_Temp;

    short int siSampleCnt,
           siCVectCnt;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Sample loop  */
    /*--------------*/
    for (siSampleCnt = 0; siSampleCnt < S_LEN; siSampleCnt++)
    {

      /* First element of output vector */
      L_Temp = L_mult(pswBitArray[0], pswBVects[0 * S_LEN + siSampleCnt]);

      /* Construct output vector */
      /*-------------------------*/
      for (siCVectCnt = 1; siCVectCnt < siNumBVctrs; siCVectCnt++)
      {
        L_Temp = L_mac(L_Temp, pswBitArray[siCVectCnt],
                       pswBVects[siCVectCnt * S_LEN + siSampleCnt]);
      }

      /* store the output vector sample */
      /*--------------------------------*/
      L_Temp = L_shl(L_Temp, 1);
      pswOutVect[siSampleCnt] = extract_h(L_Temp);
    }
  }

  // from dtx.c

#define PN_XOR_REG (int32_t)0x00000005L
#define PN_XOR_ADD (int32_t)0x40000000L

#define OH_SHIFT 3                     /* shift corresponding to OVERHANG */

#define NP_AFLAT 4
#define LPC_VQ_SEG 3

#define ASHIFT 4
#define ASCALE 0x0800

  /* interpolation curve for comfort noise (i*1/12) i=1..12 */
  int16_t psrCNNewFactor[12] = {0x0aaa, 0x1554, 0x1ffe, 0x2aa8, 0x3552,
    0x3ffc, 0x4aa6, 0x5550, 0x5ffa, 0x6aa4,
  0x754e, 0x7fff};


  /* Values of GS for voicing state 0, all values shifted down by 2
     shifts */
  int32_tRom ppLr_gsTable[4][32] =
  {
    {
      0x000011ab, 0x000038d2, 0x0000773e, 0x000144ef,
      0x00035675, 0x000648c5, 0x000c3d65, 0x0017ae17,
      0x002a3dbb, 0x005238e7, 0x00695c1a, 0x00a60d45,
      0x00e4cc68, 0x01c3ba6a, 0x019e3c96, 0x02d1fbac,
      0x030453ec, 0x0549a998, 0x05190298, 0x08258920,
      0x08daff30, 0x0c3150e0, 0x0e45d850, 0x14c111a0,
      0x0ff7e1c0, 0x18a06860, 0x13810400, 0x1abc9ee0,
      0x28500940, 0x41f22800, 0x22fc5040, 0x2cd90180
    },

    {
      0x00003ede, 0x00021fc9, 0x0013f0c3, 0x003a7be2,
      0x007a6663, 0x00fe3773, 0x012fabf4, 0x02275cd0,
      0x01c0ef14, 0x02c0b1d8, 0x0350fc70, 0x05505078,
      0x04175f30, 0x052c1098, 0x08ed3310, 0x0a63b470,
      0x05417870, 0x08995ee0, 0x07bbe018, 0x0a19fa10,
      0x0b5818c0, 0x0fd96ea0, 0x0e5cad10, 0x13b40d40,
      0x12d45840, 0x14577320, 0x2b2e5e00, 0x333e9640,
      0x194c35c0, 0x1c30f8c0, 0x2d16db00, 0x2cc970ff
    },
    {
      0x002f18e7, 0x00a47be0, 0x01222efe, 0x01c42df8,
      0x024be794, 0x03424c40, 0x036950fc, 0x04973108,
      0x038405b4, 0x05d8c8f0, 0x05063e08, 0x070cdea0,
      0x05812be8, 0x06da5fc8, 0x088fcd60, 0x0a013cb0,
      0x0909a460, 0x09e6cf40, 0x0ee581d0, 0x0ec99f20,
      0x0b4e7470, 0x0c730e80, 0x0ff39d20, 0x105d0d80,
      0x158b0b00, 0x172babe0, 0x14576460, 0x181a6720,
      0x26126e80, 0x1f590180, 0x1fdaad60, 0x2e0e8000
    },
    {
      0x00c7f603, 0x01260cda, 0x01b3926a, 0x026d82bc,
      0x0228fba0, 0x036ec5b0, 0x034bf4cc, 0x043a55d0,
      0x044f9c20, 0x05c66f50, 0x0515f890, 0x06065300,
      0x0665dc00, 0x0802b630, 0x0737a1c0, 0x087294e0,
      0x09253fc0, 0x0a619760, 0x097bd060, 0x0a6d4e50,
      0x0d19e520, 0x0e15c420, 0x0c4e4eb0, 0x0e8880e0,
      0x11cdf480, 0x12c85800, 0x10f4c0a0, 0x13e51b00,
      0x189dbaa0, 0x18a6bb60, 0x22e31500, 0x21615240
    }
  };

  /*************************************************************************
   *
   *   FUNCTION NAME: swComfortNoise
   *
   *   PURPOSE:
   *
   *   This routine perform the following tasks:
   *     - generation of the speech flag (swSP)
   *     - averaging and encoding of the comfort noise parameters
   *     - randomization of the codebook indices
   *
   *
   *   INPUTS:
   *
   *   swVadFrmCnt (global) - swVadFlag=0 frame counter.
   *   If swVadFlag=1 then this counter is 0, the first frame with
   *   swVadFlag=0 will set this counter to 1, with each additional
   *   swVadFlag=0 frame the counter is incremented.
   *
   *   swVadFlag - voise activity flag. swVadFlag=0 frame with
   *   no voice activity, swVadFlag=0 frame with voice activity
   *
   *   L_UnqntzdR0 - unquantized R(0), 32 bit value, output of
   *   FLAT.
   *
   *   pL_UnqntzdCorr[NP+1] - unquantized correlation sequence,
   *   also an output of FLAT.
   *
   *
   *   OUTPUTS:
   *
   *   swCNR0 - global variable, the output quantized R0 index
   *
   *   pswCNLpc[3]  - global variable, the output quantized LPC to the
   *   transmitted in the SID frame
   *
   *   pswCNGsp0Code[N_SUB] - global variable, the output quantized GSP0 indices
   *
   *   pswCNVSCode1[N_SUB] - global variable, the output quantized codevector 1
   *   indices.
   *
   *   pswCNVSCode2[N_SUB] - global variable, the output quantized codevector 2
   *   indices.
   *
   *
   *   RETURN VALUE:
   *
   *   swSP - speech flag, swSP=1 speech frames are generated, swSP=0
   *   SID frames are generated.
   *
   *************************************************************************/

  int16_t Codec::swComfortNoise(int16_t swVadFlag,
                               int32_t L_UnqntzdR0, int32_t *pL_UnqntzdCorr)
  {



  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swSP;
    int16_t pswFinalRc[NP];

    /* unquantized reference parameters */
    int32_t L_RefR0;
    int32_t pL_RefCorr[NP + 1];
    int32_t L_RefGs;

    int    i;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    swSP = 1;

    /* VadFrmCnt will indicate the number of sequential frames where */
    /* swVadFlag == 0                                                */
    /* ------------------------------------------------------------- */

    if (swVadFlag)
      swVadFrmCnt = 0;                   /* Voice acitvity present */
    else
      swVadFrmCnt = add(swVadFrmCnt, 1); /* no voice activity */


    /* swNElapsed will indicate the number of frames that have elapsed */
    /* since the last SID frame with updated comfort noise parameters  */
    /* was generated                                                   */
    /* --------------------------------------------------------------- */

    swNElapsed = add(swNElapsed, 1);


    /* If no voice activity was detected.  */
    /* ----------------------------------- */

    if (swVadFrmCnt)
    {

      /* Short speech burst ? */
      /* -------------------- */

      if (swVadFrmCnt == 1)
      {
        if (sub(swNElapsed, 24) < 0)
          swShortBurst = 1;              /* short speech burst detected */
        else
          swShortBurst = 0;              /* long speech burst detected */
      }


      /* Update history, with this frames data */
      /* ------------------------------------- */

      updateCNHist(L_UnqntzdR0, pL_UnqntzdCorr,
                   pL_R0Hist, ppL_CorrHist);


      /* first SID frame */
      /* --------------- */

      if (((swShortBurst == 0) && (swVadFrmCnt == OVERHANG)) ||
          ((swShortBurst == 1) && (swVadFrmCnt == 1)))
      {

        /* init. random generator */
        /* ---------------------- */
        L_TxPNSeed = PN_INIT_SEED;


        /* average GS */
        /* ---------- */
        avgGsHistQntz(pL_GsHist, &L_RefGs);


        /* GS quantization */
        /* --------------- */
        swRefGsIndex = gsQuant(L_RefGs, 0);

      }


      /* No Overhang in case of short speech bursts,                */
      /* generate SID frames with repeated comfort noise parameters */
      /* ---------------------------------------------------------- */

      if ((swShortBurst == 1) && (swVadFrmCnt < OVERHANG))
      {

        /* generate a SID frame with repeated parameters */
        /* --------------------------------------------- */

        swSP = 0;


        /* repeat data: r0, LPC, GS */
        /* ------------------------ */

        swCNR0 = swQntRefR0;

        for (i = 0; i < 3; i++)
          pswCNLpc[i] = piRefVqCodewds[i];

        for (i = 0; i < N_SUB; i++)
          pswCNGsp0Code[i] = swRefGsIndex;

      }


      /* generate SID frames with updated comfort noise parameters */
      /* --------------------------------------------------------- */

      if (swVadFrmCnt >= OVERHANG)
      {

        /* A SID frame with updated parameters */
        /* ----------------------------------- */

        swSP = 0;
        swNElapsed = 0;


        /* average R0 and correlation values */
        /* --------------------------------- */

        avgCNHist(pL_R0Hist, ppL_CorrHist, &L_RefR0,
                  pL_RefCorr);


        /* now quantize the averaged R(0) */
        /* ------------------------------ */

        swQntRefR0 = r0Quant(L_RefR0);


        /* Quantize the averaged correlation */
        /* --------------------------------- */

        lpcCorrQntz(pL_RefCorr,
                    pswFinalRc,
                    piRefVqCodewds);


        /* update frame data: r0, LPC */
        /* -------------------------- */

        swCNR0 = swQntRefR0;
        for (i = 0; i < 3; i++)
          pswCNLpc[i] = piRefVqCodewds[i];


        /* update subframe data (unvoiced mode): GSP0 */
        /* ------------------------------------------ */

        for (i = 0; i < N_SUB; i++)
          pswCNGsp0Code[i] = swRefGsIndex;

      }


      /* random codevectors */
      /* ------------------ */

      if (swSP == 0)
      {
        for (i = 0; i < N_SUB; i++)
        {
          pswCNVSCode1[i] = getPnBits(7, &L_TxPNSeed);
          pswCNVSCode2[i] = getPnBits(7, &L_TxPNSeed);
        }
      }


    }

    return (swSP);
  }


  /*************************************************************************
   *
   *   FUNCTION NAME:  updateCNHist
   *
   *   PURPOSE:
   *
   *     Add current frame's unquantized R(0) and LPC information to the
   *     comfort noise history, so that it will be available for
   *     averaging.
   *
   *   INPUTS:
   *
   *     Unquantized values from the coder:
   *
   *
   *     L_UnqntzdR0 - unquantized frame energy R(0), an output of FLAT
   *
   *     pL_UnqntzdCorr[NP+1] - unquantized correlation coefficient
   *     array.  Also an output of FLAT.
   *
   *     siUpdPointer (global) - A modulo counter which counts up from
   *     0 to OVERHANG-1.
   *
   *   OUTPUTS:
   *
   *     pL_R0History[OVERHANG] - history of the OVERHANG frames worth of
   *     R(0).
   *
   *     ppL_CorrHistory[OVERHANG][NP+1] - - history of the OVERHANG
   *     frames worth of pL_UnqntzdCorr[].
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *************************************************************************/

  void   Codec::updateCNHist(int32_t L_UnqntzdR0,
                             int32_t *pL_UnqntzdCorr,
                             int32_t pL_R0History[],
                             int32_t ppL_CorrHistory[OVERHANG][NP + 1])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int    i;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* update */
    pL_R0History[siUpdPointer] = L_UnqntzdR0;

    for (i = 0; i < NP + 1; i++)
      ppL_CorrHistory[siUpdPointer][i] = pL_UnqntzdCorr[i];

    siUpdPointer = (siUpdPointer + 1) % OVERHANG;
  }


  /*************************************************************************
   *
   *   FUNCTION NAME: avgGsHistQntz
   *
   *   PURPOSE:
   *
   *     Average gs history, where history is of length OVERHANG-1
   *     frames.  The last frame's (i.e. this frame) gs values are not
   *     available since quantization would have occured only after the
   *     VAD decision is made.
   *
   *   INPUTS:
   *
   *     pL_GsHistory[(OVERHANG-1)*N_SUB] - the GS of the past
   *     OVERHANG-1 frames. The GS values are stored shifted down by 2
   *     shifts to avoid overflow (the largest GS is greater than 2.0).
   *
   *
   *   OUTPUTS:
   *
   *     *pL_GsAvgd - the average of pL_GsHistory[], also shifted down
   *     by two shifts.
   *
   *   RETURN VALUE:
   *
   *     none.
   *
   *
   *************************************************************************/

  void Codec::avgGsHistQntz(int32_t pL_GsHistory[], int32_t *pL_GsAvgd)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int    i;
    int32_t L_avg;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    L_avg = L_shift_r(pL_GsHistory[0], -(OH_SHIFT + 2));

    for (i = 1; i < N_SUB * (OVERHANG - 1); i++)
      L_avg = L_add(L_shift_r(pL_GsHistory[i], -(OH_SHIFT + 2)), L_avg);

    /* avg number x/32 not x/28 */

    *pL_GsAvgd = L_add(L_avg, L_mpy_ls(L_avg, 0x1249));   /* L_avg *= 32/28 */

  }


  /*************************************************************************
   *
   *   FUNCTION NAME: gsQuant
   *
   *   PURPOSE:
   *
   *     Quantize a value of gs in any of the voicing modes.  Input GS
   *     is a 32 bit number.  The GSP0 index is returned.
   *
   *   INPUTS:
   *
   *     L_GsIn - 32 bit GS value,  shifted down by 2 shifts.
   *
   *     swVoicingMode - voicing level
   *
   *     ppLr_gsTable[4][32] - Rom GS Table. (global), all GS values
   *     have been shifted down by 2 from their true value.
   *
   *   OUTPUTS:
   *
   *     none
   *
   *   RETURN VALUE:
   *
   *
   *     GSP0 Index closest to the input value of GS.
   *
   *
   *************************************************************************/

  int16_t Codec::gsQuant(int32_t L_GsIn, int16_t swVoicingMode)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swGsIndex,
           swBestGs;
    int32_t L_diff,
           L_min = LW_MAX;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    for (swGsIndex = 0; swGsIndex < 32; swGsIndex++)
    {
      L_diff = L_abs(L_sub(L_GsIn, ppLr_gsTable[swVoicingMode][swGsIndex]));

      if (L_sub(L_diff, L_min) < 0)
      {
        /* new minimum */
        /* ----------- */

        swBestGs = swGsIndex;
        L_min = L_diff;

      }
    }

    return (swBestGs);

  }


  /*************************************************************************
   *
   *   FUNCTION NAME: avgCNHist
   *
   *   PURPOSE:
   *
   *     Average the unquantized R0 and LPC data stored at the encoder
   *     to arrive at an average R0 and LPC frame for use in a SID
   *     frame.
   *
   *   INPUTS:
   *
   *   pL_R0History[OVERHANG] - contains unquantized R(0) data from the
   *   most recent OVERHANG frame (including this one).
   *
   *   ppL_CorrHistory[OVERHANG][NP+1] - Unquantized correlation
   *   coefficients from the most recent OVERHANG frame (including this
   *   one).  The data stored here is an output of FLAT.
   *
   *   OUTPUTS:
   *
   *   *pL_AvgdR0 - the average of pL_R0History[]
   *
   *   pL_AvgdCorrSeq[NP+1] - the average of ppL_CorrHistory[][].
   *
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *************************************************************************/

  void Codec::avgCNHist(int32_t pL_R0History[],
                          int32_t ppL_CorrHistory[OVERHANG][NP + 1],
                          int32_t *pL_AvgdR0,
                          int32_t pL_AvgdCorrSeq[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int    i,
           j;
    int32_t L_avg;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* R0 Averaging */
    /* ------------ */

    for (L_avg = 0, i = 0; i < OVERHANG; i++)
      L_avg = L_add(L_shr(pL_R0History[i], OH_SHIFT), L_avg);

    *pL_AvgdR0 = L_avg;


    /* LPC: average the last OVERHANG frames */
    /* ------------------------------------- */

    for (j = 0; j < NP + 1; j++)
    {
      for (L_avg = 0, i = 0; i < OVERHANG; i++)
      {
        L_avg = L_add(L_shift_r(ppL_CorrHistory[i][j], -OH_SHIFT), L_avg);
      }

      pL_AvgdCorrSeq[j] = L_avg;
    }

  }


  /***************************************************************************
   *
   *    FUNCTION NAME: lpcCorrQntz
   *
   *    PURPOSE:  Quantize a correlation sequence
   *
   *
   *    INPUT:
   *
   *         pL_CorrelSeq[NP+1]
   *                     Correlation sequence to quantize.
   *
   *    OUTPUTS:
   *
   *        pswFinalRc[0:NP-1]
   *                     A quantized set of NP reflection coefficients.
   *
   *        piVQCodewds[0:2]
   *                     An array containing the indices of the 3 reflection
   *                     coefficient vectors selected from the three segment
   *                     Rc-VQ.
   *
   *    RETURN:
   *        None.
   *
   *    KEYWORDS: AFLAT,aflat,flat,vectorquantization, reflectioncoefficients
   *
   *************************************************************************/

  void   Codec::lpcCorrQntz(int32_t pL_CorrelSeq[],
                            int16_t pswFinalRc[],
                            int piVQCodewds[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t pswPOldSpace[NP_AFLAT],
           pswPNewSpace[NP_AFLAT],
           pswVOldSpace[2 * NP_AFLAT - 1],
           pswVNewSpace[2 * NP_AFLAT - 1],
          *ppswPAddrs[2],
          *ppswVAddrs[2],
          *pswVBar,
           pswPBar[NP_AFLAT],
           pswVBarSpace[2 * NP_AFLAT - 1],
           pswFlatsRc[NP],               /* Unquantized Rc's computed by FLAT */
           pswRc[NP + 1];                /* Temp list for the converted RC's */
    int32_t *pL_VBarFull,
           pL_PBarFull[NP],
           pL_VBarFullSpace[2 * NP - 1];

    int    i,
           iVec,
           iSeg,
           iCnt;                         /* Loop counter */
    struct QuantList quantList,          /* A list of vectors */
           bestPql[4];                   /* The four best vectors from
                                          * the PreQ */
    struct QuantList bestQl[LPC_VQ_SEG + 1];      /* Best vectors for each of
                                                   * the three segments */

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Setup pointers temporary space */
    /*--------------------------------*/

    pswVBar = pswVBarSpace + NP_AFLAT - 1;
    pL_VBarFull = pL_VBarFullSpace + NP - 1;
    ppswPAddrs[0] = pswPOldSpace;
    ppswPAddrs[1] = pswPNewSpace;
    ppswVAddrs[0] = pswVOldSpace + NP_AFLAT - 1;
    ppswVAddrs[1] = pswVNewSpace + NP_AFLAT - 1;


    /* Set up pL_PBarFull and pL_VBarFull initial conditions, using the   */
    /* autocorrelation sequence derived from the optimal reflection       */
    /* coefficients computed by FLAT. The initial conditions are shifted  */
    /* right by RSHIFT bits. These initial conditions, stored as          */
    /* int32_ts, are used to initialize PBar and VBar arrays for the     */
    /* next VQ segment.                                                   */
    /*--------------------------------------------------------------------*/

    initPBarFullVBarFullL(pL_CorrelSeq, pL_PBarFull, pL_VBarFull);

    /* Set up initial PBar and VBar initial conditions, using pL_PBarFull */
    /* and pL_VBarFull arrays initialized above. These are the initial    */
    /* PBar and VBar conditions to be used by the AFLAT recursion at the  */
    /* 1-st Rc-VQ segment.                                                */
    /*--------------------------------------------------------------------*/

    initPBarVBarL(pL_PBarFull, pswPBar, pswVBar);

    for (iSeg = 1; iSeg <= LPC_VQ_SEG; iSeg++)
    {
      /* initialize candidate list */
      /*---------------------------*/

      quantList.iNum = psrPreQSz[iSeg - 1];
      quantList.iRCIndex = 0;

      /* do aflat for all vectors in the list */
      /*--------------------------------------*/

      setupPreQ(iSeg, quantList.iRCIndex);        /* set up vector ptrs */

      for (iCnt = 0; iCnt < quantList.iNum; iCnt++)
      {
        /* get a vector */
        /*--------------*/

        getNextVec(pswRc);

        /* clear the limiter flag */
        /*------------------------*/

        iLimit = 0;

        /* find the error values for each vector */
        /*---------------------------------------*/

        quantList.pswPredErr[iCnt] =
                aflatRecursion(&pswRc[psvqIndex[iSeg - 1].l],
                               pswPBar, pswVBar,
                               ppswPAddrs, ppswVAddrs,
                               psvqIndex[iSeg - 1].len);

        /* check the limiter flag */
        /*------------------------*/

        if (iLimit)
          quantList.pswPredErr[iCnt] = 0x7fff;    /* set error to bad value */

      }                                  /* done list loop */

      /* find 4 best prequantizer levels */
      /*---------------------------------*/

      findBestInQuantList(quantList, 4, bestPql);

      for (iVec = 0; iVec < 4; iVec++)
      {

        /* initialize quantizer list */
        /*---------------------------*/

        quantList.iNum = psrQuantSz[iSeg - 1];
        quantList.iRCIndex = bestPql[iVec].iRCIndex * psrQuantSz[iSeg - 1];

        setupQuant(iSeg, quantList.iRCIndex);     /* set up vector ptrs */

        /* do aflat recursion on each element of list */
        /*--------------------------------------------*/

        for (iCnt = 0; iCnt < quantList.iNum; iCnt++)
        {
          /* get a vector */
          /*--------------*/

          getNextVec(pswRc);

          /* clear the limiter flag */
          /*------------------------*/

          iLimit = 0;

          /* find the error values for each vector */
          /*---------------------------------------*/

          quantList.pswPredErr[iCnt] =
                  aflatRecursion(&pswRc[psvqIndex[iSeg - 1].l],
                                 pswPBar, pswVBar,
                                 ppswPAddrs, ppswVAddrs,
                                 psvqIndex[iSeg - 1].len);

          /* check the limiter flag */
          /*------------------------*/

          if (iLimit)
            quantList.pswPredErr[iCnt] = 0x7fff;  /* set error to the worst
                                                   * value */

        }                                /* done list loop */

        /* find best quantizer vector for this segment, and save it */
        /*----------------------------------------------------------*/

        findBestInQuantList(quantList, 1, bestQl);
        if (iVec == 0)
          bestQl[iSeg] = bestQl[0];
        else if (sub(bestQl[iSeg].pswPredErr[0], bestQl[0].pswPredErr[0]) > 0)
          bestQl[iSeg] = bestQl[0];

      }

      /* find the quantized reflection coefficients */
      /*--------------------------------------------*/

      setupQuant(iSeg, bestQl[iSeg].iRCIndex);    /* set up vector ptrs */
      getNextVec((int16_t *) (pswFinalRc - 1));


      /* Update pBarFull and vBarFull for the next Rc-VQ segment, and */
      /* update the pswPBar and pswVBar for the next Rc-VQ segment    */
      /*--------------------------------------------------------------*/

      if (iSeg < LPC_VQ_SEG)
        aflatNewBarRecursionL(&pswFinalRc[psvqIndex[iSeg - 1].l - 1], iSeg,
                              pL_PBarFull, pL_VBarFull, pswPBar, pswVBar);

    }

    /* find the quantizer index (the values to be output in the symbol file) */
    /*-----------------------------------------------------------------*/

    for (iSeg = 1; iSeg <= LPC_VQ_SEG; iSeg++)
      piVQCodewds[iSeg - 1] = bestQl[iSeg].iRCIndex;

  }


  /*************************************************************************
   *
   *   FUNCTION NAME: getPnBits
   *
   *   PURPOSE:
   *
   *     Generate iBits pseudo-random bits using *pL_PNSeed as the
   *     pn-generators seed.
   *
   *   INPUTS:
   *
   *     iBits - integer indicating how many random bits to return.
   *     range [0,15], 0 yields 1 bit output
   *
   *     *pL_PNSeed - 32 bit seed (changed by function)
   *
   *   OUTPUTS:
   *
   *     *pL_PNSeed - 32 bit seed, modified.
   *
   *   RETURN VALUE:
   *
   *    random bits in iBits LSB's.
   *
   *
   *   IMPLEMENTATION:
   *
   *    implementation of x**31 + x**3 + 1 == PN_XOR_REG | PN_XOR_ADD a
   *    PN sequence generator using int32_ts generating a 2**31 -1
   *    length pn-sequence.
   *
   *************************************************************************/

  int16_t Codec::getPnBits(int iBits, int32_t *pL_PNSeed)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swPnBits = 0;
    int32_t L_Taps,
           L_FeedBack;
    int    i;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    for (i = 0; i < iBits; i++)
    {
      /* update the state */
      /* ---------------- */

      L_Taps = *pL_PNSeed & PN_XOR_REG;
      L_FeedBack = L_Taps;               /* Xor tap bits to yield
                                          * feedback bit */
      L_Taps = L_shr(L_Taps, 1);

      while (L_Taps)
      {
        L_FeedBack = L_FeedBack ^ L_Taps;
        L_Taps = L_shr(L_Taps, 1);
      }

      /* LSB of L_FeedBack is next MSB of PN register */

      *pL_PNSeed = L_shr(*pL_PNSeed, 1);
      if (L_FeedBack & 1)
        *pL_PNSeed = *pL_PNSeed | PN_XOR_ADD;

      /* State update complete.  Get the output bit from the state, add/or it
       * into output */

      swPnBits = shl(swPnBits, 1);
      swPnBits = swPnBits | (extract_l(*pL_PNSeed) & 0x0001);

    }
    return (swPnBits);
  }


  /*************************************************************************
   *
   *   FUNCTION NAME: rxInterpR0Lpc
   *
   *   PURPOSE:
   *
   *     Perform part of the comfort noise algorithm at the decoder.
   *     LPC and R0 are derived in this routine
   *
   *   INPUTS:
   *
   *     pswOldKs - Last frame's reflection coeffs.
   *
   *     pswNewKs - This frame's decoded/received reflection coeffs.
   *     This will serve a new endpoint in interpolation.
   *
   *     swRxDTXState - primary DTX state variable (at the receiver).  A
   *     modulo 12 counter, which is 0 at SID frame.
   *
   *     swDecoMode - actual mode the decoder: speech decoding mode
   *     or comfort noise insertion mode (SPEECH = speech decoding;
   *     CNIFIRSTSID = comfort noise, 1st SID received; CNICONT = comfort
   *     noise, SID frame received, but not 1st SID; CNIBFI = comfort
   *     noise, bad frame received)
   *
   *     swFrameType - type of the received frame (VALIDSID, INVALIDSID
   *     GOODSPEECH or UNUSABLE)
   *
   *     swOldR0Dec - global variable, the decoded R0 value from the last
   *     frame .  This will be modified.
   *
   *     swR0NewCN - global variable the decoded R0 value from the frame
   *     just received. Valid information if current frame is a SID frame.
   *
   *
   *   OUTPUTS:
   *
   *     pswNewKs - This frames LPC coeffs. modified to reflect
   *     interpolated correlation sequence pL_CorrSeq[].
   *
   *     swR0Dec - global variable, interpolated R0 value
   *
   *     swR0OldCN - global variable, R0 interpolation point to
   *     interpolate from.
   *
   *     swR0NewCN - global variable, R0 interpolation point to
   *     interpolate to.
   *
   *     pL_OldCorrSeq[NP+1] - global variable, starting point for
   *     interpolation of LPC information.
   *
   *     pL_NewCorrSeq[NP+1] - global variable, end point for
   *     interpolation of LPC information.
   *
   *     pL_CorrSeq[NP+1] - global variable, interpolated value of LPC
   *     information to be used in this frame.
   *
   *
   *   RETURN VALUE:
   *
   *     None.
   *
   *   KEYWORDS: interpolation, comfort noise, SID, DTX
   *
   *************************************************************************/

  void Codec::rxInterpR0Lpc(int16_t *pswOldKs, int16_t *pswNewKs,
                              int16_t swRxDTXState,
                              int16_t swDecoMode, int16_t swFrameType)
  {

  /*________________________________________________________________________
   |                                                                        |
   |                        Static Variables                                |
   |________________________________________________________________________|
  */



  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int    i;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    if (swDecoMode == CNIFIRSTSID)
    {
      /* first SID frame arrived */
      /* ----------------------- */

      /* use tx'd R0 frame as both endpoints of interp curve. */
      /* i.e. no interpolation for the first frames           */
      /* ---------------------------------------------------- */


      swR0OldCN = swOldR0Dec;            /* last non-SID, received R0 */
      swR0Dec = linInterpSidShort(swR0NewCN, swR0OldCN, swRxDTXState);


      /* generate the LPC end points for interpolation */
      /* --------------------------------------------- */

      rcToCorrDpL(ASHIFT, ASCALE, pswOldKs, pL_OldCorrSeq);
      rcToCorrDpL(ASHIFT, ASCALE, pswNewKs, pL_NewCorrSeq);

      /* linearly interpolate between the two sets of correlation coefs */
      /* -------------------------------------------------------------- */

      for (i = 0; i < NP + 1; i++)
      {
        pL_CorrSeq[i] = linInterpSid(pL_NewCorrSeq[i], pL_OldCorrSeq[i],
                                     swRxDTXState);
      }

      /* Generate this frames K's (overwrite input) */
      /* ------------------------------------------ */

      aFlatRcDp(pL_CorrSeq, pswNewKs);

    }
    else if ((swDecoMode == CNICONT) && (swFrameType == VALIDSID))
    {
      /* new (not the first) SID frame arrived */
      /* ------------------------------------- */

      swR0OldCN = swOldR0Dec;            /* move current state of R0 to old */
      swR0Dec = linInterpSidShort(swR0NewCN, swR0OldCN, swRxDTXState);


      /* LPC: generate new endpoints for interpolation */
      /* --------------------------------------------- */

      for (i = 0; i < NP + 1; i++)
      {
        pL_OldCorrSeq[i] = pL_CorrSeq[i];
      }

      rcToCorrDpL(ASHIFT, ASCALE, pswNewKs, pL_NewCorrSeq);


      /* linearly interpolate between the two sets of correlation coefs */
      /* -------------------------------------------------------------- */

      for (i = 0; i < NP + 1; i++)
      {
        pL_CorrSeq[i] = linInterpSid(pL_NewCorrSeq[i], pL_OldCorrSeq[i],
                                     swRxDTXState);
      }


      /* Use interpolated LPC for this frame, overwrite the input K's */
      /* ------------------------------------------------------------ */

      aFlatRcDp(pL_CorrSeq, pswNewKs);

    }
    else
    {
      /* in between SID frames / invalid SID frames */
      /* ------------------------------------------ */

      swR0Dec = linInterpSidShort(swR0NewCN, swR0OldCN, swRxDTXState);


      /* linearly interpolate between the two sets of correlation coefs */
      /* -------------------------------------------------------------- */

      for (i = 0; i < NP + 1; i++)
      {
        pL_CorrSeq[i] = linInterpSid(pL_NewCorrSeq[i], pL_OldCorrSeq[i],
                                     swRxDTXState);
      }


      /* Use interpolated LPC for this frame, overwrite the input K's */
      /* ------------------------------------------------------------ */

      aFlatRcDp(pL_CorrSeq, pswNewKs);

    }
  }


  /*************************************************************************
   *
   *   FUNCTION NAME: linInterpSid
   *
   *   PURPOSE:
   *
   *     Linearly interpolate between two input numbers based on what the
   *     current DtxState is.
   *
   *   INPUTS:
   *
   *     L_New - int32_t more current value
   *
   *     L_Old - int32_t oldest value
   *
   *     swDtxState - state is 0 at the transmitted SID Frame.
   *
   *
   *   OUTPUTS:
   *
   *     none
   *
   *   RETURN VALUE:
   *
   *     A value between old and new inputs with dtxState+1/12 of the new
   *     (dtxState+1)-12/12 of the old
   *
   *
   *************************************************************************/

  int32_t Codec::linInterpSid(int32_t L_New, int32_t L_Old, int16_t swDtxState)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swOldFactor;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* old factor = (1.0 - newFactor) */
    /* ------------------------------ */

    swOldFactor = sub(0x7fff, psrCNNewFactor[swDtxState]);
    swOldFactor = add(0x1, swOldFactor);


    /* contributions from new and old */
    /* ------------------------------ */

    L_New = L_mpy_ls(L_New, psrCNNewFactor[swDtxState]);
    L_Old = L_mpy_ls(L_Old, swOldFactor);

    return (L_add(L_New, L_Old));

  }


  /*************************************************************************
   *
   *   FUNCTION NAME: linInterpSidShort
   *
   *   PURPOSE:
   *
   *     Linearly interpolate between two input numbers based on what
   *     the current DtxState is.
   *
   *   INPUTS:
   *
   *     swNew - 16 bit,  more current value
   *
   *     swOld - 16 bit, oldest value
   *
   *     swDtxState - state is 0 at the transmitted SID Frame.
   *
   *
   *   OUTPUTS:
   *
   *     none
   *
   *   RETURN VALUE:
   *
   *     A value between old and new inputs with dtxState+1/12 of the new
   *     (dtxState+1)-12/12 of the old
   *
   *************************************************************************/

  int16_t Codec::linInterpSidShort(int16_t swNew, int16_t swOld,
                                     int16_t swDtxState)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swOldFactor;
    int32_t L_New,
           L_Old;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* old factor = (1.0 - newFactor) */
    /* ------------------------------ */

    swOldFactor = sub(0x7fff, psrCNNewFactor[swDtxState]);
    swOldFactor = add(0x1, swOldFactor);


    /* contributions from new and old */
    /* ------------------------------ */

    L_New = L_mult(swNew, psrCNNewFactor[swDtxState]);
    L_Old = L_mult(swOld, swOldFactor);


    return (round(L_add(L_New, L_Old)));

  }

  // sp_frm.c

  #define ASCALE  0x0800
  #define ASHIFT 4
  #define CG_INT_MACS     6
  #define CG_TERMS        (LSMAX - LSMIN + 1)
  #define CVSHIFT 2                      /* Number of right shifts to be
                                          * applied to the normalized Phi
                                          * array in cov32, also used in flat
                                          * to shift down normalized F, B, C
                                          * matrices.                        */
  #define C_FRAME_LEN     (N_SUB * CG_TERMS)
  #define DELTA_LEVELS    16
  #define G_FRAME_LEN     (LSMAX + (N_SUB-1) * S_LEN - LSMIN  + 1)
  #define HIGH 1
  #define INV_OS_FCTR     0x1555         /* 1.0/6.0 */
  #define LAG_TABLE_LEN   (1 << L_BITS)
  #define LMAX            142
  #define LMAX_FR         (LMAX * OS_FCTR)
  #define LMIN            21
  #define LMIN_FR         (LMIN * OS_FCTR)
  #define LOW 0
  #define LPC_VQ_SEG 3
  #define LSMAX           (LMAX + CG_INT_MACS/2)
  #define LSMIN           (LMIN - CG_INT_MACS/2)
  #define LSP_MASK  0xffff
  #define L_BITS          8
  #define L_ROUND (int32_t)0x8000       /* Preload accumulator value for
                                          * rounding  */
  #define NP_AFLAT     4
  #define NUM_CLOSED      3
  #define NUM_TRAJ_MAX    2
  #define ONE_EIGHTH      0x1000
  #define ONE_HALF        0x4000
  #define ONE_QUARTER     0x2000
  #define PEAK_VICINITY   3
  #define PGAIN_CLAMP    0x0021          /* 0.001 */
  #define PGAIN_SCALE    0x6000          /* 0.75 */
  #define PW_FRAC         0x3333         /* 0.4 */
  #define R0BITS 5
  #define RSHIFT  2
  #define S_SH    6                      /* Shift offset for computing frame
                                          * energy */
  #define UV_SCALE0       -0x2976
  #define UV_SCALE1       -0x46d3
  #define UV_SCALE2       -0x6676
  #define W_F_BUFF_LEN  (F_LEN + LSMAX)
  #define high(x) (shr(x,8) & 0x00ff)
  #define low(x) x & 0x00ff              /* This macro will return the low
                                          * byte of a word */
  #define odd(x) (x & 0x0001)            /* This macro will determine if an
                                          * integer is odd */


  /***************************************************************************
   *
   *    FUNCTION NAME: aflat
   *
   *    PURPOSE:  Given a vector of high-pass filtered input speech samples
   *              (A_LEN samples), function aflat computes the NP unquantized
   *              reflection coefficients using the FLAT algorithm, searches
   *              the three segment Rc-VQ based on the AFLAT recursion, and
   *              outputs a quantized set of NP reflection coefficients, along
   *              with the three indices specifying the selected vectors
   *              from the Rc-VQ. The index of the quantized frame energy R0
   *              is also output.
   *
   *
   *    INPUT:
   *
   *        pswSpeechToLpc[0:A_LEN-1]
   *                     A vector of high-pass filtered input speech, from
   *                     which the unquantized reflection coefficients and
   *                     the index of the quantized frame energy are
   *                     computed.
   *
   *    OUTPUTS:
   *
   *        piR0Index[0:0]
   *                     An index into a 5 bit table of quantized frame
   *                     energies.
   *
   *        pswFinalRc[0:NP-1]
   *                     A quantized set of NP reflection coefficients.
   *
   *        piVQCodewds[0:2]
   *                     An array containing the indices of the 3 reflection
   *                     coefficient vectors selected from the three segment
   *                     Rc-VQ.
   *
   *        swPtch
   *                     Flag to indicate a periodic signal component
   *
   *        pswVadFlag
   *                     Voice activity decision flag
   *                      = 1: voice activity
   *                      = 0: no voice activity
   *
   *        pswSP
   *                     Speech flag
   *                      = 1: encoder generates speech frames
   *                      = 0: encoder generate SID frames
   *
   *
   *    RETURN:
   *        None.
   *
   *    REFERENCE:  Sub-clauses 4.1.3, 4.1.4, and 4.1.4.1
   *        of GSM Recommendation 06.20
   *
   *    KEYWORDS: AFLAT,aflat,flat,vectorquantization, reflectioncoefficients
   *
   *************************************************************************/

    void Codec::aflat(int16_t pswSpeechToLPC[],
                        int piR0Index[],
                        int16_t pswFinalRc[],
                        int piVQCodewds[],
                        int16_t swPtch,
                        int16_t *pswVadFlag,
                        int16_t *pswSP)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t pswPOldSpace[NP_AFLAT],
           pswPNewSpace[NP_AFLAT],
           pswVOldSpace[2 * NP_AFLAT - 1],
           pswVNewSpace[2 * NP_AFLAT - 1],
          *ppswPAddrs[2],
          *ppswVAddrs[2],
          *pswVBar,
           pswPBar[NP_AFLAT],
           pswVBarSpace[2 * NP_AFLAT - 1],
           pswFlatsRc[NP],               /* Unquantized Rc's computed by FLAT */
           pswRc[NP + 1];                /* Temp list for the converted RC's */
    int32_t pL_CorrelSeq[NP + 1],
          *pL_VBarFull,
           pL_PBarFull[NP],
           pL_VBarFullSpace[2 * NP - 1];

    int    i,
           iVec,
           iSeg,
           iCnt;                         /* Loop counter */
    struct QuantList quantList,          /* A list of vectors */
           bestPql[4];                   /* The four best vectors from the
                                          * PreQ */
    struct QuantList bestQl[LPC_VQ_SEG + 1];      /* Best vectors for each of
                                                   * the three segments */
    int16_t swVadScalAuto;
    int16_t pswVadRc[4];
    int32_t pL_VadAcf[9];

    int32_t L_R0;                       /* Normalized R0 (use swRShifts to
                                          * unnormalize). This is done prior
                                          * to r0quant(). After this, its is
                                          * a unnormalized number */

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Setup pointers temporary space */
    /*--------------------------------*/

    pswVBar = pswVBarSpace + NP_AFLAT - 1;
    pL_VBarFull = pL_VBarFullSpace + NP - 1;
    ppswPAddrs[0] = pswPOldSpace;
    ppswPAddrs[1] = pswPNewSpace;
    ppswVAddrs[0] = pswVOldSpace + NP_AFLAT - 1;
    ppswVAddrs[1] = pswVNewSpace + NP_AFLAT - 1;

    /* Given the input speech, compute the optimal reflection coefficients */
    /* using the FLAT algorithm.                                           */
    /*---------------------------------------------------------------------*/

    L_R0 = flat(pswSpeechToLPC, pswFlatsRc, piR0Index, pL_VadAcf,
                &swVadScalAuto);

    /* Get unquantized reflection coefficients for VAD */      /* DTX mode */
    /* algorithm                                       */      /* DTX mode */
    /* ----------------------------------------------- */      /* DTX mode */

    for (i = 0; i < 4; i++)                                    /* DTX mode */
      pswVadRc[i] = pswFlatsRc[i];                             /* DTX mode */


    /* convert reflection coefficients to correlation */       /* DTX mode */
    /* sequence                                       */       /* DTX mode */
    /* ---------------------------------------------- */       /* DTX mode */

    rcToCorrDpL(ASHIFT, ASCALE, pswFlatsRc, pL_CorrelSeq);     /* DTX mode */


    /* Make the voice activity detection. Only swVadFlag is */ /* DTX mode */
    /*  modified.                                           */ /* DTX mode */
    /* ---------------------------------------------------- */ /* DTX mode */

    vad_algorithm(pL_VadAcf, swVadScalAuto, pswVadRc, swPtch,  /* DTX mode */
                  pswVadFlag);


    /* if DTX mode off, then always voice activity */          /* DTX mode */
    /* ------------------------------------------- */          /* DTX mode */
    if (!giDTXon) *pswVadFlag = 1;                             /* DTX mode */


    /* determination of comfort noise parameters */            /* DTX mode */
    /* ----------------------------------------- */            /* DTX mode */

    *pswSP = swComfortNoise(*pswVadFlag,                       /* DTX mode */
                            L_R0,                              /* DTX mode */
                            pL_CorrelSeq);                     /* DTX mode */

    if (*pswSP == 0)                                           /* DTX mode */
    {   /* SID frame generation */                             /* DTX mode */

      /* use unquantized reflection coefficients in the */     /* DTX mode */
      /* encoder, when SID frames are generated         */     /* DTX mode */
      /* ---------------------------------------------- */     /* DTX mode */

      for (i = 0; i < NP; i++)                                 /* DTX mode */
        pswFinalRc[i] = pswFlatsRc[i];                         /* DTX mode */

    }                                                          /* DTX mode */
    else                                                       /* DTX mode */
    { /* speech frame generation */

      /* Set up pL_PBarFull and pL_VBarFull initial conditions, using the   */
      /* autocorrelation sequence derived from the optimal reflection       */
      /* coefficients computed by FLAT. The initial conditions are shifted  */
      /* right by RSHIFT bits. These initial conditions, stored as          */
      /* int32_ts, are used to initialize PBar and VBar arrays for the     */
      /* next VQ segment.                                                   */
      /*--------------------------------------------------------------------*/

      initPBarFullVBarFullL(pL_CorrelSeq, pL_PBarFull, pL_VBarFull);

      /* Set up initial PBar and VBar initial conditions, using pL_PBarFull */
      /* and pL_VBarFull arrays initialized above. These are the initial    */
      /* PBar and VBar conditions to be used by the AFLAT recursion at the  */
      /* 1-st Rc-VQ segment.                                                */
      /*--------------------------------------------------------------------*/

      initPBarVBarL(pL_PBarFull, pswPBar, pswVBar);

      for (iSeg = 1; iSeg <= LPC_VQ_SEG; iSeg++)
      {

        /* initialize candidate list */
        /*---------------------------*/

        quantList.iNum = psrPreQSz[iSeg - 1];
        quantList.iRCIndex = 0;

        /* do aflat for all vectors in the list */
        /*--------------------------------------*/

        setupPreQ(iSeg, quantList.iRCIndex);        /* set up vector ptrs */

        for (iCnt = 0; iCnt < quantList.iNum; iCnt++)
        {
          /* get a vector */
          /*--------------*/

          getNextVec(pswRc);

          /* clear the limiter flag */
          /*------------------------*/

          iLimit = 0;

          /* find the error values for each vector */
          /*---------------------------------------*/

          quantList.pswPredErr[iCnt] =
                  aflatRecursion(&pswRc[psvqIndex[iSeg - 1].l],
                                 pswPBar, pswVBar,
                                 ppswPAddrs, ppswVAddrs,
                                 psvqIndex[iSeg - 1].len);

          /* check the limiter flag */
          /*------------------------*/

          if (iLimit)
          {
            quantList.pswPredErr[iCnt] = 0x7fff;    /* set error to bad value */
          }

        }                                  /* done list loop */

        /* find 4 best prequantizer levels */
        /*---------------------------------*/

        findBestInQuantList(quantList, 4, bestPql);

        for (iVec = 0; iVec < 4; iVec++)
        {

          /* initialize quantizer list */
          /*---------------------------*/

          quantList.iNum = psrQuantSz[iSeg - 1];
          quantList.iRCIndex = bestPql[iVec].iRCIndex * psrQuantSz[iSeg - 1];

          setupQuant(iSeg, quantList.iRCIndex);     /* set up vector ptrs */

          /* do aflat recursion on each element of list */
          /*--------------------------------------------*/

          for (iCnt = 0; iCnt < quantList.iNum; iCnt++)
          {

            /* get a vector */
            /*--------------*/

            getNextVec(pswRc);

            /* clear the limiter flag */
            /*------------------------*/

            iLimit = 0;

            /* find the error values for each vector */
            /*---------------------------------------*/

            quantList.pswPredErr[iCnt] =
                    aflatRecursion(&pswRc[psvqIndex[iSeg - 1].l],
                                   pswPBar, pswVBar,
                                   ppswPAddrs, ppswVAddrs,
                                   psvqIndex[iSeg - 1].len);

            /* check the limiter flag */
            /*------------------------*/

            if (iLimit)
            {
              quantList.pswPredErr[iCnt] = 0x7fff;  /* set error to the worst
                                                     * value */
            }

          }                                /* done list loop */

          /* find best quantizer vector for this segment, and save it */
          /*----------------------------------------------------------*/

          findBestInQuantList(quantList, 1, bestQl);
          if (iVec == 0)
          {
            bestQl[iSeg] = bestQl[0];
          }
          else
          {
            if (sub(bestQl[iSeg].pswPredErr[0],
                    bestQl[0].pswPredErr[0]) > 0)
            {
              bestQl[iSeg] = bestQl[0];
            }
          }
        }

        /* find the quantized reflection coefficients */
        /*--------------------------------------------*/

        setupQuant(iSeg, bestQl[iSeg].iRCIndex);    /* set up vector ptrs */
        getNextVec((int16_t *) (pswFinalRc - 1));


        /* Update pBarFull and vBarFull for the next Rc-VQ segment, and */
        /* update the pswPBar and pswVBar for the next Rc-VQ segment    */
        /*--------------------------------------------------------------*/

        if (iSeg < LPC_VQ_SEG)
        {

          aflatNewBarRecursionL(&pswFinalRc[psvqIndex[iSeg - 1].l - 1], iSeg,
                                pL_PBarFull, pL_VBarFull, pswPBar, pswVBar);

        }

      }

      /* find the quantizer index (the values */
      /* to be output in the symbol file)     */
      /*--------------------------------------*/

      for (iSeg = 1; iSeg <= LPC_VQ_SEG; iSeg++)
      {
        piVQCodewds[iSeg - 1] = bestQl[iSeg].iRCIndex;
      }

    }

  }

  /***************************************************************************
   *
   *    FUNCTION NAME: aflatNewBarRecursionL
   *
   *    PURPOSE:  Given the int32_t initial condition arrays, pL_PBarFull and
   *              pL_VBarFull, a reflection coefficient vector selected from
   *              the Rc-VQ at the current stage, and index of the current
   *              Rc-VQ stage, the AFLAT recursion is evaluated to obtain the
   *              updated initial conditions for the AFLAT recursion at the
   *              next Rc-VQ stage. At each lattice stage the pL_PBarFull and
   *              pL_VBarFull arrays are shifted to be RSHIFT down from full
   *              scale. Two sets of initial conditions are output:
   *
   *              1) pswPBar and pswVBar int16_t arrays are used at the
   *                 next Rc-VQ segment as the AFLAT initial conditions
   *                 for the Rc prequantizer and the Rc quantizer searches.
   *              2) pL_PBarFull and pL_VBarFull arrays are output and serve
   *                 as the initial conditions for the function call to
   *                 aflatNewBarRecursionL at the next lattice stage.
   *
   *
   *              This is an implementation of equations 4.24 through
   *              4.27.
   *    INPUTS:
   *
   *        pswQntRc[0:NP_AFLAT-1]
   *                     An input reflection coefficient vector selected from
   *                     the Rc-VQ quantizer at the current stage.
   *
   *        iSegment
   *                    An input describing the current Vector quantizer
   *                    quantizer segment (1, 2, or 3).
   *
   *        RSHIFT      The number of shifts down from full scale the
   *                     pL_PBarFull and pL_VBarFull arrays are to be shifted
   *                     at each lattice stage. RSHIFT is a global constant.
   *
   *        pL_PBar[0:NP-1]
   *                     A int32_t input array containing the P initial
   *                     conditions for the full 10-th order LPC filter.
   *                     The address of the 0-th element of  pL_PBarFull
   *                     is passed in when function aflatNewBarRecursionL
   *                     is called.
   *
   *        pL_VBar[-NP+1:NP-1]
   *                     A int32_t input array containing the V initial
   *                     conditions for the full 10-th order LPC filter.
   *                     The address of the 0-th element of  pL_VBarFull
   *                     is passed in when function aflatNewBarRecursionL
   *                     is called.
   *
   *    OUTPUTS:
   *
   *        pL_PBar[0:NP-1]
   *                     A int32_t output array containing the updated P
   *                     initial conditions for the full 10-th order LPC
   *                     filter.
   *
   *        pL_VBar[-NP+1:NP-1]
   *                     A int32_t output array containing the updated V
   *                     initial conditions for the full 10-th order LPC
   *                     filter.
   *
   *        pswPBar[0:NP_AFLAT-1]
   *                     An output int16_t array containing the P initial
   *                     conditions for the P-V AFLAT recursion for the next
   *                     Rc-VQ segment. The address of the 0-th element of
   *                     pswVBar is passed in.
   *
   *        pswVBar[-NP_AFLAT+1:NP_AFLAT-1]
   *                     The output int16_t array containing the V initial
   *                     conditions for the P-V AFLAT recursion, for the next
   *                     Rc-VQ segment. The address of the 0-th element of
   *                     pswVBar is passed in.
   *
   *    RETURN:
   *        None.
   *
   *    REFERENCE:  Sub-clause 4.1.4.1 GSM Recommendation 06.20
   *
   *************************************************************************/

  void Codec::aflatNewBarRecursionL(int16_t pswQntRc[], int iSegment,
                                      int32_t pL_PBar[], int32_t pL_VBar[],
                                   int16_t pswPBar[], int16_t pswVBar[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */
    int32_t *pL_VOld,
          *pL_VNew,
          *pL_POld,
          *pL_PNew,
          *ppL_PAddrs[2],
          *ppL_VAddrs[2],
           pL_VOldSpace[2 * NP - 1],
           pL_VNewSpace[2 * NP - 1],
           pL_POldSpace[NP],
           pL_PNewSpace[NP],
           L_temp,
           L_sum;
    int16_t swQntRcSq,
           swNShift;
    short int i,
           j,
           bound;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */
    /* Copy the addresses of the input PBar and VBar arrays into  */
    /* pL_POld and pL_VOld respectively.                          */
    /*------------------------------------------------------------*/

    pL_POld = pL_PBar;
    pL_VOld = pL_VBar;

    /* Point to PNew and VNew temporary arrays */
    /*-----------------------------------------*/

    pL_PNew = pL_PNewSpace;
    pL_VNew = pL_VNewSpace + NP - 1;

    /* Load the addresses of the temporary buffers into the address arrays. */
    /* The address arrays are used to swap PNew and POld (VNew and VOLd)    */
    /* buffers to avoid copying of the buffer contents at the end of a      */
    /* lattice filter stage.                                                */
    /*----------------------------------------------------------------------*/

    ppL_PAddrs[0] = pL_POldSpace;
    ppL_PAddrs[1] = pL_PNewSpace;
    ppL_VAddrs[0] = pL_VOldSpace + NP - 1;
    ppL_VAddrs[1] = pL_VNewSpace + NP - 1;


    /* Update AFLAT recursion initial conditions for searching the Rc vector */
    /* quantizer at the next VQ segment.                                     */
    /*-------------------------------------------------------------------*/

    for (j = 0; j < psvqIndex[iSegment - 1].len; j++)
    {
      bound = NP - psvqIndex[iSegment - 1].l - j - 1;

      /* Compute rc squared, used by the recursion at the j-th lattice stage. */
      /*---------------------------------------------------------------------*/

      swQntRcSq = mult_r(pswQntRc[j], pswQntRc[j]);

      /* Calculate PNew(i) */
      /*-------------------*/

      L_temp = L_mpy_ls(pL_VOld[0], pswQntRc[j]);
      L_sum = L_add(L_temp, pL_POld[0]);
      L_temp = L_mpy_ls(pL_POld[0], swQntRcSq);
      L_sum = L_add(L_temp, L_sum);
      L_temp = L_mpy_ls(pL_VOld[0], pswQntRc[j]);
      L_temp = L_add(L_sum, L_temp);

      /* Compute the number of bits to shift left by to achieve  */
      /* the nominal value of PNew[0] which is right shifted by  */
      /* RSHIFT bits relative to full scale.                     */
      /*---------------------------------------------------------*/

      swNShift = sub(norm_s(extract_h(L_temp)), RSHIFT);

      /* Rescale PNew[0] by shifting left by swNShift bits */
      /*---------------------------------------------------*/

      pL_PNew[0] = L_shl(L_temp, swNShift);

      for (i = 1; i <= bound; i++)
      {
        L_temp = L_mpy_ls(pL_VOld[i], pswQntRc[j]);
        L_sum = L_add(L_temp, pL_POld[i]);
        L_temp = L_mpy_ls(pL_POld[i], swQntRcSq);
        L_sum = L_add(L_temp, L_sum);
        L_temp = L_mpy_ls(pL_VOld[-i], pswQntRc[j]);
        L_temp = L_add(L_sum, L_temp);
        pL_PNew[i] = L_shl(L_temp, swNShift);
      }

      /* Calculate VNew(i) */
      /*-------------------*/

      for (i = -bound; i < 0; i++)
      {
        L_temp = L_mpy_ls(pL_VOld[-i - 1], swQntRcSq);
        L_sum = L_add(L_temp, pL_VOld[i + 1]);
        L_temp = L_mpy_ls(pL_POld[-i - 1], pswQntRc[j]);
        L_temp = L_shl(L_temp, 1);
        L_temp = L_add(L_temp, L_sum);
        pL_VNew[i] = L_shl(L_temp, swNShift);
      }
      for (i = 0; i <= bound; i++)
      {
        L_temp = L_mpy_ls(pL_VOld[-i - 1], swQntRcSq);
        L_sum = L_add(L_temp, pL_VOld[i + 1]);
        L_temp = L_mpy_ls(pL_POld[i + 1], pswQntRc[j]);
        L_temp = L_shl(L_temp, 1);
        L_temp = L_add(L_temp, L_sum);
        pL_VNew[i] = L_shl(L_temp, swNShift);
      }

      if (j < psvqIndex[iSegment - 1].len - 2)
      {

        /* Swap POld and PNew buffers, using modulo addressing */
        /*-----------------------------------------------------*/

        pL_POld = ppL_PAddrs[(j + 1) % 2];
        pL_PNew = ppL_PAddrs[j % 2];

        /* Swap VOld and VNew buffers, using modulo addressing */
        /*-----------------------------------------------------*/

        pL_VOld = ppL_VAddrs[(j + 1) % 2];
        pL_VNew = ppL_VAddrs[j % 2];
      }
      else
      {
        if (j == psvqIndex[iSegment - 1].len - 2)
        {

          /* Then recursion to be done for one more lattice stage */
          /*------------------------------------------------------*/

          /* Copy address of PNew into POld */
          /*--------------------------------*/
          pL_POld = ppL_PAddrs[(j + 1) % 2];

          /* Copy address of the input pL_PBar array into pswPNew; this will */
          /* cause the PNew array to overwrite the input pL_PBar array, thus */
          /* updating it at the final lattice stage of the current segment   */
          /*-----------------------------------------------------------------*/

          pL_PNew = pL_PBar;

          /* Copy address of VNew into VOld */
          /*--------------------------------*/

          pL_VOld = ppL_VAddrs[(j + 1) % 2];

          /* Copy address of the input pL_VBar array into pswVNew; this will */
          /* cause the VNew array to overwrite the input pL_VBar array, thus */
          /* updating it at the final lattice stage of the current segment   */
          /*-----------------------------------------------------------------*/

          pL_VNew = pL_VBar;

        }
      }
    }

    /* Update the pswPBar and pswVBar initial conditions for the AFLAT      */
    /* Rc-VQ search at the next segment.                                    */
    /*----------------------------------------------------------------------*/

    bound = psvqIndex[iSegment].len - 1;

    for (i = 0; i <= bound; i++)
    {
      pswPBar[i] = round(pL_PBar[i]);
      pswVBar[i] = round(pL_VBar[i]);
    }
    for (i = -bound; i < 0; i++)
    {
      pswVBar[i] = round(pL_VBar[i]);
    }

    return;
  }

  /***************************************************************************
   *
   *    FUNCTION NAME: aflatRecursion
   *
   *    PURPOSE:  Given the int16_t initial condition arrays, pswPBar and
   *              pswVBar, a reflection coefficient vector from the quantizer
   *              (or a prequantizer), and the order of the current Rc-VQ
   *              segment, function aflatRecursion computes and returns the
   *              residual error energy by evaluating the AFLAT recursion.
   *
   *              This is an implementation of equations 4.18 to 4.23.
   *    INPUTS:
   *
   *        pswQntRc[0:NP_AFLAT-1]
   *                     An input reflection coefficient vector from the
   *                     Rc-prequantizer or the Rc-VQ codebook.
   *
   *        pswPBar[0:NP_AFLAT-1]
   *                     The input int16_t array containing the P initial
   *                     conditions for the P-V AFLAT recursion at the current
   *                     Rc-VQ segment. The address of the 0-th element of
   *                     pswVBar is passed in.
   *
   *        pswVBar[-NP_AFLAT+1:NP_AFLAT-1]
   *                     The input int16_t array containing the V initial
   *                     conditions for the P-V AFLAT recursion, at the current
   *                     Rc-VQ segment. The address of the 0-th element of
   *                     pswVBar is passed in.
   *
   *        *ppswPAddrs[0:1]
   *                     An input array containing the address of temporary
   *                     space P1 in its 0-th element, and the address of
   *                     temporary space P2 in its 1-st element. Each of
   *                     these addresses is alternately assigned onto
   *                     pswPNew and pswPOld pointers using modulo
   *                     arithmetic, so as to avoid copying the contents of
   *                     pswPNew array into the pswPOld array at the end of
   *                     each lattice stage of the AFLAT recursion.
   *                     Temporary space P1 and P2 is allocated outside
   *                     aflatRecursion by the calling function aflat.
   *
   *        *ppswVAddrs[0:1]
   *                     An input array containing the address of temporary
   *                     space V1 in its 0-th element, and the address of
   *                     temporary space V2 in its 1-st element. Each of
   *                     these addresses is alternately assigned onto
   *                     pswVNew and pswVOld pointers using modulo
   *                     arithmetic, so as to avoid copying the contents of
   *                     pswVNew array into the pswVOld array at the end of
   *                     each lattice stage of the AFLAT recursion.
   *                     Temporary space V1 and V2 is allocated outside
   *                     aflatRecursion by the calling function aflat.
   *
   *        swSegmentOrder
   *                     This input short word describes the number of
   *                     stages needed to compute the vector
   *                     quantization of the given segment.
   *
   *    OUTPUTS:
   *        None.
   *
   *    RETURN:
   *        swRe         The int16_t value of residual energy for the
   *                     Rc vector, given the pswPBar and pswVBar initial
   *                     conditions.
   *
   *    REFERENCE:  Sub-clause 4.1.4.1 GSM Recommendation 06.20
   *
   *************************************************************************/

  int16_t Codec::aflatRecursion(int16_t pswQntRc[],
                                  int16_t pswPBar[],
                                  int16_t pswVBar[],
                                  int16_t *ppswPAddrs[],
                                  int16_t *ppswVAddrs[],
                                  int16_t swSegmentOrder)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t *pswPOld,
          *pswPNew,
          *pswVOld,
          *pswVNew,
           pswQntRcSqd[NP_AFLAT],
           swRe;
    int32_t L_sum;
    short int i,
           j,
           bound;                        /* loop control variables */

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Point to PBar and VBar, the initial condition arrays for the AFLAT  */
    /* recursion.                                                          */
    /*---------------------------------------------------------------------*/

    pswPOld = pswPBar;
    pswVOld = pswVBar;

    /* Point to PNew and VNew, the arrays into which updated values of  P  */
    /* and V functions will be written.                                    */
    /*---------------------------------------------------------------------*/

    pswPNew = ppswPAddrs[1];
    pswVNew = ppswVAddrs[1];

    /* Compute the residual error energy due to the selected Rc vector */
    /* using the AFLAT recursion.                                      */
    /*-----------------------------------------------------------------*/

    /* Compute rc squared, used by the recursion */
    /*-------------------------------------------*/

    for (j = 0; j < swSegmentOrder; j++)
    {
      pswQntRcSqd[j] = mult_r(pswQntRc[j], pswQntRc[j]);
    }

    /* Compute the residual error energy due to the selected Rc vector */
    /* using the AFLAT recursion.                                      */
    /*-----------------------------------------------------------------*/

    for (j = 0; j < swSegmentOrder - 1; j++)
    {
      bound = swSegmentOrder - j - 2;

      /* Compute Psubj(i), for i = 0, bound  */
      /*-------------------------------------*/

      for (i = 0; i <= bound; i++)
      {
        L_sum = L_mac(L_ROUND, pswVOld[i], pswQntRc[j]);
        L_sum = L_mac(L_sum, pswVOld[-i], pswQntRc[j]);
        L_sum = L_mac(L_sum, pswPOld[i], pswQntRcSqd[j]);
        L_sum = L_msu(L_sum, pswPOld[i], SW_MIN);
        pswPNew[i] = extract_h(L_sum);
      }

      /* Check if potential for limiting exists. */
      /*-----------------------------------------*/

      if (sub(pswPNew[0], 0x4000) >= 0)
        iLimit = 1;

      /* Compute the new Vsubj(i) */
      /*--------------------------*/

      for (i = -bound; i < 0; i++)
      {
        L_sum = L_msu(L_ROUND, pswVOld[i + 1], SW_MIN);
        L_sum = L_mac(L_sum, pswQntRcSqd[j], pswVOld[-i - 1]);
        L_sum = L_mac(L_sum, pswQntRc[j], pswPOld[-i - 1]);
        L_sum = L_mac(L_sum, pswQntRc[j], pswPOld[-i - 1]);
        pswVNew[i] = extract_h(L_sum);
      }

      for (i = 0; i <= bound; i++)
      {
        L_sum = L_msu(L_ROUND, pswVOld[i + 1], SW_MIN);
        L_sum = L_mac(L_sum, pswQntRcSqd[j], pswVOld[-i - 1]);
        L_sum = L_mac(L_sum, pswQntRc[j], pswPOld[i + 1]);
        L_sum = L_mac(L_sum, pswQntRc[j], pswPOld[i + 1]);
        pswVNew[i] = extract_h(L_sum);
      }

      if (j < swSegmentOrder - 2)
      {

        /* Swap POld and PNew buffers, using modulo addressing */
        /*-----------------------------------------------------*/

        pswPOld = ppswPAddrs[(j + 1) % 2];
        pswPNew = ppswPAddrs[j % 2];

        /* Swap VOld and VNew buffers, using modulo addressing */
        /*-----------------------------------------------------*/

        pswVOld = ppswVAddrs[(j + 1) % 2];
        pswVNew = ppswVAddrs[j % 2];

      }
    }

    /* Computing Psubj(0) for the last lattice stage */
    /*-----------------------------------------------*/

    j = swSegmentOrder - 1;

    L_sum = L_mac(L_ROUND, pswVNew[0], pswQntRc[j]);
    L_sum = L_mac(L_sum, pswVNew[0], pswQntRc[j]);
    L_sum = L_mac(L_sum, pswPNew[0], pswQntRcSqd[j]);
    L_sum = L_msu(L_sum, pswPNew[0], SW_MIN);
    swRe = extract_h(L_sum);

    /* Return the residual energy corresponding to the reflection   */
    /* coefficient vector being evaluated.                          */
    /*--------------------------------------------------------------*/

    return (swRe);                       /* residual error is returned */

  }

  /***************************************************************************
   *
   *   FUNCTION NAME: bestDelta
   *
   *   PURPOSE:
   *
   *     This function finds the delta-codeable lag which maximizes CC/G.
   *
   *   INPUTS:
   *
   *     pswLagList[0:siNumLags-1]
   *
   *                     List of delta-codeable lags over which search is done.
   *
   *     pswCSfrm[0:127]
   *
   *                     C(k) sequence, k integer.
   *
   *     pswGSfrm[0:127]
   *
   *                     G(k) sequence, k integer.
   *
   *     siNumLags
   *
   *                     Number of lags in contention.
   *
   *     siSfrmIndex
   *
   *                     The index of the subframe to which the delta-code
   *                     applies.
   *
   *
   *   OUTPUTS:
   *
   *     pswLTraj[0:3]
   *
   *                     The winning lag is put into this array at
   *                     pswLTraj[siSfrmIndex]
   *
   *     pswCCTraj[0:3]
   *
   *                     The corresponding winning C**2 is put into this
   *                     array at pswCCTraj[siSfrmIndex]
   *
   *     pswGTraj[0:3]
   *
   *                     The corresponding winning G is put into this arrray
   *                     at pswGTraj[siSfrmIndex]
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *   REFERENCE:  Sub-clause 4.1.8.3 of GSM Recommendation 06.20
   *
   *   KEYWORDS:
   *
   *************************************************************************/

  void Codec::bestDelta(int16_t pswLagList[],
                          int16_t pswCSfrm[],
                          int16_t pswGSfrm[],
                          short int siNumLags,
                          short int siSfrmIndex,
                          int16_t pswLTraj[],
                          int16_t pswCCTraj[],
                          int16_t pswGTraj[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t pswCBuf[DELTA_LEVELS + CG_INT_MACS + 2],
           pswGBuf[DELTA_LEVELS + CG_INT_MACS + 2],
           pswCInterp[DELTA_LEVELS + 2],
           pswGInterp[DELTA_LEVELS + 2],
          *psw1,
          *psw2,
           swCmaxSqr,
           swGmax,
           swPeak;
    short int siIPLo,
           siRemLo,
           siIPHi,
           siRemHi,
           siLoLag,
           siHiLag,
           siI;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* get bounds for integer C's and G's needed for interpolation */
    /* get integer and fractional portions of boundary lags        */
    /* ----------------------------------------------------------- */

    get_ipjj(pswLagList[0], &siIPLo, &siRemLo);

    get_ipjj(pswLagList[siNumLags - 1], &siIPHi, &siRemHi);

    /* get lag for first and last C and G required */
    /* ------------------------------------------- */

    siLoLag = sub(siIPLo, CG_INT_MACS / 2 - 1);

    if (siRemHi != 0)
    {
      siHiLag = add(siIPHi, CG_INT_MACS / 2);
    }
    else
    {
      siHiLag = add(siIPHi, CG_INT_MACS / 2 - 1);
    }

    /* transfer needed integer C's and G's to temp buffers */
    /* --------------------------------------------------- */

    psw1 = pswCBuf;
    psw2 = pswGBuf;

    if (siRemLo == 0)
    {

      /* first lag in list is integer: don't care about first entries */
      /* (they will be paired with zero tap in interpolating filter)  */
      /* ------------------------------------------------------------ */

      psw1[0] = 0;
      psw2[0] = 0;
      psw1 = &psw1[1];
      psw2 = &psw2[1];
    }

    for (siI = siLoLag; siI <= siHiLag; siI++)
    {
      psw1[siI - siLoLag] = pswCSfrm[siI - LSMIN];
      psw2[siI - siLoLag] = pswGSfrm[siI - LSMIN];
    }

    if (siRemLo == 0)
    {
      /* make siLoLag correspond to first entry in temp buffers */
      /* ------------------------------------------------------ */
      siLoLag = sub(siLoLag, 1);
    }

    /* interpolate to get C's and G's which correspond to lags in list */
    /* --------------------------------------------------------------- */

    CGInterp(pswLagList, siNumLags, pswCBuf, pswGBuf, siLoLag,
             pswCInterp, pswGInterp);

    /* find max C*C*sgn(C)/G */
    /* --------------------- */

    swPeak = maxCCOverGWithSign(pswCInterp, pswGInterp, &swCmaxSqr, &swGmax,
                                siNumLags);

    /* store best lag and corresponding C*C and G */
    /* ------------------------------------------ */

    pswLTraj[siSfrmIndex] = pswLagList[swPeak];
    pswCCTraj[siSfrmIndex] = swCmaxSqr;
    pswGTraj[siSfrmIndex] = swGmax;

  }


  /***************************************************************************
   *
   *   FUNCTION NAME: CGInterp
   *
   *   PURPOSE:
   *
   *     Given a list of fractional lags, a C(k) array, and a G(k) array
   *     (k integer), this function generates arrays of C's and G's
   *     corresponding to the list of fractional lags by interpolating the
   *     integer C(k) and G(k) arrays.
   *
   *   INPUTS:
   *
   *     pswLIn[0:siNum-1]
   *
   *                     List of valid lags
   *
   *     siNum
   *
   *                     Length of output lists
   *
   *     pswCIn[0:variable]
   *
   *                     C(k) sequence, k integer.  The zero index corresponds
   *                     to k = siLoIntLag.
   *
   *     pswGIn[0:variable]
   *
   *                     G(k) sequence, k integer.  The zero index corresponds
   *                     to k = siLoIntLag.
   *
   *     siLoIntLag
   *
   *                     Integer lag corresponding to the first entry in the
   *                     C(k) and G(k) input arrays.
   *
   *     ppsrCGIntFilt[0:5][0:5]
   *
   *                     The FIR interpolation filter for C's and G's.
   *
   *   OUTPUTS:
   *
   *     pswCOut[0:siNum-1]
   *
   *                     List of interpolated C's corresponding to pswLIn.
   *
   *     pswGOut[0:siNum-1]
   *
   *                     List of interpolated G's corresponding to pswLIn
   *
   *   RETURN VALUE: none
   *
   *   DESCRIPTION:
   *
   *
   *   REFERENCE:  Sub-clause 4.1.8.2, 4.1.8.3 of GSM Recommendation 06.20
   *
   *   KEYWORDS: lag, interpolateCG
   *
   *************************************************************************/

  void Codec::CGInterp(int16_t pswLIn[],
                         short siNum,
                         int16_t pswCIn[],
                         int16_t pswGIn[],
                         short siLoIntLag,
                         int16_t pswCOut[],
                         int16_t pswGOut[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */
    int16_t i,
           swBig,
           swLoIntLag;
    int16_t swLagInt,
           swTempRem,
           swLagRem;
    int32_t L_Temp,
           L_Temp1,
           L_Temp2;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    swLoIntLag = add(siLoIntLag, (CG_INT_MACS / 2) - 1);

    for (swBig = 0; swBig < siNum; swBig++)
    {

      /* Separate integer and fractional portions of lag */
      /*-------------------------------------------------*/
      L_Temp = L_mult(pswLIn[swBig], INV_OS_FCTR);
      swLagInt = extract_h(L_Temp);

      /* swLagRem = (OS_FCTR - pswLIn[iBig] % OS_FCTR)) */
      /*---------------------------------------------------*/
      swTempRem = extract_l(L_Temp);
      swTempRem = shr(swTempRem, 1);
      swLagRem = swTempRem & SW_MAX;
      swLagRem = mult_r(swLagRem, OS_FCTR);
      swLagRem = sub(OS_FCTR, swLagRem);

      /* Get interpolated C and G values */
      /*--------------------------*/

      L_Temp1 = L_mac(32768, pswCIn[swLagInt - swLoIntLag],
                      ppsrCGIntFilt[0][swLagRem]);
      L_Temp2 = L_mac(32768, pswGIn[swLagInt - swLoIntLag],
                      ppsrCGIntFilt[0][swLagRem]);

      for (i = 1; i <= CG_INT_MACS - 1; i++)
      {
        L_Temp1 = L_mac(L_Temp1, pswCIn[i + swLagInt - swLoIntLag],
                        ppsrCGIntFilt[i][swLagRem]);
        L_Temp2 = L_mac(L_Temp2, pswGIn[i + swLagInt - swLoIntLag],
                        ppsrCGIntFilt[i][swLagRem]);

      }
      pswCOut[swBig] = extract_h(L_Temp1);
      pswGOut[swBig] = extract_h(L_Temp2);
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: CGInterpValid
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to retrieve the valid (codeable) lags
   *     within one (exclusive) integer sample of the given integer lag, and
   *     interpolate the corresponding C's and G's from the integer arrays
   *
   *   INPUTS:
   *
   *     swFullResLag
   *
   *                     integer lag * OS_FCTR
   *
   *     pswCIn[0:127]
   *
   *                     integer C sequence
   *
   *     pswGIn[0:127]
   *
   *                     integer G sequence
   *
   *     psrLagTbl[0:255]
   *
   *                     reference table of valid (codeable) lags
   *
   *
   *   OUTPUTS:
   *
   *     pswLOut[0:*psiNum-1]
   *
   *                     list of valid lags within 1 of swFullResLag
   *
   *     pswCOut[0:*psiNum-1]
   *
   *                     list of interpolated C's corresponding to pswLOut
   *
   *     pswGOut[0:*psiNum-1]
   *
   *                     list of interpolated G's corresponding to pswLOut
   *
   *   RETURN VALUE:
   *
   *     siNum
   *
   *                     length of output lists
   *
   *   DESCRIPTION:
   *
   *   REFERENCE:  Sub-clause 4.1.8.2, 4.1.9 of GSM Recommendation 06.20
   *
   *   KEYWORDS: CGInterpValid, cginterpvalid, CG_INT_VALID
   *
   *************************************************************************/

  short Codec::CGInterpValid(int16_t swFullResLag,
                              int16_t pswCIn[],
                              int16_t pswGIn[],
                              int16_t pswLOut[],
                              int16_t pswCOut[],
                              int16_t pswGOut[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    short int siLowerBound,
           siUpperBound,
           siNum,
           siI;
    int16_t swLag;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Get lower and upper bounds for valid lags     */
    /* within 1 (exclusive) integer lag of input lag */
    /* --------------------------------------------- */

    swLag = sub(swFullResLag, OS_FCTR);
    swLag = quantLag(swLag, &siLowerBound);
    if (sub(swLag, swFullResLag) != 0)
    {
      siLowerBound = add(siLowerBound, 1);
    }

    swLag = add(swFullResLag, OS_FCTR);
    swLag = quantLag(swLag, &siUpperBound);
    if (sub(swLag, swFullResLag) != 0)
    {
      siUpperBound = sub(siUpperBound, 1);
    }

    /* Get list of full resolution lags whose */
    /* C's and G's will be interpolated       */
    /* -------------------------------------- */

    siNum = sub(siUpperBound, siLowerBound);
    siNum = add(siNum, 1);

    for (siI = 0; siI < siNum; siI++)
    {
      pswLOut[siI] = psrLagTbl[siI + siLowerBound];
    }

    /* Interpolate C's and G's */
    /* ----------------------- */

    CGInterp(pswLOut, siNum, pswCIn, pswGIn, LSMIN, pswCOut,
             pswGOut);

    /* Return the length of the output lists */
    /* ------------------------------------- */

    return (siNum);
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: compResidEnergy
   *
   *   PURPOSE:
   *
   *     Computes and compares the residual energy from interpolated and
   *     non-interpolated coefficients. From the difference determines
   *     the soft interpolation decision.
   *
   *   INPUTS:
   *
   *     pswSpeech[0:159] ( [0:F_LEN-1] )
   *
   *                     Input speech frame (after high-pass filtering).
   *
   *     ppswInterpCoef[0:3][0:9] ( [0:N_SUB-1][0:NP-1] )
   *
   *                     Set of interpolated LPC direct-form coefficients for
   *                     each subframe.
   *
   *     pswPreviousCoef[0:9} ( [0:NP-1] )
   *
   *                     Set of LPC direct-form coefficients corresponding to
   *                     the previous frame
   *
   *     pswCurrentCoef[0:9} ( [0:NP-1] )
   *
   *                     Set of LPC direct-form coefficients corresponding to
   *                     the current frame
   *
   *     psnsSqrtRs[0:3] ( [0:N_SUB-1] )
   *
   *                     Array of residual energy estimates for each subframe
   *                     based on interpolated coefficients.  Used for scaling.
   *
   *   RETURN:
   *
   *     Returned value indicates the coefficients to use for each subframe:
   *     One indicates interpolated coefficients are to be used, zero indicates
   *     un-interpolated coefficients are to be used.
   *
   *   DESCRIPTION:
   *
   *
   *   REFERENCE:  Sub-clause 4.1.6 of GSM Recommendation 06.20
   *
   *   Keywords: openlooplagsearch, openloop, lag, pitch
   *
   **************************************************************************/



  short Codec::compResidEnergy(int16_t pswSpeech[],
                                int16_t ppswInterpCoef[][NP],
                                int16_t pswPreviousCoef[],
                                int16_t pswCurrentCoef[],
                                struct NormSw psnsSqrtRs[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    short  i,
           j,
           siOverflowPossible,
           siInterpDecision;
    int16_t swMinShift,
           swShiftFactor,
           swSample,
          *pswCoef;
    int16_t pswTempState[NP];
    int16_t pswResidual[S_LEN];
    int32_t L_ResidualEng;

  /*_________________________________________________________________________
   |                                                                         |
   |                            Executable Code                              |
   |_________________________________________________________________________|
  */

    /* Find minimum shift count of the square-root of residual energy */
    /* estimates over the four subframes.  According to this minimum, */
    /* find a shift count for the residual signal which will be used  */
    /* to avoid overflow when the actual residual energies are        */
    /* calculated over the frame                                      */
    /*----------------------------------------------------------------*/

    swMinShift = SW_MAX;
    for (i = 0; i < N_SUB; i++)
    {

      if (sub(psnsSqrtRs[i].sh, swMinShift) < 0 && psnsSqrtRs[i].man > 0)
        swMinShift = psnsSqrtRs[i].sh;
    }

    if (sub(swMinShift, 1) >= 0)
    {

      siOverflowPossible = 0;
    }

    else if (swMinShift == 0)
    {
      siOverflowPossible = 1;
      swShiftFactor = ONE_HALF;
    }

    else if (sub(swMinShift, -1) == 0)
    {
      siOverflowPossible = 1;
      swShiftFactor = ONE_QUARTER;
    }

    else
    {
      siOverflowPossible = 1;
      swShiftFactor = ONE_EIGHTH;
    }

    /* Copy analysis filter state into temporary buffer */
    /*--------------------------------------------------*/

    for (i = 0; i < NP; i++)
      pswTempState[i] = pswAnalysisState[i];

    /* Send the speech frame, one subframe at a time, through the analysis */
    /* filter which is based on interpolated coefficients.  After each     */
    /* subframe, accumulate the energy in the residual signal, scaling to  */
    /* avoid overflow if necessary.                                        */
    /*---------------------------------------------------------------------*/

    L_ResidualEng = 0;

    for (i = 0; i < N_SUB; i++)
    {

      lpcFir(&pswSpeech[i * S_LEN], ppswInterpCoef[i], pswTempState,
             pswResidual);

      if (siOverflowPossible)
      {

        for (j = 0; j < S_LEN; j++)
        {

          swSample = mult_r(swShiftFactor, pswResidual[j]);
          L_ResidualEng = L_mac(L_ResidualEng, swSample, swSample);
        }
      }

      else
      {

        for (j = 0; j < S_LEN; j++)
        {

          L_ResidualEng = L_mac(L_ResidualEng, pswResidual[j], pswResidual[j]);
        }
      }
    }

    /* Send the speech frame, one subframe at a time, through the analysis */
    /* filter which is based on un-interpolated coefficients.  After each  */
    /* subframe, subtract the energy in the residual signal from the       */
    /* accumulated residual energy due to the interpolated coefficient     */
    /* analysis filter, again scaling to avoid overflow if necessary.      */
    /* Note that the analysis filter state is updated during these         */
    /* filtering operations.                                               */
    /*---------------------------------------------------------------------*/

    for (i = 0; i < N_SUB; i++)
    {

      switch (i)
      {

        case 0:

          pswCoef = pswPreviousCoef;
          break;

        case 1:
        case 2:
        case 3:

          pswCoef = pswCurrentCoef;
          break;
      }

      lpcFir(&pswSpeech[i * S_LEN], pswCoef, pswAnalysisState,
             pswResidual);

      if (siOverflowPossible)
      {

        for (j = 0; j < S_LEN; j++)
        {

          swSample = mult_r(swShiftFactor, pswResidual[j]);
          L_ResidualEng = L_msu(L_ResidualEng, swSample, swSample);
        }
      }

      else
      {

        for (j = 0; j < S_LEN; j++)
        {

          L_ResidualEng = L_msu(L_ResidualEng, pswResidual[j], pswResidual[j]);
        }
      }
    }

    /* Make soft-interpolation decision based on the difference in residual */
    /* energies                                                             */
    /*----------------------------------------------------------------------*/

    if (L_ResidualEng < 0)
      siInterpDecision = 1;

    else
      siInterpDecision = 0;

    return siInterpDecision;
  }

  /***************************************************************************
   *
   *    FUNCTION NAME: cov32
   *
   *    PURPOSE: Calculates B, F, and C correlation matrices from which
   *             the reflection coefficients are computed using the FLAT
   *             algorithm. The Spectral Smoothing Technique (SST) is applied
   *             to the correlations. End point correction is employed
   *             in computing the correlations to minimize computation.
   *
   *    INPUT:
   *
   *       pswIn[0:169]
   *                     A sampled speech vector used to compute
   *                     correlations need for generating the optimal
   *                     reflection coefficients via the FLAT algorithm.
   *
   *       CVSHIFT       The number of right shifts by which the normalized
   *                     correlations are to be shifted down prior to being
   *                     rounded into the int16_t output correlation arrays
   *                     B, F, and C.
   *
   *       pL_rFlatSstCoefs[NP]
   *
   *                     A table stored in Rom containing the spectral
   *                     smoothing function coefficients.
   *
   *    OUTPUTS:
   *
   *       pppL_B[0:NP-1][0:NP-1][0:1]
   *                     An output correlation array containing the backward
   *                     correlations of the input signal. It is a square
   *                     matrix symmetric about the diagonal. Only the upper
   *                     right hand triangular region of this matrix is
   *                     initialized, but two dimensional indexing is retained
   *                     to enhance clarity. The third array dimension is used
   *                     by function flat to swap the current and the past
   *                     values of B array, eliminating the need to copy
   *                     the updated B values onto the old B values at the
   *                     end of a given lattice stage. The third dimension
   *                     is similarily employed in arrays F and C.
   *
   *       pppL_F[0:NP-1][0:NP-1][0:1]
   *                     An output correlation array containing the forward
   *                     correlations of the input signal. It is a square
   *                     matrix symmetric about the diagonal. Only the upper
   *                     right hand triangular region of this matrix is
   *                     initialized.
   *
   *       pppL_C[0:NP-1][0:NP-1][0:1]
   *                     An output correlation array containing the cross
   *                     correlations of the input signal. It is a square
   *                     matrix which is not symmetric. All its elements
   *                     are initialized, for the third dimension index = 0.
   *
   *       pL_R0         Average normalized signal power over F_LEN
   *                     samples, given by 0.5*(Phi(0,0)+Phi(NP,NP)), where
   *                     Phi(0,0) and Phi(NP,NP) are normalized signal
   *                     autocorrelations.  The average unnormalized signal
   *                     power over the frame is given by adjusting L_R0 by
   *                     the shift count which is returned. pL_R0 along
   *                     with the returned shift count are the inputs to
   *                     the frame energy quantizer.
   *
   *        int32_t pL_VadAcf[4]
   *                     An array with the autocorrelation coefficients to be
   *                     used by the VAD.
   *
   *        int16_t *pswVadScalAuto
   *                     Input scaling factor used by the VAD.
   *
   *    RETURN:
   *
   *       swNormPwr     The shift count to be applied to pL_R0 for
   *                     reconstructing the average unnormalized
   *                     signal power over the frame.
   *                     Negative shift count means that a left shift was
   *                     applied to the correlations to achieve a normalized
   *                     value of pL_R0.
   *
   *   DESCRIPTION:
   *
   *
   *      The input energy of the signal is assumed unknown.  It maximum
   *      can be F_LEN*0.5. The 0.5 factor accounts for scaling down of the
   *      input signal in the high-pass filter.  Therefore the signal is
   *      shifted down by 3 shifts producing an energy reduction of 2^(2*3)=64.
   *      The resulting energy is then normalized.  Based on the shift count,
   *      the correlations F, B, and C are computed using as few shifts as
   *      possible, so a precise result is attained.
   *      This is an implementation of equations: 2.1 through 2.11.
   *
   *   REFERENCE:  Sub-clause 4.1.3 of GSM Recommendation 06.20
   *
   *   keywords: energy, autocorrelation, correlation, cross-correlation
   *   keywords: spectral smoothing, SST, LPC, FLAT, flat
   *
   *************************************************************************/

  int16_t Codec::cov32(int16_t pswIn[],
                         int32_t pppL_B[NP][NP][2],
                         int32_t pppL_F[NP][NP][2],
                         int32_t pppL_C[NP][NP][2],
                         int32_t *pL_R0,
                         int32_t pL_VadAcf[],
                         int16_t *pswVadScalAuto)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_max,
           L_Pwr0,
           L_Pwr,
           L_temp,
           pL_Phi[NP + 1];
    int16_t swTemp,
           swNorm,
           swNormSig,
           swNormPwr,
           pswInScale[A_LEN],
           swPhiNorm;
    short int i,
           k,
           kk,
           n;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Calculate energy in the frame vector (160 samples) for each   */
    /* of NP frame placements. The energy is reduced by 64. This is  */
    /* accomplished by shifting the input right by 3 bits. An offset */
    /* of 0x117f0b is placed into the accumulator to account for     */
    /* the worst case power gain due to the 3 LSB's of the input     */
    /* signal which were right shifted. The worst case is that the   */
    /* 3 LSB's were all set to 1 for each of the samples. Scaling of */
    /* the input by a half is assumed here.                          */
    /*---------------------------------------------------------------*/

    L_max = 0;
    for (L_Pwr = 0x117f0b, i = 0; i < F_LEN; i++)
    {
      swTemp = shr(pswIn[i], 3);
      L_Pwr = L_mac(L_Pwr, swTemp, swTemp);
    }
    L_max |= L_Pwr;

    /* L_max tracks the maximum power over NP window placements */
    /*----------------------------------------------------------*/

    for (i = 1; i <= NP; i++)
    {

      /* Subtract the power due to 1-st sample from previous window
       * placement. */
      /*-----------------------------------------------------------*/

      swTemp = shr(pswIn[i - 1], 3);
      L_Pwr = L_msu(L_Pwr, swTemp, swTemp);

      /* Add the power due to new sample at the current window placement. */
      /*------------------------------------------------------------------*/

      swTemp = shr(pswIn[F_LEN + i - 1], 3);
      L_Pwr = L_mac(L_Pwr, swTemp, swTemp);

      L_max |= L_Pwr;

    }

    /* Compute the shift count needed to achieve normalized value */
    /* of the correlations.                                       */
    /*------------------------------------------------------------*/

    swTemp = norm_l(L_max);
    swNorm = sub(6, swTemp);

    if (swNorm >= 0)
    {

      /* The input signal needs to be shifted down, to avoid limiting */
      /* so compute the shift count to be applied to the input.       */
      /*--------------------------------------------------------------*/

      swTemp = add(swNorm, 1);
      swNormSig = shr(swTemp, 1);
      swNormSig = add(swNormSig, 0x0001);

    }
    else
    {
      /* No scaling down of the input is necessary */
      /*-------------------------------------------*/

      swNormSig = 0;

    }

    /* Convert the scaling down, if any, which was done to the time signal */
    /* to the power domain, and save.                                      */
    /*---------------------------------------------------------------------*/

    swNormPwr = shl(swNormSig, 1);

    /* Buffer the input signal, scaling it down if needed. */
    /*-----------------------------------------------------*/

    for (i = 0; i < A_LEN; i++)
    {
      pswInScale[i] = shr(pswIn[i], swNormSig);
    }

    /* Compute from buffered (scaled) input signal the correlations     */
    /* needed for the computing the reflection coefficients.            */
    /*------------------------------------------------------------------*/

    /* Compute correlation Phi(0,0) */
    /*------------------------------*/

    L_Pwr = L_mult(pswInScale[NP], pswInScale[NP]);
    for (n = 1; n < F_LEN; n++)
    {
      L_Pwr = L_mac(L_Pwr, pswInScale[NP + n], pswInScale[NP + n]);
    }
    pL_Phi[0] = L_Pwr;

    /* Get ACF[0] and input scaling factor for VAD algorithm */
    *pswVadScalAuto = swNormSig;
    pL_VadAcf[0] = L_Pwr;

    /* Compute the remaining correlations along the diagonal which */
    /* starts at Phi(0,0). End-point correction is employed to     */
    /* limit computation.                                          */
    /*-------------------------------------------------------------*/

    for (i = 1; i <= NP; i++)
    {

      /* Compute the power in the last sample from the previous         */
      /* window placement, and subtract it from correlation accumulated */
      /* at the previous window placement.                              */
      /*----------------------------------------------------------------*/

      L_Pwr = L_msu(L_Pwr, pswInScale[NP + F_LEN - i],
                    pswInScale[NP + F_LEN - i]);

      /* Compute the power in the new sample for the current window       */
      /* placement, and add it to L_Pwr to obtain the value of Phi(i,i). */
      /*------------------------------------------------------------------*/

      L_Pwr = L_mac(L_Pwr, pswInScale[NP - i], pswInScale[NP - i]);

      pL_Phi[i] = L_Pwr;

    }

    /* Compute the shift count necessary to normalize the Phi array  */
    /*---------------------------------------------------------------*/

    L_max = 0;
    for (i = 0; i <= NP; i++)
    {
      L_max |= pL_Phi[i];
    }
    swPhiNorm = norm_l(L_max);

    /* Adjust the shift count to be returned to account for any scaling */
    /* down which might have been done to the input signal prior to     */
    /* computing the correlations.                                      */
    /*------------------------------------------------------------------*/

    swNormPwr = sub(swNormPwr, swPhiNorm);

    /* Compute the average power over the frame; i.e.,                   */
    /* 0.5*(Phi(0,0)+Phi(NP,NP)), given a normalized pL_Phi array.       */
    /*-------------------------------------------------------------------*/

    swTemp = sub(swPhiNorm, 1);
    L_Pwr0 = L_shl(pL_Phi[0], swTemp);
    L_Pwr = L_shl(pL_Phi[NP], swTemp);
    *pL_R0 = L_add(L_Pwr, L_Pwr0);       /* Copy power to output pointer */

    /* Check if the average power is normalized; if not, shift left by 1 bit */
    /*-----------------------------------------------------------------------*/

    if (!(*pL_R0 & 0x40000000))
    {
      *pL_R0 = L_shl(*pL_R0, 1);         /* normalize the average power    */
      swNormPwr = sub(swNormPwr, 1);     /* adjust the shift count         */
    }

    /* Reduce the shift count needed to normalize the correlations   */
    /* by CVSHIFT bits.                                              */
    /*---------------------------------------------------------------*/

    swNorm = sub(swPhiNorm, CVSHIFT);

    /* Initialize the F, B, and C output correlation arrays, using the */
    /* Phi correlations computed along the diagonal of symmetry.       */
    /*-----------------------------------------------------------------*/

    L_temp = L_shl(pL_Phi[0], swNorm);   /* Normalize the result     */

    pppL_F[0][0][0] = L_temp;            /* Write to output array    */

    for (i = 1; i <= NP - 1; i++)
    {

      L_temp = L_shl(pL_Phi[i], swNorm); /* Normalize the result     */


      pppL_F[i][i][0] = L_temp;          /* Write to output array    */
      pppL_B[i - 1][i - 1][0] = L_temp;  /* Write to output array    */
      pppL_C[i][i - 1][0] = L_temp;      /* Write to output array    */

    }

    L_temp = L_shl(pL_Phi[NP], swNorm);  /* Normalize the result     */

    pppL_B[NP - 1][NP - 1][0] = L_temp;  /* Write to output array    */

    for (k = 1; k <= NP - 1; k++)
    {

      /* Compute correlation Phi(0,k) */
      /*------------------------------*/

      L_Pwr = L_mult(pswInScale[NP], pswInScale[NP - k]);
      for (n = 1; n < F_LEN; n++)
      {
        L_Pwr = L_mac(L_Pwr, pswInScale[NP + n], pswInScale[NP + n - k]);
      }
      /* convert covariance values to ACF and store for VAD algorithm */
      if (k < 9)
      {
        pL_VadAcf[k] = L_Pwr;
        for (kk = 0; kk < k; kk++)
        {
          pL_VadAcf[k] = L_msu(pL_VadAcf[k], pswInScale[NP + kk],
                               pswInScale[NP + kk - k]);
        }
      }

      L_temp = L_shl(L_Pwr, swNorm);     /* Normalize the result */
      L_temp = L_mpy_ll(L_temp, pL_rFlatSstCoefs[k - 1]); /* Apply SST */

      pppL_F[0][k][0] = L_temp;          /* Write to output array    */
      pppL_C[0][k - 1][0] = L_temp;      /* Write to output array    */


      /* Compute the remaining correlations along the diagonal which */
      /* starts at Phi(0,k). End-point correction is employed to     */
      /* limit computation.                                          */
      /*-------------------------------------------------------------*/

      for (kk = k + 1, i = 1; kk <= NP - 1; kk++, i++)
      {

        /* Compute the power in the last sample from the previous         */
        /* window placement, and subtract it from correlation accumulated */
        /* at the previous window placement.                              */
        /*----------------------------------------------------------------*/

        L_Pwr = L_msu(L_Pwr, pswInScale[NP + F_LEN - i],
                      pswInScale[NP + F_LEN - kk]);

        /* Compute the power in the new sample for the current window       */
        /* placement, and add it to L_Pwr to obtain the value of Phi(i,kk). */
        /*------------------------------------------------------------------*/

        L_Pwr = L_mac(L_Pwr, pswInScale[NP - i], pswInScale[NP - kk]);

        L_temp = L_shl(L_Pwr, swNorm);   /* Normalize */
        L_temp = L_mpy_ll(L_temp, pL_rFlatSstCoefs[k - 1]);     /* Apply SST */

        pppL_F[i][kk][0] = L_temp;       /* Write to output array */
        pppL_B[i - 1][kk - 1][0] = L_temp;        /* Write to output array */
        pppL_C[i][kk - 1][0] = L_temp;   /* Write to output array    */
        pppL_C[kk][i - 1][0] = L_temp;   /* Write to output array    */

      }

      /* Compute the power in the last sample from the previous         */
      /* window placement, and subtract it from correlation accumulated */
      /* at the previous window placement.                              */
      /*----------------------------------------------------------------*/

      L_Pwr = L_msu(L_Pwr, pswInScale[F_LEN + k], pswInScale[F_LEN]);

      /* Compute the power in the new sample for the current window       */
      /* placement, and add it to L_Pwr to obtain the value of Phi(i,kk). */
      /*------------------------------------------------------------------*/

      L_Pwr = L_mac(L_Pwr, pswInScale[k], pswInScale[0]);

      L_temp = L_shl(L_Pwr, swNorm);     /* Normalize the result */
      L_temp = L_mpy_ll(L_temp, pL_rFlatSstCoefs[k - 1]); /* Apply SST */

      pppL_B[NP - k - 1][NP - 1][0] = L_temp;     /* Write to output array */
      pppL_C[NP - k][NP - 1][0] = L_temp;/* Write to output array */

    }

    /* Compute correlation Phi(0,NP) */
    /*-------------------------------*/

    L_Pwr = L_mult(pswInScale[NP], pswInScale[0]);
    for (n = 1; n < F_LEN; n++)
    {
      L_Pwr = L_mac(L_Pwr, pswInScale[NP + n], pswInScale[n]);
    }

    L_temp = L_shl(L_Pwr, swNorm);       /* Normalize the result */
    L_temp = L_mpy_ll(L_temp, pL_rFlatSstCoefs[NP - 1]);  /* Apply SST */

    pppL_C[0][NP - 1][0] = L_temp;       /* Write to output array */

    return (swNormPwr);

  }

  /***************************************************************************
   *
   *    FUNCTION NAME: filt4_2nd
   *
   *    PURPOSE:  Implements a fourth order filter by cascading two second
   *              order sections.
   *
   *    INPUTS:
   *
   *      pswCoef[0:9]   An array of two sets of filter coefficients.
   *
   *      pswIn[0:159]   An array of input samples to be filtered, filtered
   *                     output samples written to the same array.
   *
   *      pswXstate[0:3] An array containing x-state memory for two 2nd order
   *                     filter sections.
   *
   *      pswYstate[0:7] An array containing y-state memory for two 2nd order
   *                     filter sections.
   *
   *      npts           Number of samples to filter (must be even).
   *
   *      shifts         number of shifts to be made on output y(n).
   *
   *    OUTPUTS:
   *
   *       pswIn[0:159]  Output array containing the filtered input samples.
   *
   *    RETURN:
   *
   *       none.
   *
   *    DESCRIPTION:
   *
   *    data structure:
   *
   *    Coeff array order:  (b2,b1,b0,a2,a1)Section 1;(b2,b1,b0,a2,a1)Section 2
   *    Xstate array order: (x(n-2),x(n-1))Section 1; (x(n-2),x(n-1))Section 2
   *    Ystate array order: y(n-2)MSB,y(n-2)LSB,y(n-1)MSB,y(n-1)LSB Section 1
   *                        y(n-2)MSB,y(n-2)LSB,y(n-1)MSB,y(n-1)LSB Section 2
   *
   *    REFERENCE:  Sub-clause 4.1.1 GSM Recommendation 06.20
   *
   *    KEYWORDS: highpass filter, hp, HP, filter
   *
   *************************************************************************/

  void Codec::filt4_2nd(int16_t pswCoeff[], int16_t pswIn[],
                          int16_t pswXstate[], int16_t pswYstate[],
                          int npts, int shifts)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Do first second order section */
    /*-------------------------------*/

    iir_d(&pswCoeff[0],pswIn,&pswXstate[0],&pswYstate[0],npts,shifts,1,0);


    /* Do second second order section */
    /*--------------------------------*/

    iir_d(&pswCoeff[5],pswIn,&pswXstate[2],&pswYstate[4],npts,shifts,0,1);

  }

  /***************************************************************************
   *
   *   FUNCTION NAME: findBestInQuantList
   *
   *   PURPOSE:
   *     Given a list of quantizer vectors and their associated prediction
   *     errors, search the list for the iNumVectOut vectors and output them
   *     as a new list.
   *
   *   INPUTS: psqlInList, iNumVectOut
   *
   *   OUTPUTS: psqlBestOutList
   *
   *   RETURN VALUE: none
   *
   *   DESCRIPTION:
   *
   *     The AFLAT recursion yields prediction errors.  This routine finds
   *     the lowest candidate is the AFLAT recursion outputs.
   *
   *
   *   KEYWORDS: best quantlist find
   *
   *   REFERENCE:  Sub-clause 4.1.4.1 GSM Recommendation 06.20
   *
   *************************************************************************/

  void Codec::findBestInQuantList(struct QuantList psqlInList,
                                  int iNumVectOut,
                                  struct QuantList psqlBestOutList[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */
    int    quantIndex,
           bstIndex,
           i;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* initialize the best list */
    /* invalidate, ensure they will be dropped */
    for (bstIndex = 0; bstIndex < iNumVectOut; bstIndex++)
    {
      psqlBestOutList[bstIndex].iNum = 1;
      psqlBestOutList[bstIndex].iRCIndex = psqlInList.iRCIndex;
      psqlBestOutList[bstIndex].pswPredErr[0] = 0x7fff;
    }

    /* best list elements replaced in the order:  0,1,2,3... challenger must
     * be < (not <= ) current best */
    for (quantIndex = 0; quantIndex < psqlInList.iNum; quantIndex++)
    {
      bstIndex = 0;
      while (sub(psqlInList.pswPredErr[quantIndex],
                 psqlBestOutList[bstIndex].pswPredErr[0]) >= 0 &&
             bstIndex < iNumVectOut)
      {
        bstIndex++;                      /* only increments to next upon
                                          * failure to beat "best" */
      }

      if (bstIndex < iNumVectOut)
      {                                  /* a new value is found */
        /* now add challenger to best list at index bstIndex */
        for (i = iNumVectOut - 1; i > bstIndex; i--)
        {
          psqlBestOutList[i].pswPredErr[0] =
                  psqlBestOutList[i - 1].pswPredErr[0];
          psqlBestOutList[i].iRCIndex =
                  psqlBestOutList[i - 1].iRCIndex;
        }
        /* get new best value and place in list */
        psqlBestOutList[bstIndex].pswPredErr[0] =
                psqlInList.pswPredErr[quantIndex];
        psqlBestOutList[bstIndex].iRCIndex =
                psqlInList.iRCIndex + quantIndex;
      }
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: findPeak
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to return the lag
   *     that maximizes CC/G within +- PEAK_VICINITY of the
   *     input lag.  The input lag is an integer lag, and
   *     the search for a peak is done on the surrounding
   *     integer lags.
   *
   *   INPUTS:
   *
   *     swSingleResLag
   *
   *                     Input integer lag, expressed as lag * OS_FCTR
   *
   *     pswCIn[0:127]
   *
   *                     C(k) sequence, k an integer
   *
   *     pswGIn[0:127]
   *
   *                     G(k) sequence, k an integer
   *
   *   OUTPUTS:
   *
   *     none
   *
   *   RETURN VALUE:
   *
   *     Integer lag where peak was found, or zero if no peak was found.
   *     The lag is expressed as lag * OS_FCTR
   *
   *   DESCRIPTION:
   *
   *     This routine is called from pitchLags(), and is used to do the
   *     interpolating CC/G peak search.  This is used in a number of
   *     places in pitchLags().  See description 5.3.1.
   *
   *   REFERENCE:  Sub-clause 4.1.8.2 of GSM Recommendation 06.20
   *
   *   KEYWORDS:
   *
   *************************************************************************/

  int16_t Codec::findPeak(int16_t swSingleResLag,
                            int16_t pswCIn[],
                            int16_t pswGIn[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swCmaxSqr,
           swGmax,
           swFullResPeak;
    short int siUpperBound,
           siLowerBound,
           siRange,
           siPeak;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* get upper and lower bounds for integer lags for peak search */
    /* ----------------------------------------------------------- */

    siUpperBound = add(swSingleResLag, PEAK_VICINITY + 1);
    if (sub(siUpperBound, LMAX + 1) > 0)
    {
      siUpperBound = LMAX + 1;
    }

    siLowerBound = sub(swSingleResLag, PEAK_VICINITY + 1);
    if (sub(siLowerBound, LMIN - 1) < 0)
    {
      siLowerBound = LMIN - 1;
    }

    siRange = sub(siUpperBound, siLowerBound);
    siRange = add(siRange, 1);

    /* do peak search */
    /* -------------- */

    swCmaxSqr = 0;
    swGmax = 0x3f;

    siPeak = fnBest_CG(&pswCIn[siLowerBound - LSMIN],
                       &pswGIn[siLowerBound - LSMIN], &swCmaxSqr, &swGmax,
                       siRange);

    /* if no max found, flag no peak */
    /* ----------------------------- */

    if (add(siPeak, 1) == 0)
    {
      swFullResPeak = 0;
    }

    /* determine peak location      */
    /* if at boundary, flag no peak */
    /* else return lag at peak      */
    /* ---------------------------- */

    else
    {
      siPeak = add(siPeak, siLowerBound);

      if ((sub(siPeak, siLowerBound) == 0) ||
          (sub(siPeak, siUpperBound) == 0))
      {
        swFullResPeak = 0;
      }
      else
      {
        swFullResPeak = shr(extract_l(L_mult(siPeak, OS_FCTR)), 1);
      }
    }
    return (swFullResPeak);
  }

  /***************************************************************************
   *
   *    FUNCTION NAME: flat
   *
   *    PURPOSE:  Computes the unquantized reflection coefficients from the
   *              input speech using the FLAT algorithm. Also computes the
   *              frame energy, and the index of the element in the R0
   *              quantization table which best represents the frame energy.
   *              Calls function cov32 which computes the F, B, and C
   *              correlation arrays, required by the FLAT algorithm to
   *              compute the reflection coefficients.
   *
   *    INPUT:
   *
   *       pswSpeechIn[0:169]
   *                     A sampled speech vector used to compute
   *                     correlations need for generating the optimal
   *                     reflection coefficients via the FLAT algorithm.
   *
   *    OUTPUTS:
   *
   *       pswRc[NP]     An array of unquantized reflection coefficients.
   *
   *       *piR0Inx      An index of the quantized frame energy value.
   *
   *       int32_t pL_VadAcf[4]
   *                     An array with the autocorrelation coefficients to be
   *                     used by the VAD.  Generated by cov16(), a daughter
   *                     function of flat().
   *
   *       int16_t *pswVadScalAuto
   *                     Input scaling factor used by the VAD.
   *                     Generated by cov16(), a daughter function of flat().
   *                     function.
   *
   *    RETURN:          L_R0 normalized frame energy value, required in DTX
   *                     mode.
   *
   *   DESCRIPTION:
   *
   *    An efficient Fixed point LAtice Technique (FLAT) is used to compute
   *    the reflection coefficients, given B, F, and C arrays returned by
   *    function cov32. B, F, and C are backward, forward, and cross
   *    correlations computed from the input speech. The correlations
   *    are spectrally smoothed in cov32.
   *
   *
   *   REFERENCE:  Sub-clause 4.1.3 of GSM Recommendation 06.20
   *
   *   keywords: LPC, FLAT, reflection coefficients, covariance, correlation,
   *   keywords: spectrum, energy, R0, spectral smoothing, SST
   *
   *************************************************************************/

  int32_t Codec::flat(int16_t pswSpeechIn[],
                     int16_t pswRc[],
                     int *piR0Inx,
                     int32_t pL_VadAcf[],
                     int16_t *pswVadScalAuto)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */
    int16_t
           swNum,
           swDen,
           swRcSq,
           swSqrtOut,
           swRShifts,
           swShift,
           swShift1;
    int32_t
           pppL_F[NP][NP][2],
           pppL_B[NP][NP][2],
           pppL_C[NP][NP][2],
           L_Num,
           L_TmpA,
           L_TmpB,
           L_temp,
           L_sum,
           L_R0,
           L_Fik,
           L_Bik,
           L_Cik,
           L_Cki;
    short int i,
           j,
           k,
           l,
           j_0,
           j_1;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Compute from the input speech the elements of the B, F, and C     */
    /* arrays, which form the initial conditions for the FLAT algorithm. */
    /*-------------------------------------------------------------------*/

    swRShifts = cov32(pswSpeechIn, pppL_B, pppL_F, pppL_C, &L_R0,
                      pL_VadAcf, pswVadScalAuto);

    /* Compute the intermediate quantities required by the R0 quantizer */
    /*------------------------------------------------------------------*/

    if (L_R0 != 0)
    {
      swSqrtOut = sqroot(L_R0);          /* If L_R0 > 0, compute sqrt */
    }
    else
    {
      swSqrtOut = 0;                     /* L_R0 = 0, initialize sqrt(0) */
    }

    swRShifts = sub(S_SH + 2, swRShifts);

    /* If odd number of shifts compensate by sqrt(0.5) */
    /*-------------------------------------------------*/

    if (swRShifts & 1)
    {
      L_temp = L_mult(swSqrtOut, 0x5a82);
    }
    else
    {
      L_temp = L_deposit_h(swSqrtOut);
    }
    swRShifts = shr(swRShifts, 1);

    if (swRShifts > 0)
    {
      L_temp = L_shr(L_temp, swRShifts);
    }
    else if (swRShifts < 0)
    {
      L_temp = 0;
    }

    /* Given average energy L_temp, find the index in the R0 quantization */
    /* table which best represents it.                                    */
    /*--------------------------------------------------------------------*/

    *piR0Inx = r0Quant(L_temp);

    L_R0 = L_temp; /* save the unquantized R0 */             /* DTX mode */

    /* Zero out the number of left-shifts to be applied to the  */
    /* F, B, and C matrices.                                    */
    /*----------------------------------------------------------*/

    swShift = 0;

    /* Now compute the NP reflection coefficients  */
    /*---------------------------------------------*/

    for (j = 0; j < NP; j++)
    {

      /* Initialize the modulo indices of the third dimension of arrays  */
      /* F, B, and C, where indices j_0 and j_1 point to:                */
      /* */
      /* j_0 = points to F, B, and C matrix values at stage j-0, which   */
      /* is the current  lattice stage.                                  */
      /* j_1 = points to F, B, and C matrix values at stage j-1, which   */
      /* is the previous lattice stage.                                  */
      /* */
      /* Use of modulo address arithmetic permits to swap values of j_0 and */
      /* and j_1 at each lattice stage, thus eliminating the need to copy   */
      /* the current elements of F, B, and C arrays, into the F, B, and C   */
      /* arrays corresponding to the previous lattice stage, prior to       */
      /* incrementing j, the index of the lattice filter stage.             */
      /*--------------------------------------------------------------------*/

      j_0 = (j + 1) % 2;
      j_1 = j % 2;

      /* Get the numerator for computing the j-th reflection coefficient */
      /*-----------------------------------------------------------------*/

      L_Num = L_add(L_shl(pppL_C[0][0][j_1], swShift),
                    L_shl(pppL_C[NP - j - 1][NP - j - 1][j_1], swShift));

      /* Get the denominator for computing the j-th reflection coefficient */
      /*-------------------------------------------------------------------*/

      L_temp = L_add(L_shl(pppL_F[0][0][j_1], swShift),
                     L_shl(pppL_B[0][0][j_1], swShift));
      L_TmpA = L_add(L_shl(pppL_F[NP - j - 1][NP - j - 1][j_1], swShift),
                     L_shl(pppL_B[NP - j - 1][NP - j - 1][j_1], swShift));
      L_sum = L_add(L_TmpA, L_temp);
      L_sum = L_shr(L_sum, 1);

      /* Normalize the numerator and the denominator terms */
      /*---------------------------------------------------*/

      swShift1 = norm_s(extract_h(L_sum));

      L_sum = L_shl(L_sum, swShift1);
      L_Num = L_shl(L_Num, swShift1);

      swNum = round(L_Num);
      swDen = round(L_sum);

      if (swDen <= 0)
      {

        /* Zero prediction error at the j-th lattice stage, zero */
        /* out remaining reflection coefficients and return.     */
        /*-------------------------------------------------------*/

        for (i = j; i < NP; i++)
        {
          pswRc[i] = 0;
        }

        return (L_R0);
      }
      else
      {

        /* Non-zero prediction error, check if the j-th reflection
         * coefficient */
        /* about to be computed is stable.                           */
        /*-----------------------------------------------------------*/

        if (sub(abs_s(swNum), swDen) >= 0)
        {

          /* Reflection coefficient at j-th lattice stage unstable, so zero  */
          /* out reflection coefficients for lattice stages i=j,...,NP-1, and */
          /* return.                                                         */
          /*-----------------------------------------------------------------*/

          for (i = j; i < NP; i++)
          {
            pswRc[i] = 0;
          }

          return (L_R0);
        }
        else
        {

          /* j-th reflection coefficient is stable, compute it. */
          /*----------------------------------------------------*/

          if (swNum < 0)
          {

            swNum = negate(swNum);
            pswRc[j] = divide_s(swNum, swDen);

          }
          else
          {

            pswRc[j] = divide_s(swNum, swDen);
            pswRc[j] = negate(pswRc[j]);

          }                              /* j-th reflection coefficient
                                          * sucessfully computed. */
          /*----------------------------------------------------*/


        }                                /* End of reflection coefficient
                                          * stability test (and computation) */
        /*------------------------------------------------------------------*/

      }                                  /* End of non-zero prediction error
                                          * case */
      /*----------------------------------------*/



      /* If not at the last lattice stage, update F, B, and C arrays */
      /*-------------------------------------------------------------*/

      if (j != NP - 1)
      {

        /* Compute squared Rc[j] */
        /*-----------------------*/

        swRcSq = mult_r(pswRc[j], pswRc[j]);

        i = 0;
        k = 0;

        /* Compute the common terms used by the FLAT recursion to reduce */
        /* computation.                                                  */
        /*---------------------------------------------------------------*/

        L_Cik = L_shl(pppL_C[i][k][j_1], swShift);

        L_TmpA = L_add(L_Cik, L_Cik);
        L_TmpA = L_mpy_ls(L_TmpA, pswRc[j]);

        /* Update the F array */
        /*--------------------*/

        L_Fik = L_shl(pppL_F[i][k][j_1], swShift);
        L_Bik = L_shl(pppL_B[i][k][j_1], swShift);

        L_temp = L_mpy_ls(L_Bik, swRcSq);
        L_temp = L_add(L_temp, L_Fik);
        pppL_F[i][k][j_0] = L_add(L_temp, L_TmpA);

        for (k = i + 1; k <= NP - j - 2; k++)
        {

          /* Compute the common terms used by the FLAT recursion to reduce */
          /* computation.                                                  */
          /*---------------------------------------------------------------*/

          L_Cik = L_shl(pppL_C[i][k][j_1], swShift);
          L_Cki = L_shl(pppL_C[k][i][j_1], swShift);

          L_TmpA = L_add(L_Cik, L_Cki);
          L_TmpA = L_mpy_ls(L_TmpA, pswRc[j]);

          L_Bik = L_shl(pppL_B[i][k][j_1], swShift);
          L_Fik = L_shl(pppL_F[i][k][j_1], swShift);

          L_TmpB = L_add(L_Bik, L_Fik);
          L_TmpB = L_mpy_ls(L_TmpB, pswRc[j]);

          /* Update the F and C arrays */
          /*---------------------------------*/

          L_temp = L_mpy_ls(L_Bik, swRcSq);
          L_temp = L_add(L_temp, L_Fik);
          pppL_F[i][k][j_0] = L_add(L_temp, L_TmpA);

          L_temp = L_mpy_ls(L_Cki, swRcSq);
          L_temp = L_add(L_temp, L_Cik);
          pppL_C[i][k - 1][j_0] = L_add(L_temp, L_TmpB);

        }

        k = NP - j - 1;

        /* Compute the common terms used by the FLAT recursion to reduce */
        /* computation.                                                  */
        /*---------------------------------------------------------------*/

        L_Bik = L_shl(pppL_B[i][k][j_1], swShift);
        L_Fik = L_shl(pppL_F[i][k][j_1], swShift);

        L_TmpB = L_add(L_Bik, L_Fik);
        L_TmpB = L_mpy_ls(L_TmpB, pswRc[j]);

        /* Update the C array */
        /*-----------------------*/

        L_Cik = L_shl(pppL_C[i][k][j_1], swShift);
        L_Cki = L_shl(pppL_C[k][i][j_1], swShift);

        L_temp = L_mpy_ls(L_Cki, swRcSq);
        L_temp = L_add(L_temp, L_Cik);
        pppL_C[i][k - 1][j_0] = L_add(L_temp, L_TmpB);


        for (i = 1; i <= NP - j - 2; i++)
        {

          k = i;

          /* Compute the common terms used by the FLAT recursion to reduce */
          /* computation.                                                  */
          /*---------------------------------------------------------------*/

          L_Cik = L_shl(pppL_C[i][k][j_1], swShift);

          L_TmpA = L_add(L_Cik, L_Cik);
          L_TmpA = L_mpy_ls(L_TmpA, pswRc[j]);

          L_Bik = L_shl(pppL_B[i][k][j_1], swShift);
          L_Fik = L_shl(pppL_F[i][k][j_1], swShift);

          L_TmpB = L_add(L_Bik, L_Fik);
          L_TmpB = L_mpy_ls(L_TmpB, pswRc[j]);

          /* Update F, B and C arrays */
          /*-----------------------------------*/

          L_temp = L_mpy_ls(L_Bik, swRcSq);
          L_temp = L_add(L_temp, L_Fik);
          pppL_F[i][k][j_0] = L_add(L_temp, L_TmpA);

          L_temp = L_mpy_ls(L_Fik, swRcSq);
          L_temp = L_add(L_temp, L_Bik);
          pppL_B[i - 1][k - 1][j_0] = L_add(L_temp, L_TmpA);

          L_temp = L_mpy_ls(L_Cik, swRcSq);
          L_temp = L_add(L_temp, L_Cik);
          pppL_C[i][k - 1][j_0] = L_add(L_temp, L_TmpB);

          for (k = i + 1; k <= NP - j - 2; k++)
          {

            /* Compute the common terms used by the FLAT recursion to reduce */
            /* computation.                                                  */
            /*---------------------------------------------------------------*/

            L_Cik = L_shl(pppL_C[i][k][j_1], swShift);
            L_Cki = L_shl(pppL_C[k][i][j_1], swShift);

            L_TmpA = L_add(L_Cik, L_Cki);
            L_TmpA = L_mpy_ls(L_TmpA, pswRc[j]);

            L_Bik = L_shl(pppL_B[i][k][j_1], swShift);
            L_Fik = L_shl(pppL_F[i][k][j_1], swShift);

            L_TmpB = L_add(L_Bik, L_Fik);
            L_TmpB = L_mpy_ls(L_TmpB, pswRc[j]);

            /* Update F, B and C arrays */
            /*-----------------------------------*/

            L_temp = L_mpy_ls(L_Bik, swRcSq);
            L_temp = L_add(L_temp, L_Fik);
            pppL_F[i][k][j_0] = L_add(L_temp, L_TmpA);

            L_temp = L_mpy_ls(L_Fik, swRcSq);
            L_temp = L_add(L_temp, L_Bik);
            pppL_B[i - 1][k - 1][j_0] = L_add(L_temp, L_TmpA);

            L_temp = L_mpy_ls(L_Cki, swRcSq);
            L_temp = L_add(L_temp, L_Cik);
            pppL_C[i][k - 1][j_0] = L_add(L_temp, L_TmpB);

            L_temp = L_mpy_ls(L_Cik, swRcSq);
            L_temp = L_add(L_temp, L_Cki);
            pppL_C[k][i - 1][j_0] = L_add(L_temp, L_TmpB);

          }                              /* end of loop indexed by k */
          /*---------------------------*/

          k = NP - j - 1;

          /* Compute the common terms used by the FLAT recursion to reduce */
          /* computation.                                                  */
          /*---------------------------------------------------------------*/

          L_Cik = L_shl(pppL_C[i][k][j_1], swShift);
          L_Cki = L_shl(pppL_C[k][i][j_1], swShift);

          L_TmpA = L_add(L_Cik, L_Cki);
          L_TmpA = L_mpy_ls(L_TmpA, pswRc[j]);

          L_Bik = L_shl(pppL_B[i][k][j_1], swShift);
          L_Fik = L_shl(pppL_F[i][k][j_1], swShift);

          L_TmpB = L_add(L_Bik, L_Fik);
          L_TmpB = L_mpy_ls(L_TmpB, pswRc[j]);

          /* Update B and C arrays */
          /*-----------------------------------*/

          L_temp = L_mpy_ls(L_Fik, swRcSq);
          L_temp = L_add(L_temp, L_Bik);
          pppL_B[i - 1][k - 1][j_0] = L_add(L_temp, L_TmpA);

          L_temp = L_mpy_ls(L_Cki, swRcSq);
          L_temp = L_add(L_temp, L_Cik);
          pppL_C[i][k - 1][j_0] = L_add(L_temp, L_TmpB);

        }                                /* end of loop indexed by i */
        /*---------------------------*/

        i = NP - j - 1;
        for (k = i; k <= NP - j - 1; k++)
        {

          /* Compute the common terms used by the FLAT recursion to reduce */
          /* computation.                                                  */
          /*---------------------------------------------------------------*/

          L_Cik = L_shl(pppL_C[i][k][j_1], swShift);

          L_TmpA = L_add(L_Cik, L_Cik);
          L_TmpA = L_mpy_ls(L_TmpA, pswRc[j]);

          /* Update B array */
          /*-----------------------------------*/

          L_Bik = L_shl(pppL_B[i][k][j_1], swShift);
          L_Fik = L_shl(pppL_F[i][k][j_1], swShift);

          L_temp = L_mpy_ls(L_Fik, swRcSq);
          L_temp = L_add(L_temp, L_Bik);
          pppL_B[i - 1][k - 1][j_0] = L_add(L_temp, L_TmpA);

        }                                /* end of loop indexed by k */
        /*-----------------------------------------------------------*/

        /* OR the F and B matrix diagonals to find maximum for normalization */
        /*********************************************************************/

        L_TmpA = 0;
        for (l = 0; l <= NP - j - 2; l++)
        {
          L_TmpA |= pppL_F[l][l][j_0];
          L_TmpA |= pppL_B[l][l][j_0];
        }
        /* Compute the shift count to be applied to F, B, and C arrays */
        /* at the next lattice stage.                                  */
        /*-------------------------------------------------------------*/

        if (L_TmpA > 0)
        {
          swShift = norm_l(L_TmpA);
          swShift = sub(swShift, CVSHIFT);
        }
        else
        {
          swShift = 0;
        }

      }                                  /* End of update of F, B, and C
                                          * arrays for the next lattice stage */
      /*----------------------------------------------------------------*/

    }                                    /* Finished computation of
                                          * reflection coefficients */
    /*--------------------------------------------------------------*/

    return (L_R0);

  }

  /**************************************************************************
   *
   *   FUNCTION NAME: fnBest_CG
   *
   *   PURPOSE:
   *     The purpose of this function is to determine the C:G pair from the
   *     input arrays which maximize C*C/G
   *
   *   INPUTS:
   *
   *     pswCframe[0:siNumPairs]
   *
   *                     pointer to start of the C frame vector
   *
   *     pswGframe[0:siNumPairs]
   *
   *                     pointer to start of the G frame vector
   *
   *     pswCmaxSqr
   *
   *                     threshold Cmax**2 or 0 if no threshold
   *
   *     pswGmax
   *
   *                     threshold Gmax, must be > 0
   *
   *     siNumPairs
   *
   *                     number of C:G pairs to test
   *
   *   OUTPUTS:
   *
   *     pswCmaxSqr
   *
   *                     final Cmax**2 value
   *
   *     pswGmax
   *
   *                     final Gmax value
   *
   *   RETURN VALUE:
   *
   *     siMaxLoc
   *
   *                     index of Cmax in the input C matrix or -1 if none
   *
   *   DESCRIPTION:
   *
   *     test the result of (C * C * Gmax) - (Cmax**2 * G)
   *     if the result is > 0 then a new max has been found
   *     the Cmax**2, Gmax and MaxLoc parameters are all updated accordingly.
   *     if no new max is found for all NumPairs then MaxLoc will retain its
   *     original value
   *
   *   REFERENCE:  Sub-clause 4.1.8.1, 4.1.8.2, and 4.1.8.3 of GSM
   *     Recommendation 06.20
   *
   *   KEYWORDS: C_Frame, G_Frame, Cmax, Gmax, DELTA_LAGS, PITCH_LAGS
   *

  ****************************************************************************/

  short int Codec::fnBest_CG(int16_t pswCframe[], int16_t pswGframe[],
                             int16_t *pswCmaxSqr, int16_t *pswGmax,
                             short int siNumPairs)
  {

  /*_________________________________________________________________________
  |                                                                           |
  |                            Automatic Variables                            |
  |___________________________________________________________________________|
  */

    int32_t L_Temp2;
    int16_t swCmaxSqr,
           swGmax,
           swTemp;
    short int siLoopCnt,
           siMaxLoc;

  /*_________________________________________________________________________
  |                                                                           |
  |                              Executable Code                              |
  |___________________________________________________________________________|
  */

    /* initialize */
    /* ---------- */

    swCmaxSqr = *pswCmaxSqr;
    swGmax = *pswGmax;
    siMaxLoc = -1;

    for (siLoopCnt = 0; siLoopCnt < siNumPairs; siLoopCnt++)
    {

      /* make sure both C and energy > 0 */
      /* ------------------------------- */

      if ((pswGframe[siLoopCnt] > 0) && (pswCframe[siLoopCnt] > 0))
      {

        /* calculate (C * C) */
        /* ----------------- */

        swTemp = mult_r(pswCframe[siLoopCnt], pswCframe[siLoopCnt]);

        /* calculate (C * C * Gmax) */
        /* ------------------------ */

        L_Temp2 = L_mult(swTemp, swGmax);

        /* calculate (C * C * Gmax) - (Cmax**2 * G) */
        /* ----------------------------------------- */

        L_Temp2 = L_msu(L_Temp2, swCmaxSqr, pswGframe[siLoopCnt]);

        /* if new max found, update it and its location */
        /* -------------------------------------------- */

        if (L_Temp2 > 0)
        {
          swCmaxSqr = swTemp;            /* Cmax**2 = current C * C */
          swGmax = pswGframe[siLoopCnt]; /* Gmax */
          siMaxLoc = siLoopCnt;          /* max location = current (C)
                                          * location */
        }
      }
    }

    /* set output */
    /* ---------- */

    *pswCmaxSqr = swCmaxSqr;
    *pswGmax = swGmax;
    return (siMaxLoc);
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: fnExp2
   *
   *   PURPOSE:
   *     The purpose of this function is to implement a base two exponential
   *     2**(32*x) by polynomial approximation
   *
   *
   *   INPUTS:
   *
   *     L_Input
   *
   *                     unnormalized input exponent (input range constrained
   *                     to be < 0; for input < -0.46 output is 0)
   *
   *   OUTPUTS:
   *
   *     none
   *
   *   RETURN VALUE:
   *
   *     swTemp4
   *
   *                     exponential output
   *
   *   DESCRIPTION:
   *
   *     polynomial approximation is used for the generation of the exponential
   *
   *     2**(32*X) = 0.1713425*X*X + 0.6674432*X + 0.9979554
   *                     c2              c1            c0
   *
   *   REFERENCE:  Sub-clause 4.1.8.2 of GSM Recommendation 06.20, eqn 3.9
   *
   *   KEYWORDS: EXP2, DELTA_LAGS
   *
   *************************************************************************/

  int16_t Codec::fnExp2(int32_t L_Input)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                           Local Static Variables                        |
   |_________________________________________________________________________|
  */
    static int16_t pswPCoefE[3] =
    {                                    /* c2,   c1,    c0 */
      0x15ef, 0x556f, 0x7fbd
    };

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swTemp1,
           swTemp2,
           swTemp3,
           swTemp4;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* initialize */
    /* ---------- */

    swTemp3 = 0x0020;

    /* determine normlization shift count */
    /* ---------------------------------- */

    swTemp1 = extract_h(L_Input);
    L_Input = L_mult(swTemp1, swTemp3);
    swTemp2 = extract_h(L_Input);

    /* determine un-normalized shift count */
    /* ----------------------------------- */

    swTemp3 = -0x0001;
    swTemp4 = sub(swTemp3, swTemp2);

    /* normalize input */
    /* --------------- */

    L_Input = L_Input & LSP_MASK;
    L_Input = L_add(L_Input, L_deposit_h(swTemp3));

    L_Input = L_shr(L_Input, 1);
    swTemp1 = extract_l(L_Input);

    /* calculate x*x*c2 */
    /* ---------------- */

    swTemp2 = mult_r(swTemp1, swTemp1);
    L_Input = L_mult(swTemp2, pswPCoefE[0]);

    /* calculate x*x*c2 + x*c1 */
    /* ----------------------- */

    L_Input = L_mac(L_Input, swTemp1, pswPCoefE[1]);

    /* calculate x*x*c2 + x*c1 + c0 */
    /* --------------------------- */

    L_Input = L_add(L_Input, L_deposit_h(pswPCoefE[2]));

    /* un-normalize exponent if its requires it */
    /* ---------------------------------------- */

    if (swTemp4 > 0)
    {
      L_Input = L_shr(L_Input, swTemp4);
    }

    /* return result */
    /* ------------- */

    swTemp4 = extract_h(L_Input);
    return (swTemp4);
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: fnLog2
   *
   *   PURPOSE:
   *     The purpose of this function is to take the log base 2 of input and
   *     divide by 32 and return; i.e. output = log2(input)/32
   *
   *   INPUTS:
   *
   *     L_Input
   *
   *                     input
   *
   *   OUTPUTS:
   *
   *     none
   *
   *   RETURN VALUE:
   *
   *     int16_t
   *
   *                     output
   *
   *   DESCRIPTION:
   *
   *     log2(x) = 4.0 * (-.3372223*x*x + .9981958*x -.6626105)
   *                           c0            c1          c2   (includes sign)
   *
   *   REFERENCE:  Sub-clause 4.1.8.2 of GSM Recommendation 06.20, eqn 3.9
   *
   *   KEYWORDS: log, logarithm, logbase2, fnLog2
   *
   *************************************************************************/

  int16_t Codec::fnLog2(int32_t L_Input)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                           Static Variables                              |
   |_________________________________________________________________________|
  */

    static int16_t
           swC0 = -0x2b2a,
           swC1 = 0x7fc5,
           swC2 = -0x54d0;

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    short int siShiftCnt;
    int16_t swInSqrd,
           swIn;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* normalize input and store shifts required */
    /* ----------------------------------------- */

    siShiftCnt = norm_l(L_Input);
    L_Input = L_shl(L_Input, siShiftCnt);
    siShiftCnt = add(siShiftCnt, 1);
    siShiftCnt = negate(siShiftCnt);

    /* calculate x*x*c0 */
    /* ---------------- */

    swIn = extract_h(L_Input);
    swInSqrd = mult_r(swIn, swIn);
    L_Input = L_mult(swInSqrd, swC0);

    /* add x*c1 */
    /* --------- */

    L_Input = L_mac(L_Input, swIn, swC1);

    /* add c2 */
    /* ------ */

    L_Input = L_add(L_Input, L_deposit_h(swC2));

    /* apply *(4/32) */
    /* ------------- */

    L_Input = L_shr(L_Input, 3);
    L_Input = L_Input & 0x03ffffff;
    siShiftCnt = shl(siShiftCnt, 10);
    L_Input = L_add(L_Input, L_deposit_h(siShiftCnt));

    /* return log */
    /* ---------- */

    return (round(L_Input));
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: getCCThreshold
   *
   *   PURPOSE:
   *     The purpose of this function is to calculate a threshold for other
   *     correlations (subject to limits), given subframe energy (Rp0),
   *     correlation squared (CC), and energy of delayed sequence (G)
   *
   *   INPUTS:
   *
   *     swRp0
   *
   *                     energy of the subframe
   *
   *     swCC
   *
   *                     correlation (squared) of subframe and delayed sequence
   *
   *     swG
   *
   *                     energy of delayed sequence
   *
   *   OUTPUTS:
   *
   *     none
   *
   *   RETURN VALUE:
   *
   *     swCCThreshold
   *
   *                     correlation (squared) threshold
   *
   *   DESCRIPTION:
   *
   *     CCt/0.5 = R - R(antilog(SCALE*log(max(CLAMP,(RG-CC)/RG))))
   *
   *     The threshold CCt is then applied with an understood value of Gt = 0.5
   *
   *   REFERENCE:  Sub-clause 4.1.8.2 of GSM Recommendation 06.20, eqn 3.9
   *
   *   KEYWORDS: getCCThreshold, getccthreshold, GET_CSQ_THRES
   *
   *************************************************************************/

  int16_t Codec::getCCThreshold(int16_t swRp0,
                                  int16_t swCC,
                                  int16_t swG)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swPGainClamp,
           swPGainScale,
           sw1;
    int32_t L_1;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* load CLAMP and SCALE */
    /* -------------------- */

    swPGainClamp = PGAIN_CLAMP;
    swPGainScale = PGAIN_SCALE;

    /* calculate RG-CC */
    /* --------------- */

    L_1 = L_mult(swRp0, swG);
    sw1 = extract_h(L_1);
    L_1 = L_sub(L_1, L_deposit_h(swCC));

    /* if RG - CC > 0 do max(CLAMP, (RG-CC)/RG) */
    /* ---------------------------------------- */

    if (L_1 > 0)
    {

      sw1 = divide_s(extract_h(L_1), sw1);

      L_1 = L_deposit_h(sw1);

      if (sub(sw1, swPGainClamp) <= 0)
      {
        L_1 = L_deposit_h(swPGainClamp);
      }
    }
    /* else max(CLAMP, (RG-CC)/RG) is CLAMP */
    /* ------------------------------------ */

    else
    {
      L_1 = L_deposit_h(swPGainClamp);
    }

    /* L_1 holds max(CLAMP, (RG-CC)/RG)   */
    /* do antilog( SCALE * log( max() ) ) */
    /* ---------------------------------- */

    sw1 = fnLog2(L_1);

    L_1 = L_mult(sw1, swPGainScale);

    sw1 = fnExp2(L_1);


    /* do R - (R * antilog()) */
    /* ---------------------- */

    L_1 = L_deposit_h(swRp0);
    L_1 = L_msu(L_1, swRp0, sw1);

    /* apply Gt value */
    /* -------------- */

    L_1 = L_shr(L_1, 1);

    return (extract_h(L_1));
  }


  /***************************************************************************
   *
   *   FUNCTION NAME: getNWCoefs
   *
   *   PURPOSE:
   *
   *     Obtains best all-pole fit to various noise weighting
   *     filter combinations
   *
   *   INPUTS:
   *
   *     pswACoefs[0:9] - A(z) coefficient array
   *     psrNWCoefs[0:9] - filter smoothing coefficients
   *
   *   OUTPUTS:
   *
   *     pswHCoefs[0:9] - H(z) coefficient array
   *
   *   RETURN VALUE:
   *
   *     None
   *
   *   DESCRIPTION:
   *
   *     The function getNWCoefs() derives the spectral noise weighting
   *     coefficients W(z)and H(z).  W(z) and H(z) actually consist of
   *     three filters in cascade.  To avoid having such a complicated
   *     filter required for weighting, the filters are reduced to a
   *     single filter.
   *
   *     This is accomplished by passing an impulse through the cascased
   *     filters.  The impulse response of the filters is used to generate
   *     autocorrelation coefficients, which are then are transformed into
   *     a single direct form estimate of W(z) and H(z).  This estimate is
   *     called HHat(z) in the documentation.
   *
   *
   *   REFERENCE:  Sub-clause 4.1.7 of GSM Recommendation 06.20
   *
   *   KEYWORDS: spectral noise weighting, direct form coefficients
   *   KEYWORDS: getNWCoefs
   *
   *************************************************************************/

  void Codec::getNWCoefs(int16_t pswACoefs[],
                           int16_t pswHCoefs[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t pswCoefTmp2[NP],
           pswCoefTmp3[NP],
           pswVecTmp[S_LEN],
           pswVecTmp2[S_LEN],
           pswTempRc[NP];
    int16_t swNormShift,
           iLoopCnt,
           iLoopCnt2;
    int32_t pL_AutoCorTmp[NP + 1],
           L_Temp;
    short int siNum,
           k;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Calculate smoothing parameters for all-zero filter */
    /* -------------------------------------------------- */

    for (iLoopCnt = 0; iLoopCnt < NP; iLoopCnt++)
    {
      pswCoefTmp2[iLoopCnt]
              = mult_r(psrNWCoefs[iLoopCnt], pswACoefs[iLoopCnt]);
    }

    /* Calculate smoothing parameters for all-pole filter */
    /* -------------------------------------------------- */

    for (iLoopCnt = 0; iLoopCnt < NP; iLoopCnt++)
    {
      pswCoefTmp3[iLoopCnt] = msu_r(0, psrNWCoefs[iLoopCnt + NP],
                                    pswACoefs[iLoopCnt]);
    }

    /* Get impulse response of 1st filter                             */
    /* Done by direct form IIR filter of order NP zero input response */
    /* -------------------------------------------------------------- */

    lpcIrZsIir(pswACoefs, pswVecTmp2);

    /* Send impulse response of 1st filter through 2nd filter */
    /* All-zero filter (FIR)                                  */
    /* ------------------------------------------------------ */

    lpcZsFir(pswVecTmp2, pswCoefTmp2, pswVecTmp);

    /* Send impulse response of 2nd filter through 3rd filter */
    /* All-pole filter (IIR)                                  */
    /* ------------------------------------------------------ */

    lpcZsIirP(pswVecTmp, pswCoefTmp3);

    /* Calculate energy in impulse response */
    /* ------------------------------------ */

    swNormShift = g_corr1(pswVecTmp, &L_Temp);

    pL_AutoCorTmp[0] = L_Temp;

    /* Calculate normalized autocorrelation function */
    /* --------------------------------------------- */

    for (k = 1; k <= NP; k++)
    {

      /* Calculate R(k), equation 2.31 */
      /* ----------------------------- */

      L_Temp = L_mult(pswVecTmp[0], pswVecTmp[0 + k]);

      for (siNum = S_LEN - k, iLoopCnt2 = 1; iLoopCnt2 < siNum; iLoopCnt2++)
      {
        L_Temp = L_mac(L_Temp, pswVecTmp[iLoopCnt2],
                       pswVecTmp[iLoopCnt2 + k]);
      }

      /* Normalize R(k) relative to R(0): */
      /* -------------------------------- */

      pL_AutoCorTmp[k] = L_shl(L_Temp, swNormShift);

    }


    /* Convert normalized autocorrelations to direct form coefficients */
    /* --------------------------------------------------------------- */

    aFlatRcDp(pL_AutoCorTmp, pswTempRc);
    rcToADp(ASCALE, pswTempRc, pswHCoefs);

  }

  /***************************************************************************
   *
   *   FUNCTION NAME: getNextVec
   *
   *   PURPOSE:
   *     The purpose of this function is to get the next vector in the list.
   *
   *   INPUTS: none
   *
   *   OUTPUTS: pswRc
   *
   *   RETURN VALUE: none
   *
   *   DESCRIPTION:
   *
   *     Both the quantizer and pre-quantizer are set concatenated 8 bit
   *     words.  Each of these words represents a reflection coefficient.
   *     The 8 bit words, are actually indices into a reflection
   *     coefficient lookup table.  Memory is organized in 16 bit words, so
   *     there are two reflection coefficients per ROM word.
   *
   *
   *     The full quantizer is subdivided into blocks.  Each of the
   *     pre-quantizers vectors "points" to a full quantizer block.  The
   *     vectors in a block, are comprised of either three or four
   *     elements.  These are concatenated, without leaving any space
   *     between them.
   *
   *     A block of full quantizer elements always begins on an even word.
   *     This may or may not leave a space depending on vector quantizer
   *     size.
   *
   *     getNextVec(), serves to abstract this arcane data format.  Its
   *     function is to simply get the next reflection coefficient vector
   *     in the list, be it a pre or full quantizer list.  This involves
   *     figuring out whether to pick the low or the high part of the 16
   *     bit ROM word.  As well as transforming the 8 bit stored value
   *     into a fractional reflection coefficient.  It also requires a
   *     setup routine to initialize iWordPtr and iWordHalfPtr, two
   *     variables global to this file.
   *
   *
   *
   *   REFERENCE:  Sub-clause 4.1.4.1 of GSM Recommendation 06.20
   *
   *   KEYWORDS: Quant quant vector quantizer
   *
   *************************************************************************/

  void Codec::getNextVec(int16_t pswRc[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */
    int    i;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */
    i = iLow;

    if (iThree)
    {

      if (iWordHalfPtr == HIGH)
      {
        pswRc[i++] = psrSQuant[high(psrTable[iWordPtr])];
        pswRc[i++] = psrSQuant[low(psrTable[iWordPtr++])];
        pswRc[i] = psrSQuant[high(psrTable[iWordPtr])];
        iWordHalfPtr = LOW;
      }
      else
      {
        pswRc[i++] = psrSQuant[low(psrTable[iWordPtr++])];
        pswRc[i++] = psrSQuant[high(psrTable[iWordPtr])];
        pswRc[i] = psrSQuant[low(psrTable[iWordPtr++])];
        iWordHalfPtr = HIGH;
      }

    }
    else
    {
      pswRc[i++] = psrSQuant[high(psrTable[iWordPtr])];
      pswRc[i++] = psrSQuant[low(psrTable[iWordPtr++])];
      pswRc[i++] = psrSQuant[high(psrTable[iWordPtr])];
      pswRc[i] = psrSQuant[low(psrTable[iWordPtr++])];
    }
  }

  /***************************************************************************
   *
   *    FUNCTION NAME: getSfrmLpcTx
   *
   *    PURPOSE:
   *       Given frame information from past and present frame, interpolate
   *       (or copy) the frame based lpc coefficients into subframe
   *       lpc coeffs, i.e. the ones which will be used by the subframe
   *       as opposed to those coded and transmitted.
   *
   *    INPUT:
   *       swPrevR0,swNewR0 - Rq0 for the last frame and for this frame.
   *          These are the decoded values, not the codewords.
   *
   *       Previous lpc coefficients from the previous FRAME:
   *          in all filters below array[0] is the t=-1 element array[NP-1]
   *        t=-NP element.
   *       pswPrevFrmKs[NP] - decoded version of the rc's tx'd last frame
   *       pswPrevFrmAs[NP] - the above K's converted to A's.  i.e. direct
   *          form coefficients.
   *       pswPrevFrmSNWCoef[NP] - Coefficients for the Spectral Noise
   *          weighting filter from the previous frame
   *
   *       pswHPFSppech - pointer to High Pass Filtered Input speech
   *
   *       pswSoftInterp - a flag containing the soft interpolation
   *          decision.
   *
   *       Current lpc coefficients from the current frame:
   *       pswNewFrmKs[NP],pswNewFrmAs[NP],
   *       pswNewFrmSNWCoef[NP] - Spectral Noise Weighting Coefficients
   *           for the current frame
   *       ppswSNWCoefAs[1][NP] - pointer into a matrix containing
   *           the interpolated and uninterpolated LP Coefficient
   *           values for the Spectral Noise Weighting Filter.
   *
   *    OUTPUT:
   *       psnsSqrtRs[N_SUB] - a normalized number (struct NormSw)
   *          containing an estimate
   *          of RS for each subframe.  (number and a shift)
   *
   *       ppswSynthAs[N_SUM][NP] - filter coefficients used by the
   *          synthesis filter.
   *
   *    DESCRIPTION:
   *        For interpolated subframes, the direct form coefficients
   *        are converted to reflection coeffiecients to check for
   *        filter stability. If unstable, the uninterpolated coef.
   *        are used for that subframe.
   *
   *
   *    REFERENCE: Sub-clause of 4.1.6 and 4.1.7 of GSM Recommendation
   *        06.20
   *
   *    KEYWORDS: soft interpolation, int_lpc, interpolate, atorc, res_eng
   *
   *************************************************************************/


  void Codec::getSfrmLpcTx(int16_t swPrevR0, int16_t swNewR0,
  /* last frm*/
                         int16_t pswPrevFrmKs[], int16_t pswPrevFrmAs[],
                             int16_t pswPrevFrmSNWCoef[],
  /* this frm*/
                           int16_t pswNewFrmKs[], int16_t pswNewFrmAs[],
                             int16_t pswNewFrmSNWCoef[],
                             int16_t pswHPFSpeech[],
  /* output */
                             short *pswSoftInterp,
                             struct NormSw *psnsSqrtRs,
                 int16_t ppswSynthAs[][NP], int16_t ppswSNWCoefAs[][NP])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swSi;
    int32_t L_Temp;
    short int siSfrm,
           siStable,
           i;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* perform interpolation - both for synth filter and noise wgt filt */
    /*------------------------------------------------------------------*/
    siSfrm = 0;
    siStable = interpolateCheck(pswPrevFrmKs, pswPrevFrmAs,
                                pswPrevFrmAs, pswNewFrmAs,
                                psrOldCont[siSfrm], psrNewCont[siSfrm],
                                swPrevR0,
                                &psnsSqrtRs[siSfrm],
                                ppswSynthAs[siSfrm]);
    if (siStable)
    {
      for (i = 0; i < NP; i++)
      {
        L_Temp = L_mult(pswNewFrmSNWCoef[i], psrNewCont[siSfrm]);
        ppswSNWCoefAs[siSfrm][i] = mac_r(L_Temp, pswPrevFrmSNWCoef[i],
                                         psrOldCont[siSfrm]);
      }
    }
    else
    {

      /* this subframe is unstable */
      /*---------------------------*/

      for (i = 0; i < NP; i++)
      {
        ppswSNWCoefAs[siSfrm][i] = pswPrevFrmSNWCoef[i];
      }

    }

    /* interpolate subframes one and two */
    /*-----------------------------------*/

    for (siSfrm = 1; siSfrm < N_SUB - 1; siSfrm++)
    {
      siStable = interpolateCheck(pswNewFrmKs, pswNewFrmAs,
                                  pswPrevFrmAs, pswNewFrmAs,
                                  psrOldCont[siSfrm], psrNewCont[siSfrm],
                                  swNewR0,
                                  &psnsSqrtRs[siSfrm],
                                  ppswSynthAs[siSfrm]);
      if (siStable)
      {
        for (i = 0; i < NP; i++)
        {
          L_Temp = L_mult(pswNewFrmSNWCoef[i], psrNewCont[siSfrm]);
          ppswSNWCoefAs[siSfrm][i] = mac_r(L_Temp, pswPrevFrmSNWCoef[i],
                                           psrOldCont[siSfrm]);
        }
      }
      else
      {
        /* this sfrm has unstable filter coeffs, would like to interp but
         * cant */
        /*--------------------------------------*/

        for (i = 0; i < NP; i++)
        {
          ppswSNWCoefAs[siSfrm][i] = pswNewFrmSNWCoef[i];
        }

      }
    }


    /* the last subframe: never interpolate. */
    /*--------------------------------------*/

    siSfrm = 3;
    for (i = 0; i < NP; i++)
    {
      ppswSNWCoefAs[siSfrm][i] = pswNewFrmSNWCoef[i];
      ppswSynthAs[siSfrm][i] = pswNewFrmAs[i];
    }

    /* calculate the residual energy for the last subframe */
    /*-----------------------------------------------------*/

    res_eng(pswNewFrmKs, swNewR0, &psnsSqrtRs[siSfrm]);



    /* done with interpolation, now compare the two sets of coefs.   */
    /* make the decision whether to interpolate (1) or not (0)       */
    /*---------------------------------------------------------------*/

    swSi = compResidEnergy(pswHPFSpeech,
                           ppswSynthAs, pswPrevFrmAs,
                           pswNewFrmAs, psnsSqrtRs);

    if (swSi == 0)
    {

      /* no interpolation done: copy the frame based data to output
       * coeffiecient arrays */

      siSfrm = 0;
      for (i = 0; i < NP; i++)
      {
        ppswSNWCoefAs[siSfrm][i] = pswPrevFrmSNWCoef[i];
        ppswSynthAs[siSfrm][i] = pswPrevFrmAs[i];
      }

      /* get RS (energy in the residual) for subframe 0 */
      /*------------------------------------------------*/

      res_eng(pswPrevFrmKs, swPrevR0, &psnsSqrtRs[siSfrm]);

      /* for subframe 1 and all subsequent sfrms, use lpc and R0 from new frm */
      /*---------------------------------------------------------------------*/

      res_eng(pswNewFrmKs, swNewR0, &psnsSqrtRs[1]);

      for (siSfrm = 2; siSfrm < N_SUB; siSfrm++)
        psnsSqrtRs[siSfrm] = psnsSqrtRs[1];

      for (siSfrm = 1; siSfrm < N_SUB; siSfrm++)
      {
        for (i = 0; i < NP; i++)
        {
          ppswSNWCoefAs[siSfrm][i] = pswNewFrmSNWCoef[i];
          ppswSynthAs[siSfrm][i] = pswNewFrmAs[i];
        }
      }
    }

    *pswSoftInterp = swSi;
  }

  /***************************************************************************
   *
   *    FUNCTION NAME: iir_d
   *
   *    PURPOSE:  Performs one second order iir section using double-precision.
   *              feedback,single precision xn and filter coefficients
   *
   *    INPUTS:
   *
   *      pswCoef[0:4]   An array of filter coefficients.
   *
   *      pswIn[0:159]   An array of input samples to be filtered, filtered
   *                     output samples written to the same array.
   *
   *      pswXstate[0:1] An array containing x-state memory.
   *
   *      pswYstate[0:3] An array containing y-state memory.
   *
   *      npts           Number of samples to filter (must be even).
   *
   *      shifts         number of shifts to be made on output y(n) before
   *                     storing to y(n) states.
   *
   *      swPreFirDownSh number of shifts apply to signal before the FIR.
   *
   *      swFinalUpShift number of shifts apply to signal before outputting.
   *
   *    OUTPUTS:
   *
   *       pswIn[0:159]  Output array containing the filtered input samples.
   *
   *    RETURN:
   *
   *       none.
   *
   *    DESCRIPTION:
   *
   *       Transfer function implemented:
   *         (b0 + b1*z-1 + b2*z-2)/(a0 - a1*z-1 - a2*z-2+
   *       data structure:
   *         Coeff array order:  b2,b1,b0,a2,a1
   *         Xstate array order: x(n-2),x(n-1)
   *         Ystate array order: y(n-2)MSB,y(n-2)LSB,y(n-1)MSB,y(n-1)LSB
   *
   *       There is no elaborate discussion of the filter, since it is
   *       trivial.
   *
   *       The filter's cutoff frequency is 120 Hz.
   *
   *    REFERENCE:  Sub-clause 4.1.1 GSM Recommendation 06.20
   *
   *    KEYWORDS: highpass filter, hp, HP, filter
   *
   *************************************************************************/

  void Codec::iir_d(int16_t pswCoeff[], int16_t pswIn[], int16_t pswXstate[],
                      int16_t pswYstate[], int npts, int shifts,
                      int16_t swPreFirDownSh, int16_t swFinalUpShift)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int    loop_cnt;
    int32_t L_sumA,
           L_sumB;
    int16_t swTemp,
           pswYstate_0,
           pswYstate_1,
           pswYstate_2,
           pswYstate_3,
           pswXstate_0,
           pswXstate_1,
           swx0,
           swx1;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* initialize the temporary state variables */
    /*------------------------------------------*/

    pswYstate_0 = pswYstate[0];
    pswYstate_1 = pswYstate[1];
    pswYstate_2 = pswYstate[2];
    pswYstate_3 = pswYstate[3];

    pswXstate_0 = pswXstate[0];
    pswXstate_1 = pswXstate[1];

    for (loop_cnt = 0; loop_cnt < npts; loop_cnt += 2)
    {

      swx0 = shr(pswIn[loop_cnt], swPreFirDownSh);
      swx1 = shr(pswIn[loop_cnt + 1], swPreFirDownSh);

      L_sumB = L_mult(pswYstate_1, pswCoeff[3]);
      L_sumB = L_mac(L_sumB, pswYstate_3, pswCoeff[4]);
      L_sumB = L_shr(L_sumB, 14);
      L_sumB = L_mac(L_sumB, pswYstate_0, pswCoeff[3]);
      L_sumB = L_mac(L_sumB, pswYstate_2, pswCoeff[4]);


      L_sumA = L_mac(L_sumB, pswCoeff[0], pswXstate_0);
      L_sumA = L_mac(L_sumA, pswCoeff[1], pswXstate_1);
      L_sumA = L_mac(L_sumA, pswCoeff[2], swx0);

      L_sumA = L_shl(L_sumA, shifts);

      pswXstate_0 = swx0;                /* Update X state x(n-1) <- x(n) */

      /* Update double precision Y state temporary variables */
      /*-----------------------------------------------------*/

      pswYstate_0 = extract_h(L_sumA);
      swTemp = extract_l(L_sumA);
      swTemp = shr(swTemp, 2);
      pswYstate_1 = 0x3fff & swTemp;

      /* Round, store output sample and increment to next input sample */
      /*---------------------------------------------------------------*/

      pswIn[loop_cnt] = round(L_shl(L_sumA, swFinalUpShift));

      L_sumB = L_mult(pswYstate_3, pswCoeff[3]);
      L_sumB = L_mac(L_sumB, pswYstate_1, pswCoeff[4]);
      L_sumB = L_shr(L_sumB, 14);
      L_sumB = L_mac(L_sumB, pswYstate_2, pswCoeff[3]);
      L_sumB = L_mac(L_sumB, pswYstate_0, pswCoeff[4]);


      L_sumA = L_mac(L_sumB, pswCoeff[0], pswXstate_1);
      L_sumA = L_mac(L_sumA, pswCoeff[1], pswXstate_0);
      L_sumA = L_mac(L_sumA, pswCoeff[2], swx1);

      L_sumA = L_shl(L_sumA, shifts);

      pswXstate_1 = swx1;                /* Update X state x(n-1) <- x(n) */

      /* Update double precision Y state temporary variables */
      /*-----------------------------------------------------*/

      pswYstate_2 = extract_h(L_sumA);

      swTemp = extract_l(L_sumA);
      swTemp = shr(swTemp, 2);
      pswYstate_3 = 0x3fff & swTemp;

      /* Round, store output sample and increment to next input sample */
      /*---------------------------------------------------------------*/

      pswIn[loop_cnt + 1] = round(L_shl(L_sumA, swFinalUpShift));

    }

    /* update the states for the next frame */
    /*--------------------------------------*/

    pswYstate[0] = pswYstate_0;
    pswYstate[1] = pswYstate_1;
    pswYstate[2] = pswYstate_2;
    pswYstate[3] = pswYstate_3;

    pswXstate[0] = pswXstate_0;
    pswXstate[1] = pswXstate_1;

  }

  /***************************************************************************
   *
   *    FUNCTION NAME: initPBarVBarFullL
   *
   *    PURPOSE:  Given the int32_t normalized correlation sequence, function
   *              initPBarVBarL initializes the int32_t initial condition
   *              arrays pL_PBarFull and pL_VBarFull for a full 10-th order LPC
   *              filter. It also shifts down the pL_VBarFull and pL_PBarFull
   *              arrays by a global constant RSHIFT bits. The pL_PBarFull and
   *              pL_VBarFull arrays are used to set the initial int16_t
   *              P and V conditions which are used in the actual search of the
   *              Rc prequantizer and the Rc quantizer.
   *
   *              This is an implementation of equations 4.14 and
   *              4.15.
   *
   *    INPUTS:
   *
   *        pL_CorrelSeq[0:NP]
   *                     A int32_t normalized autocorrelation array computed
   *                     from unquantized reflection coefficients.
   *
   *       RSHIFT       The number of right shifts to be applied to the
   *                     input correlations prior to initializing the elements
   *                     of pL_PBarFull and pL_VBarFull output arrays. RSHIFT
   *                     is a global constant.
   *
   *    OUTPUTS:
   *
   *        pL_PBarFull[0:NP-1]
   *                     A int32_t output array containing the P initial
   *                     conditions for the full 10-th order LPC filter.
   *                     The address of the 0-th element of  pL_PBarFull
   *                     is passed in when function initPBarVBarFullL is
   *                     called.
   *
   *        pL_VBarFull[-NP+1:NP-1]
   *                     A int32_t output array containing the V initial
   *                     conditions for the full 10-th order LPC filter.
   *                     The address of the 0-th element of pL_VBarFull is
   *                     passed in when function initPBarVBarFullL is called.
   *    RETURN:
   *        none.
   *
   *    REFERENCE:  Sub-clause 4.1.4.1 GSM Recommendation 06.20
   *
   *************************************************************************/

  void Codec::initPBarFullVBarFullL(int32_t pL_CorrelSeq[],
                                      int32_t pL_PBarFull[],
                                      int32_t pL_VBarFull[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int    i,
           bound;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Initialize the AFLAT recursion PBarFull and VBarFull 32 bit arrays */
    /* for a 10-th order LPC filter.                                      */
    /*--------------------------------------------------------------------*/

    bound = NP - 1;

    for (i = 0; i <= bound; i++)
    {
      pL_PBarFull[i] = L_shr(pL_CorrelSeq[i], RSHIFT);
    }

    for (i = -bound; i < 0; i++)
    {
      pL_VBarFull[i] = pL_PBarFull[-i - 1];
    }

    for (i = 0; i < bound; i++)
    {
      pL_VBarFull[i] = pL_PBarFull[i + 1];
    }

    pL_VBarFull[bound] = L_shr(pL_CorrelSeq[bound + 1], RSHIFT);

  }

  /***************************************************************************
   *
   *    FUNCTION NAME: initPBarVBarL
   *
   *    PURPOSE:  Given the int32_t pL_PBarFull array,
   *              function initPBarVBarL initializes the int16_t initial
   *              condition arrays pswPBar and pswVBar for a 3-rd order LPC
   *              filter, since the order of the 1st Rc-VQ segment is 3.
   *              The pswPBar and pswVBar arrays are a int16_t subset
   *              of the initial condition array pL_PBarFull.
   *              pswPBar and pswVBar are the initial conditions for the AFLAT
   *              recursion at a given segment. The AFLAT recursion is used
   *              to evaluate the residual error due to an Rc vector selected
   *              from a prequantizer or a quantizer.
   *
   *              This is an implementation of equation 4.18 and 4.19.
   *
   *    INPUTS:
   *
   *        pL_PBarFull[0:NP-1]
   *                     A int32_t input array containing the P initial
   *                     conditions for the full 10-th order LPC filter.
   *                     The address of the 0-th element of  pL_PBarFull
   *                     is passed in when function initPBarVBarL is called.
   *
   *    OUTPUTS:
   *
   *        pswPBar[0:NP_AFLAT-1]
   *                     The output int16_t array containing the P initial
   *                     conditions for the P-V AFLAT recursion, set here
   *                     for the Rc-VQ search at the 1st Rc-VQ segment.
   *                     The address of the 0-th element of pswPBar is
   *                     passed in when function initPBarVBarL is called.
   *
   *        pswVBar[-NP_AFLAT+1:NP_AFLAT-1]
   *                     The output int16_t array containing the V initial
   *                     conditions for the P-V AFLAT recursion, set here
   *                     for the Rc-VQ search at the 1st Rc-VQ segment.
   *                     The address of the 0-th element of pswVBar is
   *                     passed in when function initPBarVBarL is called.
   *
   *    RETURN:
   *
   *        none.
   *
   *    REFERENCE:  Sub-clause 4.1.4.1 GSM Recommendation 06.20
   *
   *
   *************************************************************************/

  void Codec::initPBarVBarL(int32_t pL_PBarFull[],
                              int16_t pswPBar[],
                              int16_t pswVBar[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int    bound,
           i;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Initialize the AFLAT recursion P and V 16 bit arrays for a 3-rd    */
    /* order LPC filter corresponding to the 1-st reflection coefficient  */
    /* VQ segment. The PBar and VBar arrays store the initial conditions  */
    /* for the evaluating the residual error due to Rc vectors being      */
    /* evaluated from the Rc-VQ codebook at the 1-st Rc-VQ segment.       */
    /*--------------------------------------------------------------------*/
    bound = 2;

    for (i = 0; i <= bound; i++)
    {
      pswPBar[i] = round(pL_PBarFull[i]);
    }
    for (i = -bound; i < 0; i++)
    {
      pswVBar[i] = pswPBar[-i - 1];
    }
    for (i = 0; i < bound; i++)
    {
      pswVBar[i] = pswPBar[i + 1];
    }
    pswVBar[bound] = round(pL_PBarFull[bound + 1]);

  }

  /***************************************************************************
   *
   *   FUNCTION NAME: maxCCOverGWithSign
   *
   *   PURPOSE:
   *
   *     Finds lag which maximizes C^2/G ( C is allowed to be negative ).
   *
   *   INPUTS:
   *
   *     pswCIn[0:swNum-1]
   *
   *                     Array of C values
   *
   *     pswGIn[0:swNum-1]
   *
   *                     Array of G values
   *
   *     pswCCmax
   *
   *                     Initial value of CCmax
   *
   *     pswGmax
   *
   *                     Initial value of Gmax
   *
   *     swNum
   *
   *                     Number of lags to be searched
   *
   *   OUTPUTS:
   *
   *     pswCCmax
   *
   *                     Value of CCmax after search
   *
   *     pswGmax
   *
   *                     Value of Gmax after search
   *
   *   RETURN VALUE:
   *
   *     maxCCGIndex - index for max C^2/G, defaults to zero if all G <= 0
   *
   *   DESCRIPTION:
   *
   *     This routine is called from bestDelta().  The routine is a simple
   *     find the best in a list search.
   *
   *   REFERENCE:  Sub-clause 4.1.8.3 of GSM Recommendation 06.20
   *
   *   KEYWORDS: LTP correlation peak
   *
   *************************************************************************/

  int16_t Codec::maxCCOverGWithSign(int16_t pswCIn[],
                                      int16_t pswGIn[],
                                      int16_t *pswCCMax,
                                      int16_t *pswGMax,
                                      int16_t swNum)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swCC,
           i,
           maxCCGIndex,
           swCCMax,
           swGMax;
    int32_t L_Temp;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* initialize max c^2/g to be the initial trajectory */
    /*---------------------------------------------------*/

    maxCCGIndex = 0;
    swGMax = pswGIn[0];

    if (pswCIn[0] < 0)
      swCCMax = negate(mult_r(pswCIn[0], pswCIn[0]));
    else
      swCCMax = mult_r(pswCIn[0], pswCIn[0]);

    for (i = 1; i < swNum; i++)
    {

      /* Imperfect interpolation can result in negative energies. */
      /* Check for this                                             */
      /*----------------------------------------------------------*/

      if (pswGIn[i] > 0)
      {

        swCC = mult_r(pswCIn[i], pswCIn[i]);

        if (pswCIn[i] < 0)
          swCC = negate(swCC);

        L_Temp = L_mult(swCC, swGMax);
        L_Temp = L_msu(L_Temp, pswGIn[i], swCCMax);

        /* Check if C^2*Gmax - G*Cmax^2 > 0 */
        /* -------------------------------- */

        if (L_Temp > 0)
        {
          swGMax = pswGIn[i];
          swCCMax = swCC;
          maxCCGIndex = i;
        }
      }
    }
    *pswGMax = swGMax;
    *pswCCMax = swCCMax;

    return (maxCCGIndex);
  }                                      /* end of maxCCOverGWithSign */

  /***************************************************************************
   *
   *   FUNCTION NAME: openLoopLagSearch
   *
   *   PURPOSE:
   *
   *     Determines voicing level for the frame.  If voiced, obtains list of
   *     lags to be searched in closed-loop lag search; and value of smoothed
   *     pitch and coefficient for harmonic-noise-weighting.
   *
   *   INPUTS:
   *
   *     pswWSpeech[-145:159] ( [-LSMAX:F_LEN-1] )
   *
   *                     W(z) filtered speech frame, with some history.
   *
   *     swPrevR0Index
   *
   *                     Index of R0 from the previous frame.
   *
   *     swCurrR0Index
   *
   *                     Index of R0 for the current frame.
   *
   *     psrLagTbl[0:255]
   *
   *                     Lag quantization table, in global ROM.
   *
   *     ppsrCGIntFilt[0:5][0:5] ( [tap][phase] )
   *
   *                     Interpolation smoothing filter for generating C(k)
   *                     and G(k) terms, where k is fractional.  Global ROM.
   *
   *     swSP
   *                     speech flag, required for DTX mode
   *
   *   OUTPUTS:
   *
   *     psiUVCode
   *
   *                     (Pointer to) the coded voicing level.
   *
   *     pswLagList[0:?]
   *
   *                     Array of lags to use in the search of the adaptive
   *                     codebook (long-term predictor).  Length determined
   *                     by pswNumLagList[].
   *
   *     pswNumLagList[0:3] ( [0:N_SUB-1] )
   *
   *                     Array of number of lags to use in search of adaptive
   *                     codebook (long-term predictor) for each subframe.
   *
   *     pswPitchBuf[0:3] ( [0:N_SUB-1] )
   *
   *                     Array of estimates of pitch, to be used in harmonic-
   *                     noise-weighting.
   *
   *     pswHNWCoefBuf[0:3] ( [0:N_SUB-1] )
   *
   *                     Array of harmonic-noise-weighting coefficients.
   *
   *     psnsWSfrmEng[-4:3] ( [-N_SUB:N_SUB-1] )
   *
   *                     Array of energies of weighted speech (input speech
   *                     sent through W(z) weighting filter), each stored as
   *                     normalized fraction and shift count.  The zero index
   *                     corresponds to the first subframe of the current
   *                     frame, so there is some history represented.  The
   *                     energies are used for scaling purposes only.
   *
   *     pswVadLags[4]
   *
   *                     An array of int16_ts containing the best open
   *                     loop LTP lags for the four subframes.

   *
   *   DESCRIPTION:
   *
   *     Scaling is done on the input weighted speech, such that the C(k) and
   *     G(k) terms will all be representable.  The amount of scaling is
   *     determined by the maximum energy of any subframe of weighted speech
   *     from the current frame or last frame.  These energies are maintained
   *     in a buffer, and used for scaling when the excitation is determined
   *     later in the analysis.
   *
   *     This function is the main calling program for the open loop lag
   *     search.
   *
   *   REFERENCE:  Sub-clauses 4.1.8.1-4.1.8.4 of GSM Recommendation 06.20
   *
   *   Keywords: openlooplagsearch, openloop, lag, pitch
   *
   **************************************************************************/



  void Codec::openLoopLagSearch(int16_t pswWSpeech[],
                                  int16_t swPrevR0Index,
                                  int16_t swCurrR0Index,
                                  int16_t *psiUVCode,
                                  int16_t pswLagList[],
                                  int16_t pswNumLagList[],
                                  int16_t pswPitchBuf[],
                                  int16_t pswHNWCoefBuf[],
                                  struct NormSw psnsWSfrmEng[],
                                  int16_t pswVadLags[],
                                  int16_t swSP)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_WSfrmEng,
           L_G,
           L_C,
           L_Voicing;
    int16_t swBestPG,
           swCCMax,
           swGMax,
           swCCDivG;
    int16_t swTotalCCDivG,
           swCC,
           swG,
           swRG;
    short  i,
           j,
           k,
           siShift,
           siIndex,
           siTrajIndex,
           siAnchorIndex;
    short  siNumPeaks,
           siNumTrajToDo,
           siPeakIndex,
           siFIndex;
    short  siNumDelta,
           siBIndex,
           siBestTrajIndex = 0;
    short  siLowestSoFar,
           siLagsSoFar,
           si1,
           si2,
           si3;
    struct NormSw snsMax;

    int16_t pswGFrame[G_FRAME_LEN];
    int16_t *ppswGSfrm[N_SUB];
    int16_t pswSfrmEng[N_SUB];
    int16_t pswCFrame[C_FRAME_LEN];
    int16_t *ppswCSfrm[N_SUB];
    int16_t pswLIntBuf[N_SUB];
    int16_t pswCCThresh[N_SUB];
    int16_t pswScaledWSpeechBuffer[W_F_BUFF_LEN];
    int16_t *pswScaledWSpeech;
    int16_t ppswTrajLBuf[N_SUB * NUM_TRAJ_MAX][N_SUB];
    int16_t ppswTrajCCBuf[N_SUB * NUM_TRAJ_MAX][N_SUB];
    int16_t ppswTrajGBuf[N_SUB * NUM_TRAJ_MAX][N_SUB];
    int16_t pswLPeaks[2 * LMAX / LMIN];
    int16_t pswCPeaks[2 * LMAX / LMIN];
    int16_t pswGPeaks[2 * LMAX / LMIN];
    int16_t pswLArray[DELTA_LEVELS];

    pswScaledWSpeech = pswScaledWSpeechBuffer + LSMAX;

  /*_________________________________________________________________________
   |                                                                         |
   |                            Executable Code                              |
   |_________________________________________________________________________|
  */

    /* Scale the weighted speech so that all correlations and energies */
    /* will be less than 1.0 in magnitude.  The scale factor is        */
    /* determined by the maximum energy of any subframe contained in   */
    /* the weighted speech buffer                                      */
    /*-----------------------------------------------------------------*/

    /* Perform one frame of delay on the subframe energy array */
    /*---------------------------------------------------------*/

    for (i = 0; i < N_SUB; i++)
      psnsWSfrmEng[i - N_SUB] = psnsWSfrmEng[i];

    /* Calculate the subframe energies of the current weighted speech frame. */
    /* Overflow protection is done based on the energy in the LPC analysis  */
    /* window (previous or current) which is closest to the subframe.       */
    /*----------------------------------------------------------------------*/

    psnsWSfrmEng[0].sh = g_corr1s(&pswWSpeech[0],
                                  r0BasedEnergyShft(swPrevR0Index),
                                  &L_WSfrmEng);
    psnsWSfrmEng[0].man = round(L_WSfrmEng);

    psnsWSfrmEng[1].sh = g_corr1s(&pswWSpeech[S_LEN],
                                  r0BasedEnergyShft(swCurrR0Index),
                                  &L_WSfrmEng);
    psnsWSfrmEng[1].man = round(L_WSfrmEng);

    psnsWSfrmEng[2].sh = g_corr1s(&pswWSpeech[2 * S_LEN],
                                  r0BasedEnergyShft(swCurrR0Index),
                                  &L_WSfrmEng);
    psnsWSfrmEng[2].man = round(L_WSfrmEng);

    psnsWSfrmEng[3].sh = g_corr1s(&pswWSpeech[3 * S_LEN],
                                  r0BasedEnergyShft(swCurrR0Index),
                                  &L_WSfrmEng);
    psnsWSfrmEng[3].man = round(L_WSfrmEng);

    /* Find the maximum weighted speech subframe energy from all values */
    /* in the array (the array includes the previous frame's subframes, */
    /* and the current frame's subframes)                               */
    /*------------------------------------------------------------------*/

    snsMax.man = 0;
    snsMax.sh = 0;
    for (i = -N_SUB; i < N_SUB; i++)
    {

      if (psnsWSfrmEng[i].man > 0)
      {

        if (snsMax.man == 0)
          snsMax = psnsWSfrmEng[i];

        if (sub(psnsWSfrmEng[i].sh, snsMax.sh) < 0)
          snsMax = psnsWSfrmEng[i];

        if (sub(psnsWSfrmEng[i].sh, snsMax.sh) == 0 &&
            sub(psnsWSfrmEng[i].man, snsMax.man) > 0)
          snsMax = psnsWSfrmEng[i];
      }
    }

    /* Now scale speech up or down, such that the maximum subframe */
    /* energy value will be in range [0.125, 0.25).  This gives a  */
    /* little room for other maxima, and interpolation filtering   */
    /*-------------------------------------------------------------*/

    siShift = sub(shr(snsMax.sh, 1), 1);

    for (i = 0; i < W_F_BUFF_LEN; i++)
      pswScaledWSpeech[i - LSMAX] = shl(pswWSpeech[i - LSMAX], siShift);

    /* Calculate the G(k) (k an integer) terms for all subframes.  (A note */
    /* on the organization of the G buffer: G(k) for a given subframe is   */
    /* the energy in the weighted speech sequence of length S_LEN (40)     */
    /* which begins k back from the beginning of the given subframe-- that */
    /* is, it begins at a lag of k.  These sequences overlap from one      */
    /* subframe to the next, so it is only necessary to compute and store  */
    /* the unique energies.  The unique energies are computed and stored   */
    /* in this buffer, and pointers are assigned for each subframe to make */
    /* array indexing for each subframe easier.                            */
    /* */
    /* (Terms in the G buffer are in order of increasing k, so the energy  */
    /* of the first sequence-- that is, the oldest sequence-- in the       */
    /* weighted speech buffer appears at the end of the G buffer.          */
    /* */
    /* (The subframe pointers are assigned so that they point to the first */
    /* k for their respective subframes, k = LSMIN.)                       */
    /*---------------------------------------------------------------------*/

    L_G = 0;
    for (i = -LSMAX; i < -LSMAX + S_LEN; i++)
      L_G = L_mac(L_G, pswScaledWSpeech[i], pswScaledWSpeech[i]);

    pswGFrame[G_FRAME_LEN - 1] = extract_h(L_G);

    for (i = -LSMAX; i < G_FRAME_LEN - LSMAX - 1; i++)
    {

      L_G = L_msu(L_G, pswScaledWSpeech[i], pswScaledWSpeech[i]);
      L_G = L_mac(L_G, pswScaledWSpeech[i + S_LEN],
                  pswScaledWSpeech[i + S_LEN]);
      pswGFrame[G_FRAME_LEN - LSMAX - 2 - i] = extract_h(L_G);
    }

    ppswGSfrm[0] = pswGFrame + 3 * S_LEN;
    ppswGSfrm[1] = pswGFrame + 2 * S_LEN;
    ppswGSfrm[2] = pswGFrame + S_LEN;
    ppswGSfrm[3] = pswGFrame;

    /* Copy the G(k) terms which also happen to be the subframe energies; */
    /* calculate the 4th subframe energy, which is not a G(k)             */
    /*--------------------------------------------------------------------*/

    pswSfrmEng[0] = pswGFrame[G_FRAME_LEN - 1 - LSMAX];
    pswSfrmEng[1] = pswGFrame[G_FRAME_LEN - 1 - LSMAX - S_LEN];
    pswSfrmEng[2] = pswGFrame[G_FRAME_LEN - 1 - LSMAX - 2 * S_LEN];

    L_WSfrmEng = 0;
    for (i = F_LEN - S_LEN; i < F_LEN; i++)
      L_WSfrmEng = L_mac(L_WSfrmEng, pswScaledWSpeech[i], pswScaledWSpeech[i]);

    pswSfrmEng[3] = extract_h(L_WSfrmEng);

    /* Calculate the C(k) (k an integer) terms for all subframes. */
    /* (The C(k) terms are all unique, so there is no overlapping */
    /* as in the G buffer.)                                       */
    /*------------------------------------------------------------*/

    for (i = 0; i < N_SUB; i++)
    {

      for (j = LSMIN; j <= LSMAX; j++)
      {

        L_C = 0;
        for (k = 0; k < S_LEN; k++)
        {

          L_C = L_mac(L_C, pswScaledWSpeech[i * S_LEN + k],
                      pswScaledWSpeech[i * S_LEN - j + k]);
        }

        pswCFrame[i * CG_TERMS + j - LSMIN] = extract_h(L_C);
      }
    }

    ppswCSfrm[0] = pswCFrame;
    ppswCSfrm[1] = pswCFrame + CG_TERMS;
    ppswCSfrm[2] = pswCFrame + 2 * CG_TERMS;
    ppswCSfrm[3] = pswCFrame + 3 * CG_TERMS;

    /* For each subframe: find the max C(k)*C(k)/G(k) where C(k) > 0 and */
    /* k is integer; save the corresponding k; calculate the             */
    /* threshold against which other peaks in the interpolated CC/G      */
    /* sequence will be checked.  Meanwhile, accumulate max CC/G over    */
    /* the frame for the voiced/unvoiced determination.                  */
    /*-------------------------------------------------------------------*/

    swBestPG = 0;
    for (i = 0; i < N_SUB; i++)
    {

      /* Find max CC/G (C > 0), store corresponding k */
      /*----------------------------------------------*/

      swCCMax = 0;
      swGMax = 0x003f;
      siIndex = fnBest_CG(&ppswCSfrm[i][LMIN - LSMIN],
                          &ppswGSfrm[i][LMIN - LSMIN],
                          &swCCMax, &swGMax,
                          LMAX - LMIN + 1);

      if (siIndex == -1)
      {
        pswLIntBuf[i] = 0;
        pswVadLags[i] = LMIN;            /* store lag value for VAD algorithm */
      }
      else
      {
        pswLIntBuf[i] = add(LMIN, (int16_t) siIndex);
        pswVadLags[i] = pswLIntBuf[i];   /* store lag value for VAD algorithm */
      }

      if (pswLIntBuf[i] > 0)
      {

        /* C > 0 was found: accumulate CC/G, get threshold */
        /*-------------------------------------------------*/

        if (sub(swCCMax, swGMax) < 0)
          swCCDivG = divide_s(swCCMax, swGMax);
        else
          swCCDivG = SW_MAX;

        swBestPG = add(swCCDivG, swBestPG);

        pswCCThresh[i] = getCCThreshold(pswSfrmEng[i], swCCMax, swGMax);
      }
      else
        pswCCThresh[i] = 0;
    }

    /* Make voiced/unvoiced decision */
    /*-------------------------------*/

    L_Voicing = 0;
    for (i = 0; i < N_SUB; i++)
      L_Voicing = L_mac(L_Voicing, pswSfrmEng[i], UV_SCALE0);

    L_Voicing = L_add(L_Voicing, L_deposit_h(swBestPG));

    if ( (L_Voicing <= 0) || (swSP == 0) )
    {

      /* Unvoiced: set return values to zero */
      /*-------------------------------------*/

      for (i = 0; i < N_SUB; i++)
      {

        pswNumLagList[i] = 0;
        pswLagList[i] = 0;
        pswPitchBuf[i] = 0;
        pswHNWCoefBuf[i] = 0;
      }

      *psiUVCode = 0;
    }
    else
    {

      /* Voiced: get best delta-codeable lag trajectory; find pitch and */
      /* harmonic-noise-weighting coefficients for each subframe        */
      /*----------------------------------------------------------------*/

      siTrajIndex = 0;
      swBestPG = SW_MIN;
      for (siAnchorIndex = 0; siAnchorIndex < N_SUB; siAnchorIndex++)
      {

        /* Call pitchLags: for the current subframe, find peaks in the */
        /* C(k)**2/G(k) (k fractional) function which exceed the       */
        /* threshold set by the maximum C(k)**2/G(k) (k integer)       */
        /* (also get the smoothed pitch and harmonic-noise-weighting   */
        /* coefficient).                                               */
        /* */
        /* If there is no C(k) > 0 (k integer), set the smoothed pitch */
        /* to its minimum value and set the harmonic-noise-weighting   */
        /* coefficient to zero.                                        */
        /*-------------------------------------------------------------*/

        if (pswLIntBuf[siAnchorIndex] != 0)
        {

          pitchLags(pswLIntBuf[siAnchorIndex],
                    ppswCSfrm[siAnchorIndex],
                    ppswGSfrm[siAnchorIndex],
                    pswCCThresh[siAnchorIndex],
                    pswLPeaks,
                    pswCPeaks,
                    pswGPeaks,
                    &siNumPeaks,
                    &pswPitchBuf[siAnchorIndex],
                    &pswHNWCoefBuf[siAnchorIndex]);

          siPeakIndex = 0;
        }
        else
        {

          /* No C(k) > 0 (k integer): set pitch to min, coef to zero, */
          /* go to next subframe.                                     */
          /*----------------------------------------------------------*/

          pswPitchBuf[siAnchorIndex] = LMIN_FR;
          pswHNWCoefBuf[siAnchorIndex] = 0;
          continue;
        }

        /* It is possible that by interpolating, the only positive */
        /* C(k) was made negative.  Check for this here            */
        /*---------------------------------------------------------*/

        if (siNumPeaks == 0)
        {

          pswPitchBuf[siAnchorIndex] = LMIN_FR;
          pswHNWCoefBuf[siAnchorIndex] = 0;
          continue;
        }

        /* Peak(s) found in C**2/G function: find the best delta-codeable */
        /* trajectory through each peak (unless that peak has already     */
        /* paritcipated in a trajectory) up to a total of NUM_TRAJ_MAX    */
        /* peaks.  After each trajectory is found, check to see if that   */
        /* trajectory is the best one so far.                             */
        /*----------------------------------------------------------------*/

        if (siNumPeaks > NUM_TRAJ_MAX)
          siNumTrajToDo = NUM_TRAJ_MAX;
        else
          siNumTrajToDo = siNumPeaks;

        while (siNumTrajToDo)
        {

          /* Check if this peak has already participated in a trajectory. */
          /* If so, skip it, decrement the number of trajectories yet to  */
          /* be evaluated, and go on to the next best peak                */
          /*--------------------------------------------------------------*/

          si1 = 0;
          for (i = 0; i < siTrajIndex; i++)
          {

            if (sub(pswLPeaks[siPeakIndex],
                    ppswTrajLBuf[i][siAnchorIndex]) == 0)
              si1 = 1;
          }

          if (si1)
          {

            siNumTrajToDo--;
            if (siNumTrajToDo)
            {

              siPeakIndex++;
              continue;
            }
            else
              break;
          }

          /* The current peak is not in a previous trajectory: find */
          /* the best trajectory through it.                        */
          /* */
          /* First, store the lag, C**2, and G for the peak in the  */
          /* trajectory storage buffer                              */
          /*--------------------------------------------------------*/

          ppswTrajLBuf[siTrajIndex][siAnchorIndex] = pswLPeaks[siPeakIndex];
          ppswTrajGBuf[siTrajIndex][siAnchorIndex] = pswGPeaks[siPeakIndex];
          ppswTrajCCBuf[siTrajIndex][siAnchorIndex] =
                  mult_r(pswCPeaks[siPeakIndex], pswCPeaks[siPeakIndex]);

          /* Complete the part of the trajectory that extends forward */
          /* from the anchor subframe                                 */
          /*----------------------------------------------------------*/

          for (siFIndex = siAnchorIndex + 1; siFIndex < N_SUB; siFIndex++)
          {

            /* Get array of lags which are delta-codeable */
            /* */
            /* First, get code for largest lag in array   */
            /* (limit it)                                 */
            /*--------------------------------------------*/

            quantLag(ppswTrajLBuf[siTrajIndex][siFIndex - 1],
                     &si1);
            si2 = add(si1, (DELTA_LEVELS / 2 - 1) - (NUM_CLOSED - 1));
            if (sub(si2, (1 << L_BITS) - 1) > 0)
              si2 = (1 << L_BITS) - 1;

            /* Get code for smallest lag in array (limit it) */
            /*-----------------------------------------------*/

            si3 = sub(si1, (DELTA_LEVELS / 2) - (NUM_CLOSED - 1));
            if (si3 < 0)
              si3 = 0;

            /* Generate array of lags */
            /*------------------------*/

            for (i = si3, j = 0; i <= si2; i++, j++)
              pswLArray[j] = psrLagTbl[i];

            siNumDelta = add(sub(si2, si3), 1);

            /* Search array of delta-codeable lags for one which maximizes */
            /* C**2/G                                                      */
            /*-------------------------------------------------------------*/

            bestDelta(pswLArray, ppswCSfrm[siFIndex], ppswGSfrm[siFIndex],
                      siNumDelta, siFIndex,
                      ppswTrajLBuf[siTrajIndex], ppswTrajCCBuf[siTrajIndex],
                      ppswTrajGBuf[siTrajIndex]);
          }

          /* Complete the part of the trajectory that extends backward */
          /* from the anchor subframe                                  */
          /*-----------------------------------------------------------*/

          for (siBIndex = siAnchorIndex - 1; siBIndex >= 0; siBIndex--)
          {

            /* Get array of lags which are delta-codeable */
            /* */
            /* First, get code for largest lag in array   */
            /* (limit it)                                 */
            /*--------------------------------------------*/

            quantLag(ppswTrajLBuf[siTrajIndex][siBIndex + 1],
                     &si1);
            si2 = add(si1, (DELTA_LEVELS / 2) - (NUM_CLOSED - 1));
            if (sub(si2, (1 << L_BITS) - 1) > 0)
              si2 = (1 << L_BITS) - 1;

            /* Get code for smallest lag in array (limit it) */
            /*-----------------------------------------------*/

            si3 = sub(si1, (DELTA_LEVELS / 2 - 1) - (NUM_CLOSED - 1));
            if (si3 < 0)
              si3 = 0;

            /* Generate array of lags */
            /*------------------------*/

            for (i = si3, j = 0; i <= si2; i++, j++)
              pswLArray[j] = psrLagTbl[i];

            siNumDelta = add(sub(si2, si3), 1);

            /* Search array of delta-codeable lags for one which maximizes */
            /* C**2/G                                                      */
            /*-------------------------------------------------------------*/

            bestDelta(pswLArray, ppswCSfrm[siBIndex], ppswGSfrm[siBIndex],
                      siNumDelta, siBIndex,
                      ppswTrajLBuf[siTrajIndex], ppswTrajCCBuf[siTrajIndex],
                      ppswTrajGBuf[siTrajIndex]);
          }

          /* This trajectory done: check total C**2/G for this trajectory */
          /* against current best trajectory                              */
          /* */
          /* Get total C**2/G for this trajectory                         */
          /*--------------------------------------------------------------*/

          swTotalCCDivG = 0;
          for (i = 0; i < N_SUB; i++)
          {

            swCC = ppswTrajCCBuf[siTrajIndex][i];
            swG = ppswTrajGBuf[siTrajIndex][i];

            if (swG <= 0)
            {

              /* Negative G (imperfect interpolation): do not include in */
              /* total                                                   */
              /*---------------------------------------------------------*/

              swCCDivG = 0;
            }
            else if (sub(abs_s(swCC), swG) > 0)
            {

              /* C**2/G > 0: limit quotient, add to total */
              /*------------------------------------------*/

              if (swCC > 0)
                swCCDivG = SW_MAX;
              else
                swCCDivG = SW_MIN;

              swTotalCCDivG = add(swTotalCCDivG, swCCDivG);
            }
            else
            {

              /* accumulate C**2/G */
              /*-------------------*/

              if (swCC < 0)
              {

                swCCDivG = divide_s(negate(swCC), swG);
                swTotalCCDivG = sub(swTotalCCDivG, swCCDivG);
              }
              else
              {

                swCCDivG = divide_s(swCC, swG);
                swTotalCCDivG = add(swTotalCCDivG, swCCDivG);
              }
            }
          }

          /* Compare this trajectory with current best, update if better */
          /*-------------------------------------------------------------*/

          if (sub(swTotalCCDivG, swBestPG) > 0)
          {

            swBestPG = swTotalCCDivG;
            siBestTrajIndex = siTrajIndex;
          }

          /* Update trajectory index, peak index, decrement the number */
          /* of trajectories left to do.                               */
          /*-----------------------------------------------------------*/

          siTrajIndex++;
          siPeakIndex++;
          siNumTrajToDo--;
        }
      }

      if (siTrajIndex == 0)
      {

        /* No trajectories searched despite voiced designation: change */
        /* designation to unvoiced.                                    */
        /*-------------------------------------------------------------*/

        for (i = 0; i < N_SUB; i++)
        {

          pswNumLagList[i] = 0;
          pswLagList[i] = 0;
          pswPitchBuf[i] = 0;
          pswHNWCoefBuf[i] = 0;
        }

        *psiUVCode = 0;
      }
      else
      {

        /* Best trajectory determined: get voicing level, generate the */
        /* constrained list of lags to search in the adaptive codebook */
        /* for each subframe                                           */
        /* */
        /* First, get voicing level                                    */
        /*-------------------------------------------------------------*/

        *psiUVCode = 3;
        siLowestSoFar = 2;
        for (i = 0; i < N_SUB; i++)
        {

          /* Check this subframe against highest voicing threshold */
          /*-------------------------------------------------------*/

          swCC = ppswTrajCCBuf[siBestTrajIndex][i];
          swG = ppswTrajGBuf[siBestTrajIndex][i];

          swRG = mult_r(swG, pswSfrmEng[i]);
          L_Voicing = L_deposit_h(swCC);
          L_Voicing = L_mac(L_Voicing, swRG, UV_SCALE2);

          if (L_Voicing < 0)
          {

            /* Voicing for this subframe failed to meet 2/3 threshold:  */
            /* therefore, voicing level for entire frame can only be as */
            /* high as 2                                                */
            /*----------------------------------------------------------*/

            *psiUVCode = siLowestSoFar;

            L_Voicing = L_deposit_h(swCC);
            L_Voicing = L_mac(L_Voicing, swRG, UV_SCALE1);

            if (L_Voicing < 0)
            {

              /* Voicing for this subframe failed to meet 1/2 threshold: */
              /* therefore, voicing level for entire frame can only be   */
              /* as high as 1                                            */
              /*---------------------------------------------------------*/

              *psiUVCode = siLowestSoFar = 1;
            }
          }
        }

        /* Generate list of lags to be searched in closed-loop */
        /*-----------------------------------------------------*/

        siLagsSoFar = 0;
        for (i = 0; i < N_SUB; i++)
        {

          quantLag(ppswTrajLBuf[siBestTrajIndex][i], &si1);

          si2 = add(si1, NUM_CLOSED / 2);
          if (sub(si2, (1 << L_BITS) - 1) > 0)
            si2 = (1 << L_BITS) - 1;

          si3 = sub(si1, NUM_CLOSED / 2);
          if (si3 < 0)
            si3 = 0;

          pswNumLagList[i] = add(sub(si2, si3), 1);

          for (j = siLagsSoFar; j < siLagsSoFar + pswNumLagList[i]; j++)
            pswLagList[j] = psrLagTbl[si3++];

          siLagsSoFar += pswNumLagList[i];
        }
      }
    }
  }                                      /* end of openLoopLagSearch */

  /***************************************************************************
   *
   *   FUNCTION NAME: pitchLags
   *
   *   PURPOSE:
   *
   *     Locates peaks in the interpolated C(k)*C(k)/G(k) sequence for a
   *     subframe which exceed a given threshold. Also determines the
   *     fundamental pitch, and a harmonic-noise-weighting coefficient.
   *
   *   INPUTS:
   *
   *     swBestIntLag
   *
   *                     The integer lag for which CC/G is maximum.
   *
   *     pswIntCs[0:127]
   *
   *                     The C(k) sequence for the subframe, with k an integer.
   *
   *     pswIntGs[0:127]
   *
   *                     The G(k) sequence for the subframe, with k an integer.
   *
   *     swCCThreshold
   *
   *                     The CC/G threshold which peaks must exceed (G is
   *                     understood to be 0.5).
   *
   *     psrLagTbl[0:255]
   *
   *                     The lag quantization table.
   *
   *
   *   OUTPUTS:
   *
   *     pswLPeaksSorted[0:10(max)]
   *
   *                     List of fractional lags where CC/G peaks, highest
   *                     peak first.
   *
   *     pswCPeaksSorted[0:10(max)]
   *
   *                     List of C's where CC/G peaks.
   *
   *     pswGPeaksSorted[0:10(max)]
   *
   *                     List of G's where CC/G peaks.
   *
   *     psiNumSorted
   *
   *                     Number of peaks found.
   *
   *     pswPitch
   *
   *                     The fundamental pitch for current subframe.
   *
   *     pswHNWCoef
   *
   *                     The harmonic-noise-weighting coefficient for the
   *                     current subframe.
   *
   *   RETURN VALUE:
   *
   *     None
   *
   *   DESCRIPTION:
   *
   *
   *   REFERENCE:  Sub-clauses 4.1.9, 4.1.8.2 of GSM Recommendation 06.20
   *
   *   KEYWORDS: pitchLags, pitchlags, PITCH_LAGS
   *
   *************************************************************************/

  void Codec::pitchLags(int16_t swBestIntLag,
                          int16_t pswIntCs[],
                          int16_t pswIntGs[],
                          int16_t swCCThreshold,
                          int16_t pswLPeaksSorted[],
                          int16_t pswCPeaksSorted[],
                          int16_t pswGPeaksSorted[],
                          int16_t *psiNumSorted,
                          int16_t *pswPitch,
                          int16_t *pswHNWCoef)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t pswLBuf[2 * OS_FCTR - 1],
           pswCBuf[2 * OS_FCTR - 1];
    int16_t pswGBuf[2 * OS_FCTR - 1],
           pswLPeaks[2 * LMAX / LMIN];
    int16_t pswCPeaks[2 * LMAX / LMIN],
           pswGPeaks[2 * LMAX / LMIN];
    short  siLPeakIndex,
           siCPeakIndex,
           siGPeakIndex,
           siPeakIndex;
    short  siSortedIndex,
           siLPeaksSortedInit,
           swWorkingLag;
    int16_t swSubMult,
           swFullResPeak,
           swCPitch,
           swGPitch,
           swMult;
    int16_t swMultInt,
           sw1,
           sw2,
           si1,
           si2;
    int32_t L_1;
    short  siNum,
           siUpperBound,
           siLowerBound,
           siSMFIndex;
    short  siNumPeaks,
           siRepeat,
           i,
           j;

    static int16_tRom psrSubMultFactor[] = {0x0aab,     /* 1.0/12.0 */
      0x071c,                            /* 1.0/18.0 */
      0x0555,                            /* 1.0/24.0 */
      0x0444,                            /* 1.0/30.0 */
    0x038e};                             /* 1.0/36.0 */


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Get array of valid lags within one integer lag of the best open-loop */
    /* integer lag; get the corresponding interpolated C and G arrays;      */
    /* find the best among these; store the info corresponding to this peak */
    /* in the interpolated CC/G sequence                                    */
    /*----------------------------------------------------------------------*/

    sw1 = shr(extract_l(L_mult(swBestIntLag, OS_FCTR)), 1);

    siNum = CGInterpValid(sw1, pswIntCs, pswIntGs,
                          pswLBuf, pswCBuf, pswGBuf);

    sw1 = 0;
    sw2 = 0x003f;
    siPeakIndex = fnBest_CG(pswCBuf, pswGBuf, &sw1, &sw2, siNum);
    if (siPeakIndex == -1)
    {

      /* It is possible, although rare, that the interpolated sequence */
      /* will not have a peak where the original sequence did.         */
      /* Indicate this on return                                       */
      /*---------------------------------------------------------------*/

      *psiNumSorted = 0;
      return;
    }

    siLPeakIndex = 0;
    siCPeakIndex = 0;
    siGPeakIndex = 0;
    siSortedIndex = 0;
    pswCPeaks[siCPeakIndex] = pswCBuf[siPeakIndex];
    siCPeakIndex = add(siCPeakIndex, 1);
    pswLPeaks[siLPeakIndex] = pswLBuf[siPeakIndex];
    siLPeakIndex = add(siLPeakIndex, 1);
    pswGPeaks[siGPeakIndex] = pswGBuf[siPeakIndex];
    siGPeakIndex = add(siGPeakIndex, 1);

    /* Find all peaks at submultiples of the first peak */
    /*--------------------------------------------------*/

    siSMFIndex = 0;
    swSubMult = mult_r(pswLPeaks[0], psrSubMultFactor[siSMFIndex++]);

    while (sub(swSubMult, LMIN) >= 0 && siSMFIndex <= 5)
    {

      /* Check if there is peak in the integer CC/G sequence within */
      /* PEAK_VICINITY of the submultiple                           */
      /*------------------------------------------------------------*/

      swFullResPeak = findPeak(swSubMult, pswIntCs, pswIntGs);

      if (swFullResPeak)
      {

        /* Peak found at submultiple: interpolate to get C's and G's */
        /* corresponding to valid lags within one of the new found   */
        /* peak; get best C**2/G from these;  if it meets threshold, */
        /* store the info corresponding to this peak                 */
        /*-----------------------------------------------------------*/

        siNum = CGInterpValid(swFullResPeak, pswIntCs, pswIntGs,
                              pswLBuf, pswCBuf, pswGBuf);

        sw1 = swCCThreshold;
        sw2 = 0x4000;
        siPeakIndex = fnBest_CG(pswCBuf, pswGBuf, &sw1, &sw2, siNum);
        if (siPeakIndex != -1)
        {

          pswCPeaks[siCPeakIndex] = pswCBuf[siPeakIndex];
          siCPeakIndex = add(siCPeakIndex, 1);
          pswLPeaks[siLPeakIndex] = pswLBuf[siPeakIndex];
          siLPeakIndex = add(siLPeakIndex, 1);
          pswGPeaks[siGPeakIndex] = pswGBuf[siPeakIndex];
          siGPeakIndex = add(siGPeakIndex, 1);

        }
      }


      if (siSMFIndex < 5)
      {

        /* Get next submultiple */
        /*----------------------*/
        swSubMult = mult_r(pswLPeaks[0], psrSubMultFactor[siSMFIndex]);

      }

      siSMFIndex++;
    }

    /* Get pitch from fundamental peak: first, build array of fractional */
    /* lags around which to search for peak.  Note that these lags are   */
    /* NOT restricted to those in the lag table, but may take any value  */
    /* in the range LMIN_FR to LMAX_FR                                   */
    /*-------------------------------------------------------------------*/

    siUpperBound = add(pswLPeaks[siLPeakIndex - 1], OS_FCTR);
    siUpperBound = sub(siUpperBound, 1);
    if (sub(siUpperBound, LMAX_FR) > 0)
      siUpperBound = LMAX_FR;

    siLowerBound = sub(pswLPeaks[siLPeakIndex - 1], OS_FCTR);
    siLowerBound = add(siLowerBound, 1);
    if (sub(siLowerBound, LMIN_FR) < 0)
      siLowerBound = LMIN_FR;
    for (i = siLowerBound, j = 0; i <= siUpperBound; i++, j++)
      pswLBuf[j] = i;

    /* Second, find peak in the interpolated CC/G sequence. */
    /* The corresponding lag is the fundamental pitch.  The */
    /* interpolated C(pitch) and G(pitch) values are stored */
    /* for later use in calculating the Harmonic-Noise-     */
    /* Weighting coefficient                                */
    /*------------------------------------------------------*/

    siNum = sub(siUpperBound, siLowerBound);
    siNum = add(siNum, 1);
    CGInterp(pswLBuf, siNum, pswIntCs, pswIntGs, LSMIN,
             pswCBuf, pswGBuf);
    sw1 = 0;
    sw2 = 0x003f;
    siPeakIndex = fnBest_CG(pswCBuf, pswGBuf, &sw1, &sw2, siNum);
    if (siPeakIndex == -1)
    {
      swCPitch = 0;
      *pswPitch = LMIN_FR;
      swGPitch = 0x003f;
    }
    else
    {
      swCPitch = pswCBuf[siPeakIndex];
      *pswPitch = pswLBuf[siPeakIndex];
      swGPitch = pswGBuf[siPeakIndex];
    }

    /* Find all peaks which are multiples of fundamental pitch */
    /*---------------------------------------------------------*/

    swMult = add(*pswPitch, *pswPitch);
    swMultInt = mult_r(swMult, INV_OS_FCTR);

    while (sub(swMultInt, LMAX) <= 0)
    {

      /* Check if there is peak in the integer CC/G sequence within */
      /* PEAK_VICINITY of the multiple                              */
      /*------------------------------------------------------------*/

      swFullResPeak = findPeak(swMultInt, pswIntCs, pswIntGs);

      if (swFullResPeak)
      {

        /* Peak found at multiple: interpolate to get C's and G's    */
        /* corresponding to valid lags within one of the new found   */
        /* peak; get best C**2/G from these;  if it meets threshold, */
        /* store the info corresponding to this peak                 */
        /*-----------------------------------------------------------*/

        siNum = CGInterpValid(swFullResPeak, pswIntCs, pswIntGs,
                              pswLBuf, pswCBuf, pswGBuf);

        sw1 = swCCThreshold;
        sw2 = 0x4000;
        siPeakIndex = fnBest_CG(pswCBuf, pswGBuf, &sw1, &sw2, siNum);
        if (siPeakIndex != -1)
        {

          pswCPeaks[siCPeakIndex] = pswCBuf[siPeakIndex];
          siCPeakIndex = add(siCPeakIndex, 1);
          pswLPeaks[siLPeakIndex] = pswLBuf[siPeakIndex];
          siLPeakIndex = add(siLPeakIndex, 1);
          pswGPeaks[siGPeakIndex] = pswGBuf[siPeakIndex];
          siGPeakIndex = add(siGPeakIndex, 1);

        }
      }

      /* Get next multiple */
      /*-------------------*/

      swMult = add(*pswPitch, swMult);
      swMultInt = mult_r(swMult, INV_OS_FCTR);
    }

    /* Get Harmonic-Noise-Weighting coefficient = 0.4 * C(pitch) / G(pitch) */
    /* Note: a factor of 0.5 is applied the the HNW coeffcient              */
    /*----------------------------------------------------------------------*/

    si2 = norm_s(swCPitch);
    sw1 = shl(swCPitch, si2);
    L_1 = L_mult(sw1, PW_FRAC);

    si1 = norm_s(swGPitch);
    sw1 = shl(swGPitch, si1);

    sw2 = round(L_shr(L_1, 1));
    sw2 = divide_s(sw2, sw1);

    if (sub(si1, si2) > 0)
      sw2 = shl(sw2, sub(si1, si2));

    if (sub(si1, si2) < 0)
      sw2 = shift_r(sw2, sub(si1, si2));

    *pswHNWCoef = sw2;

    /* Sort peaks into output arrays, largest first */
    /*----------------------------------------------*/

    siLPeaksSortedInit = siSortedIndex;
    *psiNumSorted = 0;
    siNumPeaks = siLPeakIndex;
    for (i = 0; i < siNumPeaks; i++)
    {

      sw1 = 0;
      sw2 = 0x003f;
      siPeakIndex = fnBest_CG(pswCPeaks, pswGPeaks, &sw1, &sw2, siNumPeaks);

      siRepeat = 0;
      swWorkingLag = pswLPeaks[siPeakIndex];
      for (j = 0; j < *psiNumSorted; j++)
      {

        if (sub(swWorkingLag, pswLPeaksSorted[j + siLPeaksSortedInit]) == 0)
          siRepeat = 1;
      }

      if (!siRepeat)
      {

        pswLPeaksSorted[siSortedIndex] = swWorkingLag;
        pswCPeaksSorted[siSortedIndex] = pswCPeaks[siPeakIndex];
        pswGPeaksSorted[siSortedIndex] = pswGPeaks[siPeakIndex];
        siSortedIndex = add(siSortedIndex, 1);
        *psiNumSorted = add(*psiNumSorted, 1);
      }

      pswCPeaks[siPeakIndex] = 0x0;
    }
  }                                      /* end of pitchLags */



  /***************************************************************************
   *
   *   FUNCTION NAME: quantLag
   *
   *   PURPOSE:
   *
   *     Quantizes a given fractional lag according to the provided table
   *     of allowable fractional lags.
   *
   *   INPUTS:
   *
   *     swRawLag
   *
   *                     Raw lag value: a fractional lag value*OS_FCTR.
   *
   *     psrLagTbl[0:255]
   *
   *                     Fractional lag table.
   *
   *   OUTPUTS:
   *
   *     pswCode
   *
   *                     Index in lag table of the quantized lag-- that is,
   *                     the coded value of the lag.
   *
   *   RETURN VALUE:
   *
   *     Quantized lag value.
   *
   *
   *   REFERENCE:  Sub-clause 4.1.8.3 of GSM Recommendation 06.20
   *
   *   KEYWORDS: quantization, LTP, adaptive codebook
   *
   *************************************************************************/

  int16_t Codec::quantLag(int16_t swRawLag,
                            int16_t *pswCode)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */
    int16_t siOffset,
           swIndex1,
           swIndex2;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    swIndex1 = 0;
    siOffset = shr(LAG_TABLE_LEN, 1);
    swIndex2 = siOffset;
    siOffset = shr(siOffset, 1);

    while (sub(swIndex2, swIndex1) != 0)
    {
      if (sub(swRawLag, psrLagTbl[swIndex2]) >= 0)
        swIndex1 = swIndex2;
      swIndex2 = add(swIndex1, siOffset);
      siOffset = shr(siOffset, 1);
    }
    *pswCode = swIndex2;

    return (psrLagTbl[swIndex2]);

  }                                      /* end of quantLag */

  /***************************************************************************
   *
   *   FUNCTION NAME: r0Quant
   *
   *   PURPOSE:
   *
   *      Quantize the unquantized R(0).  Returns r0 codeword (index).
   *
   *   INPUTS:
   *
   *     L_UnqntzdR0
   *                     The average frame energy R(0)
   *
   *   OUTPUTS: none
   *
   *   RETURN VALUE:
   *
   *                     A 16 bit number representing the codeword whose
   *                     associated R(0) is closest to the input frame energy.
   *
   *   DESCRIPTION:
   *
   *     Returns r0 codeword (index) not the actual Rq(0).
   *
   *     Level compare input with boundary value (the boundary
   *     above ,louder) of candidate r0Index i.e.
   *     lowerBnd[i] <= inputR(0) < upperBnd[i+1]
   *
   *     The compare in the routine is:
   *     inputR(0) < upperBnd[i+1] if false return i as codeword
   *
   *   REFERENCE:  Sub-clause 4.1.3 of GSM Recommendation 06.20
   *
   *
   *************************************************************************/

  int16_t Codec::r0Quant(int32_t L_UnqntzdR0)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t swR0Index,
           swUnqntzdR0;

  /*_________________________________________________________________________
   |                                                                         |
   |                            Executable Code                              |
   |_________________________________________________________________________|
  */

    swUnqntzdR0 = round(L_UnqntzdR0);

    for (swR0Index = 0; swR0Index < (1 << R0BITS) - 1; swR0Index++)
    {
      if (sub(swUnqntzdR0, psrR0DecTbl[2 * swR0Index + 1]) < 0)
      {
        return (swR0Index);
      }
    }
    return ((1 << R0BITS) - 1);          /* return the maximum */
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: setupPreQ
   *
   *   PURPOSE:
   *     The purpose of this function is to setup the internal pointers so that
   *     getNextVec knows where to start.
   *
   *   INPUTS: iSeg, iVector
   *
   *   OUTPUTS: None
   *
   *   RETURN VALUE: none
   *
   *   DESCRIPTION:
   *
   *   REFERENCE:  Sub-clause  4.1.4.1 of GSM Recommendation 06.20
   *
   *   KEYWORDS:  vector quantizer preQ
   *
   *************************************************************************/

  void Codec::setupPreQ(int iSeg, int iVector)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    iLow = psvqIndex[iSeg - 1].l;
    iThree = ((psvqIndex[iSeg - 1].h - iLow) == 2);

    switch (iSeg)
    {
      case 1:
        {
          psrTable = psrPreQ1;
          iWordPtr = (iVector * 3) >> 1;
          if (odd(iVector))
            iWordHalfPtr = LOW;
          else
            iWordHalfPtr = HIGH;
          break;
        }

      case 2:
        {
          psrTable = psrPreQ2;
          iWordPtr = (iVector * 3) >> 1;
          if (odd(iVector))
            iWordHalfPtr = LOW;
          else
            iWordHalfPtr = HIGH;
          break;
        }

      case 3:
        {
          psrTable = psrPreQ3;
          iWordPtr = iVector * 2;
          iWordHalfPtr = HIGH;
          break;
        }
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: setupQuant
   *
   *   PURPOSE:
   *     The purpose of this function is to setup the internal pointers so that
   *     getNextVec knows where to start.
   *
   *
   *   INPUTS: iSeg, iVector
   *
   *   OUTPUTS: None
   *
   *   RETURN VALUE: none
   *
   *   DESCRIPTION:
   *
   *   REFERENCE:  Sub-clause 4.1.4.1 of GSM Recommendation 06.20
   *
   *   KEYWORDS:  vector quantizer Quant
   *
   *************************************************************************/

  void Codec::setupQuant(int iSeg, int iVector)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    iLow = psvqIndex[iSeg - 1].l;
    iThree = ((psvqIndex[iSeg - 1].h - iLow) == 2);

    switch (iSeg)
    {
      case 1:
        {
          psrTable = psrQuant1;
          iWordPtr = (iVector * 3) >> 1;
          if (odd(iVector))
            iWordHalfPtr = LOW;
          else
            iWordHalfPtr = HIGH;
          break;
        }

      case 2:
        {
          psrTable = psrQuant2;
          iWordPtr = (iVector * 3) >> 1;
          if (odd(iVector))
            iWordHalfPtr = LOW;
          else
            iWordHalfPtr = HIGH;
          break;
        }

      case 3:
        {
          psrTable = psrQuant3;
          iWordPtr = iVector * 2;
          iWordHalfPtr = HIGH;
          break;
        }
    }
  }

  /***************************************************************************
   *
   *   FUNCTION NAME: weightSpeechFrame
   *
   *   PURPOSE:
   *
   *     The purpose of this function is to perform the spectral
   *     weighting filter  (W(z)) of the input speech frame.
   *
   *   INPUTS:
   *
   *     pswSpeechFrm[0:F_LEN]
   *
   *                     high pass filtered input speech frame
   *
   *     pswWNumSpace[0:NP*N_SUB]
   *
   *                     W(z) numerator coefficients
   *
   *     pswWDenomSpace[0:NP*N_SUB]
   *
   *                     W(z) denominator coefficients
   *
   *     pswWSpeechBuffBase[0:F_LEN+LMAX+CG_INT_MACS/2]
   *
   *                     previous W(z) filtered speech
   *
   *   OUTPUTS:
   *
   *     pswWSpeechBuffBase[0:F_LEN+LMAX+CG_INT_MACS/2]
   *
   *                     W(z) filtered speech frame
   *
   *   RETURN VALUE:
   *
   *     none
   *
   *   DESCRIPTION:
   *
   *   REFERENCE:  Sub-clause 4.1.7 of GSM Recommendation 06.20
   *
   *   KEYWORDS:
   *
   *************************************************************************/

  void Codec::weightSpeechFrame(int16_t pswSpeechFrm[],
                                  int16_t pswWNumSpace[],
                                  int16_t pswWDenomSpace[],
                                  int16_t pswWSpeechBuffBase[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t pswScratch0[W_F_BUFF_LEN],
          *pswWSpeechFrm;
    short int siI;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /* Delay the weighted speech buffer by one frame */
    /* --------------------------------------------- */

    for (siI = 0; siI < LSMAX; siI++)
    {
      pswWSpeechBuffBase[siI] = pswWSpeechBuffBase[siI + F_LEN];
    }

    /* pass speech frame through W(z) */
    /* ------------------------------ */

    pswWSpeechFrm = pswWSpeechBuffBase + LSMAX;

    for (siI = 0; siI < N_SUB; siI++)
    {
      lpcFir(&pswSpeechFrm[siI * S_LEN], &pswWNumSpace[siI * NP],
             pswWStateNum, &pswScratch0[siI * S_LEN]);
    }

    for (siI = 0; siI < N_SUB; siI++)
    {
      lpcIir(&pswScratch0[siI * S_LEN], &pswWDenomSpace[siI * NP],
             pswWStateDenom, &pswWSpeechFrm[siI * S_LEN]);
    }
  }

  // vad.c

  void Codec::vad_reset(void)

  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int    i;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    pswRvad[0] = 24576;
    swNormRvad = 7;
    swPt_sacf = 0;
    swPt_sav0 = 0;
    L_lastdm = 0;
    swE_thvad = 21;
    swM_thvad = 21875;
    swAdaptCount = 0;
    swBurstCount = 0;
    swHangCount = -1;
    swOldLagCount = 0;
    swVeryOldLagCount = 0;
    swOldLag = 21;

    for (i = 1; i < 9; i++)
      pswRvad[i] = 0;
    for (i = 0; i < 27; i++)
      pL_sacf[i] = 0;
    for (i = 0; i < 36; i++)
      pL_sav0[i] = 0;

  }

  /****************************************************************************
   *
   *     FUNCTION:  vad_algorithm
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Returns a decision as to whether the current frame being
   *                processed by the speech encoder contains speech or not.
   *
   *     INPUTS:    pL_acf[0..8]  autocorrelation of input signal frame
   *                swScaleAcf    L_acf scaling factor
   *                pswRc[0..3]   speech encoder reflection coefficients
   *                swPtch        flag to indicate a periodic signal component
   *
   *     OUTPUTS:   pswVadFlag    vad decision
   *
   ***************************************************************************/

  void   Codec::vad_algorithm(int32_t pL_acf[9],
                              int16_t swScaleAcf,
                              int16_t pswRc[4],
                              int16_t swPtch,
                              int16_t *pswVadFlag)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t
           pL_av0[9],
           pL_av1[9];

    int16_t
           swM_acf0,
           swE_acf0,
           pswRav1[9],
           swNormRav1,
           swM_pvad,
           swE_pvad,
           swStat,
           swTone,
           swVvad;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    energy_computation
            (
             pL_acf, swScaleAcf,
             pswRvad, swNormRvad,
             &swM_pvad, &swE_pvad,
             &swM_acf0, &swE_acf0
            );

    average_acf
            (
             pL_acf, swScaleAcf,
             pL_av0, pL_av1
            );

    predictor_values
            (
             pL_av1,
             pswRav1,
             &swNormRav1
            );

    spectral_comparison
            (
             pswRav1, swNormRav1,
             pL_av0,
             &swStat
            );

    tone_detection
            (
             pswRc,
             &swTone
            );

    threshold_adaptation
            (
             swStat, swPtch, swTone,
             pswRav1, swNormRav1,
             swM_pvad, swE_pvad,
             swM_acf0, swE_acf0,
             pswRvad, &swNormRvad,
             &swM_thvad, &swE_thvad
            );

    vad_decision
            (
             swM_pvad, swE_pvad,
             swM_thvad, swE_thvad,
             &swVvad
            );

    vad_hangover
            (
             swVvad,
             pswVadFlag
            );

  }

  /****************************************************************************
   *
   *     FUNCTION:  energy_computation
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Computes the input and residual energies of the adaptive
   *                filter in a floating point representation.
   *
   *     INPUTS:    pL_acf[0..8]   autocorrelation of input signal frame
   *                swScaleAcf     L_acf scaling factor
   *                pswRvad[0..8]  autocorrelated adaptive filter coefficients
   *                swNormRvad     rvad scaling factor
   *
   *     OUTPUTS:   pswM_pvad      mantissa of filtered signal energy
   *                pswE_pvad      exponent of filtered signal energy
   *                pswM_acf0      mantissa of signal frame energy
   *                pswE_acf0      exponent of signal frame energy
   *
   ***************************************************************************/

  void   Codec::energy_computation(int32_t pL_acf[],
                                   int16_t swScaleAcf,
                                   int16_t pswRvad[],
                                   int16_t swNormRvad,
                                   int16_t *pswM_pvad,
                                   int16_t *pswE_pvad,
                                   int16_t *pswM_acf0,
                                   int16_t *pswE_acf0)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t
           L_temp;

    int16_t
           pswSacf[9],
           swNormAcf,
           swNormProd,
           swShift;

    int
           i;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /*** Test if acf[0] is zero ***/

    if (pL_acf[0] == 0)
    {
      *pswE_pvad = -0x8000;
      *pswM_pvad = 0;
      *pswE_acf0 = -0x8000;
      *pswM_acf0 = 0;
      return;
    }


    /*** Re-normalisation of L_acf[0..8] ***/

    swNormAcf = norm_l(pL_acf[0]);
    swShift = sub(swNormAcf, 3);

    for (i = 0; i <= 8; i++)
      pswSacf[i] = extract_h(L_shl(pL_acf[i], swShift));


    /*** Computation of e_acf0 and m_acf0 ***/

    *pswE_acf0 = add(32, shl(swScaleAcf, 1));
    *pswE_acf0 = sub(*pswE_acf0, swNormAcf);
    *pswM_acf0 = shl(pswSacf[0], 3);


    /*** Computation of e_pvad and m_pvad ***/

    *pswE_pvad = add(*pswE_acf0, 14);
    *pswE_pvad = sub(*pswE_pvad, swNormRvad);

    L_temp = 0;

    for (i = 1; i <= 8; i++)
      L_temp = L_mac(L_temp, pswSacf[i], pswRvad[i]);

    L_temp = L_add(L_temp, L_shr(L_mult(pswSacf[0], pswRvad[0]), 1));

    if (L_temp <= 0)
      L_temp = 1;

    swNormProd = norm_l(L_temp);
    *pswE_pvad = sub(*pswE_pvad, swNormProd);
    *pswM_pvad = extract_h(L_shl(L_temp, swNormProd));

  }

  /****************************************************************************
   *
   *     FUNCTION:  average_acf
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Computes the arrays L_av0 [0..8] and L_av1 [0..8].
   *
   *     INPUTS:    pL_acf[0..8]  autocorrelation of input signal frame
   *                swScaleAcf    L_acf scaling factor
   *
   *     OUTPUTS:   pL_av0[0..8]  ACF averaged over last four frames
   *                pL_av1[0..8]  ACF averaged over previous four frames
   *
   ***************************************************************************/

  void   Codec::average_acf(int32_t pL_acf[],
                            int16_t swScaleAcf,
                            int32_t pL_av0[],
                            int32_t pL_av1[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t L_temp;

    int16_t swScale;

    int    i;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /*** computation of the scaleing factor ***/

    swScale = sub(10, shl(swScaleAcf, 1));


    /*** Computation of the arrays L_av0 and L_av1 ***/

    for (i = 0; i <= 8; i++)
    {
      L_temp = L_shr(pL_acf[i], swScale);
      pL_av0[i] = L_add(pL_sacf[i], L_temp);
      pL_av0[i] = L_add(pL_sacf[i + 9], pL_av0[i]);
      pL_av0[i] = L_add(pL_sacf[i + 18], pL_av0[i]);
      pL_sacf[swPt_sacf + i] = L_temp;
      pL_av1[i] = pL_sav0[swPt_sav0 + i];
      pL_sav0[swPt_sav0 + i] = pL_av0[i];
    }


    /*** Update the array pointers ***/

    if (swPt_sacf == 18)
      swPt_sacf = 0;
    else
      swPt_sacf = add(swPt_sacf, 9);

    if (swPt_sav0 == 27)
      swPt_sav0 = 0;
    else
      swPt_sav0 = add(swPt_sav0, 9);

  }

  /****************************************************************************
   *
   *     FUNCTION:  predictor_values
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Computes the array rav [0..8] needed for the spectral
   *                comparison and the threshold adaptation.
   *
   *     INPUTS:    pL_av1 [0..8]  ACF averaged over previous four frames
   *
   *     OUTPUTS:   pswRav1 [0..8] ACF obtained from L_av1
   *                pswNormRav1    r_av1 scaling factor
   *
   ***************************************************************************/

  void  Codec::predictor_values(int32_t pL_av1[],
                                 int16_t pswRav1[],
                                 int16_t *pswNormRav1)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t
           pswVpar[8],
           pswAav1[9];

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    schur_recursion(pL_av1, pswVpar);
    step_up(8, pswVpar, pswAav1);
    compute_rav1(pswAav1, pswRav1, pswNormRav1);

  }

  /****************************************************************************
   *
   *     FUNCTION:  schur_recursion
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Uses the Schur recursion to compute adaptive filter
   *                reflection coefficients from an autorrelation function.
   *
   *     INPUTS:    pL_av1[0..8]   autocorrelation function
   *
   *     OUTPUTS:   pswVpar[0..7]  reflection coefficients
   *
   ***************************************************************************/

  void  Codec::schur_recursion(int32_t pL_av1[],
                                int16_t pswVpar[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t
           pswAcf[9],
           pswPp[9],
           pswKk[9],
           swTemp;

    int    i,
           k,
           m,
           n;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /*** Schur recursion with 16-bit arithmetic ***/

    if (pL_av1[0] == 0)
    {
      for (i = 0; i < 8; i++)
        pswVpar[i] = 0;
      return;
    }

    swTemp = norm_l(pL_av1[0]);

    for (k = 0; k <= 8; k++)
      pswAcf[k] = extract_h(L_shl(pL_av1[k], swTemp));


    /*** Initialise array pp[..] and kk[..] for the recursion: ***/

    for (i = 1; i <= 7; i++)
      pswKk[9 - i] = pswAcf[i];

    for (i = 0; i <= 8; i++)
      pswPp[i] = pswAcf[i];


    /*** Compute Parcor coefficients: ***/

    for (n = 0; n < 8; n++)
    {
      if (pswPp[0] < abs_s(pswPp[1]))
      {
        for (i = n; i < 8; i++)
          pswVpar[i] = 0;
        return;
      }
      pswVpar[n] = divide_s(abs_s(pswPp[1]), pswPp[0]);

      if (pswPp[1] > 0)
        pswVpar[n] = negate(pswVpar[n]);
      if (n == 7)
        return;


      /*** Schur recursion: ***/

      pswPp[0] = add(pswPp[0], mult_r(pswPp[1], pswVpar[n]));

      for (m = 1; m <= (7 - n); m++)
      {
        pswPp[m] = add(pswPp[1 + m], mult_r(pswKk[9 - m], pswVpar[n]));
        pswKk[9 - m] = add(pswKk[9 - m], mult_r(pswPp[1 + m], pswVpar[n]));
      }
    }

  }

  /****************************************************************************
   *
   *     FUNCTION:  step_up
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Computes the transversal filter coefficients from the
   *                reflection coefficients.
   *
   *     INPUTS:    swNp             filter order (2..8)
   *                pswVpar[0..np-1] reflection coefficients
   *
   *     OUTPUTS:   pswAav1[0..np]   transversal filter coefficients
   *
   ***************************************************************************/

  void  Codec::step_up(int16_t swNp,
                        int16_t pswVpar[],
                        int16_t pswAav1[])
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t
           pL_coef[9],
           pL_work[9];

    int16_t
           swTemp;

    int
           i,
           m;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /*** Initialisation of the step-up recursion ***/

    pL_coef[0] = L_shl(0x4000L, 15);
    pL_coef[1] = L_shl(L_deposit_l(pswVpar[0]), 14);


    /*** Loop on the LPC analysis order: ***/

    for (m = 2; m <= swNp; m++)
    {
      for (i = 1; i < m; i++)
      {
        swTemp = extract_h(pL_coef[m - i]);
        pL_work[i] = L_mac(pL_coef[i], pswVpar[m - 1], swTemp);
      }
      for (i = 1; i < m; i++)
        pL_coef[i] = pL_work[i];
      pL_coef[m] = L_shl(L_deposit_l(pswVpar[m - 1]), 14);
    }


    /*** Keep the aav1[0..swNp] in 15 bits for the following subclause ***/

    for (i = 0; i <= swNp; i++)
      pswAav1[i] = extract_h(L_shr(pL_coef[i], 3));

  }

  /****************************************************************************
   *
   *     FUNCTION:  compute_rav1
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Computes the autocorrelation function of the adaptive
   *                filter coefficients.
   *
   *     INPUTS:    pswAav1[0..8]  adaptive filter coefficients
   *
   *     OUTPUTS:   pswRav1[0..8]  ACF of aav1
   *                pswNormRav1    r_av1 scaling factor
   *
   ***************************************************************************/

  void Codec::compute_rav1(int16_t pswAav1[],
                             int16_t pswRav1[],
                             int16_t *pswNormRav1)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t
           pL_work[9];

    int
           i,
           k;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /*** Computation of the rav1[0..8] ***/

    for (i = 0; i <= 8; i++)
    {
      pL_work[i] = 0;

      for (k = 0; k <= 8 - i; k++)
        pL_work[i] = L_mac(pL_work[i], pswAav1[k], pswAav1[k + i]);
    }

    if (pL_work[0] == 0)
      *pswNormRav1 = 0;
    else
      *pswNormRav1 = norm_l(pL_work[0]);

    for (i = 0; i <= 8; i++)
      pswRav1[i] = extract_h(L_shl(pL_work[i], *pswNormRav1));

  }

  /****************************************************************************
   *
   *     FUNCTION:  spectral_comparison
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Computes the stat flag needed for the threshold
   *                adaptation decision.
   *
   *     INPUTS:    pswRav1[0..8]   ACF obtained from L_av1
   *                swNormRav1      rav1 scaling factor
   *                pL_av0[0..8]    ACF averaged over last four frames
   *
   *     OUTPUTS:   pswStat         flag to indicate spectral stationarity
   *
   ***************************************************************************/

  void Codec::spectral_comparison(int16_t pswRav1[],
                                    int16_t swNormRav1,
                                    int32_t pL_av0[],
                                    int16_t *pswStat)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t
           L_dm,
           L_sump,
           L_temp;

    int16_t
           pswSav0[9],
           swShift,
           swDivShift,
           swTemp;

    int
           i;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /*** Re-normalise L_av0[0..8] ***/

    if (pL_av0[0] == 0)
    {
      for (i = 0; i <= 8; i++)
        pswSav0[i] = 4095;
    }
    else
    {
      swShift = sub(norm_l(pL_av0[0]), 3);
      for (i = 0; i <= 8; i++)
        pswSav0[i] = extract_h(L_shl(pL_av0[i], swShift));
    }

    /*** compute partial sum of dm ***/

    L_sump = 0;

    for (i = 1; i <= 8; i++)
      L_sump = L_mac(L_sump, pswRav1[i], pswSav0[i]);

    /*** compute the division of the partial sum by sav0[0] ***/

    if (L_sump < 0)
      L_temp = L_negate(L_sump);
    else
      L_temp = L_sump;

    if (L_temp == 0)
    {
      L_dm = 0;
      swShift = 0;
    }
    else
    {
      pswSav0[0] = shl(pswSav0[0], 3);
      swShift = norm_l(L_temp);
      swTemp = extract_h(L_shl(L_temp, swShift));

      if (pswSav0[0] >= swTemp)
      {
        swDivShift = 0;
        swTemp = divide_s(swTemp, pswSav0[0]);
      }
      else
      {
        swDivShift = 1;
        swTemp = sub(swTemp, pswSav0[0]);
        swTemp = divide_s(swTemp, pswSav0[0]);
      }

      if (swDivShift == 1)
        L_dm = 0x8000L;
      else
        L_dm = 0;

      L_dm = L_shl((L_add(L_dm, L_deposit_l(swTemp))), 1);

      if (L_sump < 0)
        L_dm = L_sub(0L, L_dm);
    }


    /*** Re-normalisation and final computation of L_dm ***/

    L_dm = L_shl(L_dm, 14);
    L_dm = L_shr(L_dm, swShift);
    L_dm = L_add(L_dm, L_shl(L_deposit_l(pswRav1[0]), 11));
    L_dm = L_shr(L_dm, swNormRav1);


    /*** Compute the difference and save L_dm ***/

    L_temp = L_sub(L_dm, L_lastdm);
    L_lastdm = L_dm;

    if (L_temp < 0L)
      L_temp = L_negate(L_temp);


    /*** Evaluation of the state flag  ***/

    L_temp = L_sub(L_temp, 4456L);

    if (L_temp < 0)
      *pswStat = 1;
    else
      *pswStat = 0;

  }

  /****************************************************************************
   *
   *     FUNCTION:  tone_detection
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Computes the tone flag needed for the threshold
   *                adaptation decision.
   *
   *     INPUTS:    pswRc[0..3] reflection coefficients calculated in the
   *                            speech encoder short term predictor
   *
   *     OUTPUTS:   pswTone     flag to indicate a periodic signal component
   *
   ***************************************************************************/

  void Codec::tone_detection(int16_t pswRc[4],
                               int16_t *pswTone)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t
           L_num,
           L_den,
           L_temp;

    int16_t
           swTemp,
           swPredErr,
           pswA[3];

    int
           i;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    *pswTone = 0;


    /*** Calculate filter coefficients ***/

    step_up(2, pswRc, pswA);


    /*** Calculate ( a[1] * a[1] ) ***/

    swTemp = shl(pswA[1], 3);
    L_den = L_mult(swTemp, swTemp);


    /*** Calculate ( 4*a[2] - a[1]*a[1] ) ***/

    L_temp = L_shl(L_deposit_h(pswA[2]), 3);
    L_num = L_sub(L_temp, L_den);


    /*** Check if pole frequency is less than 385 Hz ***/

    if (L_num <= 0)
      return;

    if (pswA[1] < 0)
    {
      swTemp = extract_h(L_den);
      L_temp = L_msu(L_num, swTemp, 3189);

      if (L_temp < 0)
        return;
    }


    /*** Calculate normalised prediction error ***/

    swPredErr = 0x7fff;

    for (i = 0; i < 4; i++)
    {
      swTemp = mult(pswRc[i], pswRc[i]);
      swTemp = sub(32767, swTemp);
      swPredErr = mult(swPredErr, swTemp);
    }


    /*** Test if prediction error is smaller than threshold ***/

    swTemp = sub(swPredErr, 1464);

    if (swTemp < 0)
      *pswTone = 1;

  }

  #define M_PTH    26250
  #define E_PTH    18
  #define M_PLEV   17500
  #define E_PLEV   20
  #define M_MARGIN 27343
  #define E_MARGIN 27
  /****************************************************************************
   *
   *     FUNCTION:  threshold_adaptation
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Evaluates the secondary VAD decision.  If speech is not
   *                present then the noise model rvad and adaptive threshold
   *                thvad are updated.
   *
   *     INPUTS:    swStat        flag to indicate spectral stationarity
   *                swPtch        flag to indicate a periodic signal component
   *                swTone        flag to indicate a tone signal component
   *                pswRav1[0..8] ACF obtained from l_av1
   *                swNormRav1    r_av1 scaling factor
   *                swM_pvad      mantissa of filtered signal energy
   *                swE_pvad      exponent of filtered signal energy
   *                swM_acf0      mantissa of signal frame energy
   *                swE_acf0      exponent of signal frame energy
   *
   *     OUTPUTS:   pswRvad[0..8] autocorrelated adaptive filter coefficients
   *                pswNormRvad   rvad scaling factor
   *                pswM_thvad    mantissa of decision threshold
   *                pswE_thvad    exponent of decision threshold
   *
   ***************************************************************************/

  void Codec::threshold_adaptation(int16_t swStat,
                                     int16_t swPtch,
                                     int16_t swTone,
                                     int16_t pswRav1[],
                                     int16_t swNormRav1,
                                     int16_t swM_pvad,
                                     int16_t swE_pvad,
                                     int16_t swM_acf0,
                                     int16_t swE_acf0,
                                     int16_t pswRvad[],
                                     int16_t *pswNormRvad,
                                     int16_t *pswM_thvad,
                                     int16_t *pswE_thvad)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int32_t
           L_temp;

    int16_t
           swTemp,
           swComp,
           swComp2,
           swM_temp,
           swE_temp;

    int
           i;


  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    swComp = 0;

    /*** Test if acf0 < pth; if yes set thvad to plev ***/

    if (swE_acf0 < E_PTH)
      swComp = 1;
    if ((swE_acf0 == E_PTH) && (swM_acf0 < M_PTH))
      swComp = 1;

    if (swComp == 1)
    {
      *pswE_thvad = E_PLEV;
      *pswM_thvad = M_PLEV;

      return;
    }


    /*** Test if an adaption is required ***/

    if (swPtch == 1)
      swComp = 1;
    if (swStat == 0)
      swComp = 1;
    if (swTone == 1)
      swComp = 1;

    if (swComp == 1)
    {
      swAdaptCount = 0;
      return;
    }


    /*** Increment adaptcount ***/

    swAdaptCount = add(swAdaptCount, 1);
    if (swAdaptCount <= 8)
      return;


    /*** computation of thvad-(thvad/dec) ***/

    *pswM_thvad = sub(*pswM_thvad, shr(*pswM_thvad, 5));

    if (*pswM_thvad < 0x4000)
    {
      *pswM_thvad = shl(*pswM_thvad, 1);
      *pswE_thvad = sub(*pswE_thvad, 1);
    }


    /*** computation of pvad*fac ***/

    L_temp = L_mult(swM_pvad, 20889);
    L_temp = L_shr(L_temp, 15);
    swE_temp = add(swE_pvad, 1);

    if (L_temp > 0x7fffL)
    {
      L_temp = L_shr(L_temp, 1);
      swE_temp = add(swE_temp, 1);
    }
    swM_temp = extract_l(L_temp);


    /*** test if thvad < pavd*fac ***/

    if (*pswE_thvad < swE_temp)
      swComp = 1;

    if ((*pswE_thvad == swE_temp) && (*pswM_thvad < swM_temp))
      swComp = 1;


    /*** compute minimum(thvad+(thvad/inc), pvad*fac) when comp = 1 ***/

    if (swComp == 1)
    {

      /*** compute thvad + (thvad/inc) ***/

      L_temp = L_add(L_deposit_l(*pswM_thvad),L_deposit_l(shr(*pswM_thvad, 4)));

      if (L_temp > 0x7fffL)
      {
        *pswM_thvad = extract_l(L_shr(L_temp, 1));
        *pswE_thvad = add(*pswE_thvad, 1);
      }
      else
        *pswM_thvad = extract_l(L_temp);

      swComp2 = 0;

      if (swE_temp < *pswE_thvad)
        swComp2 = 1;

      if ((swE_temp == *pswE_thvad) && (swM_temp < *pswM_thvad))
        swComp2 = 1;

      if (swComp2 == 1)
      {
        *pswE_thvad = swE_temp;
        *pswM_thvad = swM_temp;
      }
    }


    /*** compute pvad + margin ***/

    if (swE_pvad == E_MARGIN)
    {
      L_temp = L_add(L_deposit_l(swM_pvad), L_deposit_l(M_MARGIN));
      swM_temp = extract_l(L_shr(L_temp, 1));
      swE_temp = add(swE_pvad, 1);
    }
    else
    {
      if (swE_pvad > E_MARGIN)
      {
        swTemp = sub(swE_pvad, E_MARGIN);
        swTemp = shr(M_MARGIN, swTemp);
        L_temp = L_add(L_deposit_l(swM_pvad), L_deposit_l(swTemp));

        if (L_temp > 0x7fffL)
        {
          swE_temp = add(swE_pvad, 1);
          swM_temp = extract_l(L_shr(L_temp, 1));
        }
        else
        {
          swE_temp = swE_pvad;
          swM_temp = extract_l(L_temp);
        }
      }
      else
      {
        swTemp = sub(E_MARGIN, swE_pvad);
        swTemp = shr(swM_pvad, swTemp);
        L_temp = L_add(L_deposit_l(M_MARGIN), L_deposit_l(swTemp));

        if (L_temp > 0x7fffL)
        {
          swE_temp = add(E_MARGIN, 1);
          swM_temp = extract_l(L_shr(L_temp, 1));
        }
        else
        {
          swE_temp = E_MARGIN;
          swM_temp = extract_l(L_temp);
        }
      }
    }

    /*** Test if thvad > pvad + margin ***/

    swComp = 0;

    if (*pswE_thvad > swE_temp)
      swComp = 1;

    if ((*pswE_thvad == swE_temp) && (*pswM_thvad > swM_temp))
      swComp = 1;

    if (swComp == 1)
    {
      *pswE_thvad = swE_temp;
      *pswM_thvad = swM_temp;
    }

    /*** Normalise and retain rvad[0..8] in memory ***/

    *pswNormRvad = swNormRav1;

    for (i = 0; i <= 8; i++)
      pswRvad[i] = pswRav1[i];

    /*** Set adaptcount to adp + 1 ***/

    swAdaptCount = 9;

  }

  /****************************************************************************
   *
   *     FUNCTION:  vad_decision
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Computes the VAD decision based on the comparison of the
   *                floating point representations of pvad and thvad.
   *
   *     INPUTS:    swM_pvad      mantissa of filtered signal energy
   *                swE_pvad      exponent of filtered signal energy
   *                swM_thvad     mantissa of decision threshold
   *                swE_thvad     exponent of decision threshold
   *
   *     OUTPUTS:   pswVvad       vad decision before hangover is added
   *
   ***************************************************************************/

  void Codec::vad_decision(int16_t swM_pvad,
                             int16_t swE_pvad,
                             int16_t swM_thvad,
                             int16_t swE_thvad,
                             int16_t *pswVvad)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    *pswVvad = 0;

    if (swE_pvad > swE_thvad)
      *pswVvad = 1;
    if ((swE_pvad == swE_thvad) && (swM_pvad > swM_thvad))
      *pswVvad = 1;

  }

  /****************************************************************************
   *
   *     FUNCTION:  vad_hangover
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Computes the final VAD decision for the current frame
   *                being processed.
   *
   *     INPUTS:    swVvad        vad decision before hangover is added
   *
   *     OUTPUTS:   pswVadFlag    vad decision after hangover is added
   *
   ***************************************************************************/

  void Codec::vad_hangover(int16_t swVvad,
                             int16_t *pswVadFlag)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    if (swVvad == 1)
      swBurstCount = add(swBurstCount, 1);
    else
      swBurstCount = 0;

    if (swBurstCount >= 3)
    {
      swHangCount = 5;
      swBurstCount = 3;
    }

    *pswVadFlag = swVvad;

    if (swHangCount >= 0)
    {
      *pswVadFlag = 1;
      swHangCount = sub(swHangCount, 1);
    }

  }

  /****************************************************************************
   *
   *     FUNCTION:  periodicity_update
   *
   *     VERSION:   1.2
   *
   *     PURPOSE:   Computes the ptch flag needed for the threshold
   *                adaptation decision for the next frame.
   *
   *     INPUTS:    pswLags[0..3]    speech encoder long term predictor lags
   *
   *     OUTPUTS:   pswPtch          Boolean voiced / unvoiced decision
   *
   ***************************************************************************/

  void Codec::periodicity_update(int16_t pswLags[4],
                                   int16_t *pswPtch)
  {

  /*_________________________________________________________________________
   |                                                                         |
   |                            Automatic Variables                          |
   |_________________________________________________________________________|
  */

    int16_t
           swMinLag,
           swMaxLag,
           swSmallLag,
           swLagCount,
           swTemp;

    int
           i,
           j;

  /*_________________________________________________________________________
   |                                                                         |
   |                              Executable Code                            |
   |_________________________________________________________________________|
  */

    /*** Run loop for No. of sub-segments in the frame ***/

    swLagCount = 0;

    for (i = 0; i <= 3; i++)
    {
      /*** Search the maximum and minimum of consecutive lags ***/

      if (swOldLag > pswLags[i])
      {
        swMinLag = pswLags[i];
        swMaxLag = swOldLag;
      }
      else
      {
        swMinLag = swOldLag;
        swMaxLag = pswLags[i];
      }


      /*** Compute smallag (modulo operation not defined) ***/

      swSmallLag = swMaxLag;

      for (j = 0; j <= 2; j++)
      {
        if (swSmallLag >= swMinLag)
          swSmallLag = sub(swSmallLag, swMinLag);
      }


      /***  Minimum of smallag and minlag - smallag ***/

      swTemp = sub(swMinLag, swSmallLag);

      if (swTemp < swSmallLag)
        swSmallLag = swTemp;

      if (swSmallLag < 2)
        swLagCount = add(swLagCount, 1);


      /*** Save the current LTP lag ***/

      swOldLag = pswLags[i];
    }


    /*** Update the veryoldlagcount and oldlagcount ***/

    swVeryOldLagCount = swOldLagCount;
    swOldLagCount = swLagCount;


    /*** Make ptch decision ready for next frame ***/

    swTemp = add(swOldLagCount, swVeryOldLagCount);

    if (swTemp >= 7)
      *pswPtch = 1;
    else
      *pswPtch = 0;

  }

} // end of namespace
