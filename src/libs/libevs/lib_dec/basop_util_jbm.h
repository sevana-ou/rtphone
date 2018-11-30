/*====================================================================================
    EVS Codec 3GPP TS26.442 Apr 03, 2018. Version 12.11.0 / 13.6.0 / 14.2.0
  ====================================================================================*/

/*! @file basop_util_jbm.h basop utility functions for JBM. */

#ifndef BASOP_UTIL_JBM_H
#define BASOP_UTIL_JBM_H

#include "typedef.h"

/*! Adds two uint32_t values with overflow like plain C. */
Word32 rtpTs_add( Word32 ts1, Word32 ts2 );

/*! Subtracts two uint32_t values with overflow like plain C. */
Word32 rtpTs_sub( Word32 ts1, Word32 ts2 );

#endif
