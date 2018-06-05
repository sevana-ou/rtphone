/*
   ITU-T G.729A Speech Coder    ANSI-C Source Code
   Version 1.1    Last modified: September 1996

   Copyright (c) 1996,
   AT&T, France Telecom, NTT, Universite de Sherbrooke
   All rights reserved.
*/
#ifndef _G729_PRE_PROC_H
#define _G729_PRE_PROC_H

extern void Init_Pre_Process (CodState *coder);

extern void Pre_Process (CodState *coder, 
		  Word16 signal[],	/* Input/output signal */
		  Word16 lg		/* Length of signal    */ );


#endif
