/*
 *  sha1.h
 *
 *  Description:
 *      This is the header file for code which implements the Secure
 *      Hashing Algorithm 1 as defined in FIPS PUB 180-1 published
 *      April 17, 1995.
 *
 *      Many of the variable names in this code, especially the
 *      single character names, were used because those were the names
 *      used in the publication.
 *
 *      Please read the file sha1.c for more information.
 *
 */

#ifndef _HMAC_SHA1_H_
#define _HMAC_SHA1_H_

#include <stdint.h>

#define SHA1HashSize 20

enum
{
  shaSuccess = 0,
  shaNull,            /* Null pointer parameter */
  shaInputTooLong,    /* input data too long */
  shaStateError       /* called Input after Result */
};

#define FLAG_COMPUTED   1
#define FLAG_CORRUPTED  2

/*
 * Data structure holding contextual information about the SHA-1 hash
 */
struct sha1
{
  uint8_t  Message_Block[64];       /* 512-bit message blocks         */
  uint32_t Intermediate_Hash[5];    /* Message Digest                 */
  uint32_t Length_Low;              /* Message length in bits         */
  uint32_t Length_High;             /* Message length in bits         */
  uint16_t Message_Block_Index;     /* Index into message block array */
  uint8_t  flags;
};



/* 
 * Public API
 */
int sha1_reset (struct sha1* context);
int sha1_input (struct sha1* context, const uint8_t* message_array, unsigned length);
int sha1_result(struct sha1* context, uint8_t Message_Digest[SHA1HashSize]);



#define HMAC_SHA1_DIGEST_SIZE 20
#define HMAC_SHA1_BLOCK_SIZE  64

/***********************************************************************'
 * HMAC(K,m)      : HMAC SHA1
 * @param key     : secret key
 * @param keysize : key-length ín bytes
 * @param msg     : msg to calculate HMAC over
 * @param msgsize : msg-length in bytes
 * @param output  : writeable buffer with at least 20 bytes available
 */
void hmac_sha1(const uint8_t* key, const uint32_t keysize, const uint8_t* msg, const uint32_t msgsize, uint8_t* output);

#endif

