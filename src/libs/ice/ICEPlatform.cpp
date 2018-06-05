/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEPlatform.h"

#ifndef WIN32
#include <ctype.h>
char* strupr(char* buffer)
{
	char* result = buffer;
	
	while (*buffer)
	{
		*buffer = toupper(*buffer);
		buffer++;
	}
	
	return result; 
}
#endif
