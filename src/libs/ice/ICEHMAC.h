/*
  Based on 100% free public domain implementation of the HMAC-SHA1 algorithm
	by Chien-Chung, Chung (Jim Chung) <jimchung1221@gmail.com>
*/

#ifndef __ICE_HMAC_H
#define __ICE_HMAC_H

#include "ICESHA1.h"

class ICEHMAC: public ICESHA1
{
private:
		unsigned char m_ipad[64];
    unsigned char m_opad[64];

		unsigned char * szReport ;
		unsigned char * SHA1_Key ;
		unsigned char * AppendBuf1 ;
		unsigned char * AppendBuf2 ;


public:
		enum {
			SHA1_DIGEST_LENGTH	= 20,
			SHA1_BLOCK_SIZE		= 64,
			HMAC_BUF_LEN		= 4096
		} ;

		ICEHMAC()
			:szReport(new unsigned char[HMAC_BUF_LEN]),
        AppendBuf1(new unsigned char[HMAC_BUF_LEN]),
        AppendBuf2(new unsigned char[HMAC_BUF_LEN]),
        SHA1_Key(new unsigned char[HMAC_BUF_LEN])
		{
    }

    ~ICEHMAC()
    {
      delete[] szReport ;
      delete[] AppendBuf1 ;
      delete[] AppendBuf2 ;
      delete[] SHA1_Key ;
    }

    void GetDigest(unsigned char *text, int text_len, unsigned char* key, int key_len, unsigned char *digest);
};


#endif /* __HMAC_SHA1_H__ */
