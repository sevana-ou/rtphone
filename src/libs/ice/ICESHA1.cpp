/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICESHA1.h"

#ifdef USE_CRYPTOPP

#include "../CryptoPP/hmac.h"
#include "../CryptoPP/sha.h"

void hmacSha1Digest(const void* inputData, size_t inputSize, void* outputData, const void* key, size_t keySize)
{
  CryptoPP::HMAC<CryptoPP::SHA1> mac((const byte*)key, keySize);
  
  mac.Update((const byte*)inputData, inputSize);
  mac.Final((byte*)outputData);
}

#elif defined(USE_OPENSSL)

#include <openssl/hmac.h>

void hmacSha1Digest(const void* inputData, size_t inputSize, void* outputData, const void* key, size_t keySize)
{
  unsigned outputSize = 0;
  HMAC(EVP_sha1(), key, keySize, (const unsigned char*)inputData, inputSize, (unsigned char*)outputData, &outputSize);
}

#endif