/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_SHA1_H
#define __ICE_SHA1_H

#include <stdlib.h>

extern void hmacSha1Digest(const void* inputData, size_t inputSize, void* outputData, const void* key, size_t keySize);

#endif
