/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEMD5.h"

using namespace ice;

#ifdef USE_CRYPTOPP
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "../CryptoPP/md5.h"
using namespace CryptoPP;

void ice::md5Bin(const void* inputData, size_t inputSize, void* digest)
{
  Weak::MD5 md5;
  md5.Update((const byte*)inputData, inputSize);

  md5.Final((byte*)digest);
}

#elif defined(USE_OPENSSL)
#  include <openssl/md5.h>

void ice::md5Bin(const void* inputData, size_t inputSize, void* digest)
{
  MD5_CTX md5;
  MD5_Init(&md5);
  MD5_Update(&md5, (void*)inputData, inputSize);
  MD5_Final((unsigned char*)digest, &md5);
}
#else

#include "md5_impl.h"
// Use own MD5 implementation
void ice::md5Bin(const void* inputData, size_t inputSize, void* digest)
{
    MD5Context ctx;
    md5Init(&ctx);
    md5Update(&ctx, (const uint8_t*)inputData, inputSize);
    md5Finalize(&ctx);

    memcpy(digest, ctx.digest, sizeof(ctx.digest));
}
#endif

