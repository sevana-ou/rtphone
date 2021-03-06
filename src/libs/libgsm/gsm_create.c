/*
 * gsm_create.c
 *
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */


//#include	"config.h"

#ifdef	HAS_STRING_H
#include	<string.h>
#else
#	include "gsm_proto.h"
//TODO: error C2371: 'memset' : redefinition; different basic types
//	extern char	* memset P((char *, int, int));
#endif

#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#else

//#	ifdef	HAS_MALLOC_H
//#		include 	<malloc.h>
//#	else
//		extern char * malloc();
//#	endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gsm.h"
#include "gsm_private.h"
#include "gsm_proto.h"

gsm gsm_create P0()
{
	gsm  r;

	r = (gsm)malloc(sizeof(struct gsm_state));
	if (!r) return r;

	memset((char *)r, 0, sizeof(*r));
	r->nrp = 40;

	return r;
}
