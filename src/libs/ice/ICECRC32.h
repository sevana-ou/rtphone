/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_CRC32_H
#define __ICE_CRC32_H
namespace ice
{
  class CRC32
  {
    public:
        CRC32(void);
        ~CRC32(void);

        void initialize(void);

        bool fileCrc(const char *sFileName, unsigned long *ulOutCRC);
        bool fileCrc(const char *sFileName, unsigned long *ulOutCRC, unsigned long ulBufferSize);

        unsigned long fullCrc(const unsigned char *sData, unsigned long ulDataLength);
        void fullCrc(const unsigned char *sData, unsigned long ulLength, unsigned long *ulOutCRC);

        void partialCrc(unsigned long *ulCRC, const unsigned char *sData, unsigned long ulDataLength);

    private:
        unsigned long reflect(unsigned long ulReflect, const char cChar);
        unsigned long ulTable[256]; // CRC lookup table array.
  };
}

#endif