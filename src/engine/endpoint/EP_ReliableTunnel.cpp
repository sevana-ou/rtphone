/* 
 * Copyright (C) 2007-2012 Dmytro Bogovych <dmytro.bogovych@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifdef _WIN32
# include <winsock2.h>
# include <windows.h>
#endif

#include <algorithm>

#include "EP_ReliableTunnel.h"
#include "EP_Engine.h"
#include "Log.h"

#include "../ICE/ICECRC32.h"

enum
{
  CONFIRMATION_PT = 1,
  DATA_PT = 2
};

#define CONFIRMATION_TIMEOUT 500
#define LOG_SUBSYSTEM "RT"

ReliableTunnel::ReliableTunnel(const char* streamname)
{
  mStack.setEncryption(this);
  mStreamName = streamname;
  mBandwidth = 0;
  mExitSignal = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  mDataSignal = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  
}

ReliableTunnel::~ReliableTunnel()
{
  ::CloseHandle(mDataSignal);
  ::CloseHandle(mExitSignal);
}

std::string ReliableTunnel::streamName()
{
  return mStreamName;
}

std::string ReliableTunnel::streamProfile()
{
  return "RTP/DP";
}

void ReliableTunnel::setDestinationAddress(InternetAddress& addr)
{
  mDestination = addr;
}


void ReliableTunnel::queueData(const void* bufferptr, int buffersize)
{
  assert(bufferptr != NULL);
  assert(buffersize != 0);

  resip::Lock l(mNewQueuedGuard);
  mNewQueued.push_back(std::string((const char*)bufferptr, buffersize));
  
  ::SetEvent(mDataSignal);
}

// This method is called by user agent to send ICE packet from mediasocket
void ReliableTunnel::sendData(InternetAddress& addr, const void* dataBuffer, unsigned int datasize)
{
  switch (addr.type())
  {
  case AF_INET:
    mSocket4.sendDatagram(addr, dataBuffer, datasize);
    return;

  case AF_INET6:
    mSocket4.sendDatagram(addr, dataBuffer, datasize);
    return;
  }
}

void ReliableTunnel::sessionEstablished(int conntype)
{
  // Start worker thread
  if (conntype == EV_ICE)
    run();
}

void ReliableTunnel::sessionTerminated()
{
  // Stop worker thread
  ::SetEvent(mExitSignal);
  shutdown();
  join();
}

void ReliableTunnel::updateSdpOffer(resip::SdpContents::Session::Medium& sdp)
{
  // Get new destination port
  mDestination.setPort((unsigned short)sdp.port());
  
  sdp.addCodec(resip::SdpContents::Session::Codec("rt", 104));
}

void ReliableTunnel::setSocket(DatagramSocket& socket4, DatagramSocket& socket6)
{
  mSocket4 = socket4;
  mSocket6 = socket6;
}
  
DatagramSocket& ReliableTunnel::socket(int family)
{
  switch (family)
  {
  case AF_INET:
    return mSocket4;

  case AF_INET6:
    return mSocket4;

  default:
    assert(0);
  }
}

bool ReliableTunnel::processSdpOffer(const resip::SdpContents::Session::Medium& media)
{
  //check for default port number
  mDestination.setPort(media.port());
  
  return true;
}

void ReliableTunnel::thread()
{
  // Construct event array
  while (true)
  {
    HANDLE eventarray[2] = { mDataSignal, mExitSignal };

    DWORD rescode = ::WaitForMultipleObjects(2, eventarray, FALSE, INFINITE);
    if (rescode == WAIT_OBJECT_0)
    {
      resip::Lock l(mNewQueuedGuard);
      for (unsigned i = 0; i<mNewQueued.size(); i++)
        mStack.queueOutgoing(mNewQueued[i].c_str(), mNewQueued[i].size());
      mNewQueued.clear();
      
      sendOutgoing();
    }
    else
      break;
  }
}

void ReliableTunnel::setBandwidth(unsigned int bytesPerSecond)
{
  mBandwidth = bytesPerSecond;
}

unsigned int ReliableTunnel::bandwidth()
{
  return mBandwidth;
}

void ReliableTunnel::processData(const void* dataptr, int datasize)
{
  resip::Lock l(mStackGuard);
  mStack.processIncoming(dataptr, datasize);
}

bool ReliableTunnel::hasData()
{
  resip::Lock l(mStackGuard);
  
  return mIncomingData.size() || mStack.hasAppData();
}

unsigned ReliableTunnel::getData(void* ptr, unsigned capacity)
{
  resip::Lock l(mStackGuard);
   
  char* dataOut = (char*)ptr;
  
  while (capacity && hasData())
  {
    // Check if mIncomingData is empty
    if (!mIncomingData.size())
    {
      unsigned available = mStack.appData(NULL);
      if (!available)
        return 0;

      mIncomingData.resize(available);
      mIncomingData.rewind();
      mStack.appData(mIncomingData.mutableData());
    }

    if (mIncomingData.size())
    {
      unsigned toCopy = min(capacity, mIncomingData.size());
      mIncomingData.dequeueBuffer(dataOut, toCopy);
      dataOut += toCopy;
      capacity -= toCopy;
    }
  }

  return dataOut - (char*)ptr;
}

// Returns block size for encryption algorythm
int ReliableTunnel::blockSize()
{
  return 8;
}
      
// Encrypts dataPtr buffer inplace. dataSize must be odd to GetBlockSize() returned value.
void ReliableTunnel::encrypt(void* dataPtr, int dataSize)
{
  if (mEncryptionKey.empty())
    return;

#ifdef USE_OPENSSL
  for (unsigned i=0; i<dataSize / blockSize(); i++)
    BF_ecb_encrypt((unsigned char*)dataPtr + i * blockSize(), (unsigned char*)dataPtr + i * blockSize(), &mCipher, BF_ENCRYPT);
#endif

#ifdef USE_CRYPTOPP
  for (unsigned i=0; i<dataSize / blockSize(); i++)
    mEncryptor.ProcessBlock((unsigned char*)dataPtr + i * blockSize());
#endif
}

// Decrypts dataPtr buffer inplace. dataSize must be odd to GetBlockSize() returned value.
void ReliableTunnel::decrypt(void* dataPtr, int dataSize)
{
  if (mEncryptionKey.empty())
    return;
#ifdef USE_OPENSSL
  for (unsigned i=0; i<dataSize / blockSize(); i++)
    BF_ecb_encrypt((unsigned char*)dataPtr + i * blockSize(), (unsigned char*)dataPtr + i * blockSize(), &mCipher, BF_DECRYPT);
#endif
  
#ifdef USE_CRYPTOPP
  for (unsigned i=0; i<dataSize / blockSize(); i++)
    mDecryptor.ProcessBlock((unsigned char*)dataPtr + i * blockSize());
#endif
}

// Calculates CRC
unsigned ReliableTunnel::crc(const void* dataptr, int datasize)
{
  unsigned long result;
  ICEImpl::CRC32 crc;
  crc.fullCrc((const unsigned char*)dataptr, datasize, &result);
  
  return result;
}

void ReliableTunnel::sendOutgoing()
{
  // Check if stack has to send smth
  if (mStack.hasPacketToSend())
  {
    // Get data to send
    char buffer[2048];
    int length = sizeof(buffer);
    mStack.getPacketToSend(buffer, length);
    
    // Send it over UDP
    sendData(this->mDestination, buffer, length);
  }
}

void ReliableTunnel::setEncryptionKey(void* ptr, unsigned length)
{
#ifdef USE_OPENSSL
  BF_set_key(&mCipher, length, (const unsigned char*)ptr);
#endif

#ifdef USE_CRYPTOPP
  mEncryptor.SetKey((unsigned char*)ptr, length);
  mDecryptor.SetKey((unsigned char*)ptr, length);
#endif

  // Save key
  mEncryptionKey = std::string((const char*)ptr, length);
}
