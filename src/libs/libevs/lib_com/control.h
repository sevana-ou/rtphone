/*====================================================================================
    EVS Codec 3GPP TS26.443 Nov 13, 2018. Version 12.11.0 / 13.7.0 / 14.3.0 / 15.1.0
  ====================================================================================*/

#ifndef _CONTROL_H
#define _CONTROL_H

/* BASOP -> FLC brigde: flow control instructions */

#include "stl.h"

#define FOR( a) if( incrFor(), 0); else for( a)
static __inline void incrFor( void)
{
}

#define WHILE( a) if (incrFlcWhile(), 0); else while(a)
static __inline void incrFlcWhile(void)
{
}

#define DO do

#define IF( a) if( incrIf(), a)
static __inline void incrIf( void)
{
}

#define ELSE else

#define SWITCH( a) switch( incrSwitch(), a)
static __inline void incrSwitch( void)
{
}

#define CONTINUE continue

#define BREAK break

#define GOTO goto

#endif /* _CONTROL_H */
