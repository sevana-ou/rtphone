/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __HL_TYPES_H
#define __HL_TYPES_H

#ifdef WIN32
# define tstring wstring
# define to_tstring to_wstring
#else
# define tstring string
# define to_tstring to_string
#endif

#ifdef WIN32
#define ALLOCA(X) _alloca(X)
#else
#define ALLOCA(X) alloca(X)
#endif

enum SdpDirection
{
  Sdp_Answer,
  Sdp_Offer
};

#endif
