/* 
 * Copyright (C) 2007-2010 Dmytro Bogovych <dmytro.bogovych@gmail.com>
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
#ifndef __RELIABLE_TUNNEL_H
#define __RELIABLE_TUNNEL_H

#include "DataProvider.h"
#include "InternetAddress.h"
#include "rutil/ThreadIf.hxx"
#include <vector>
#include <string>

#include "../ICE/ICEReliableTransport.h"
#ifdef USE_CRYPTOPP
# include "../Libs/CryptoPP/blowfish.h"
#endif
#ifdef USE_OPENSSL
# include "../Libs/openssl/include/openssl/blowfish.h"
#endif

class ReliableTunnel: public DataProvider, public resip::ThreadIf, public ICEImpl::ReliableTransport::Encryption
{
public:
  ReliableTunnel(const char* streamname);
  virtual ~ReliableTunnel();
  
  // Returns provider RTP name
  virtual std::string   streamName();
  
  // Returns provider RTP profile name
  virtual std::string   streamProfile();
  
  // Sets destination IP address
  virtual void          setDestinationAddress(InternetAddress& addr);

  // Processes incoming data
  virtual void          processData(const void* dataBuffer, int dataSize);

  // This method is called by user agent to send ICE packet from mediasocket
  virtual void          sendData(InternetAddress& destination, const void* dataBuffer, unsigned int datasize);

  // Updates SDP offer
  virtual void          updateSdpOffer(resip::SdpContents::Session::Medium& sdp);

  // Called by user agent when session is terminated.
  virtual void          sessionTerminated();

  // Called by user agent when session is started.
  virtual void          sessionEstablished(int conntype);

    // Called by user agent to save media socket for this provider
  virtual void          setSocket(DatagramSocket& socket4, DatagramSocket& socket6);
  
  // Called by user agent to get media socket for this provider
  virtual DatagramSocket& socket(int family);

  // Called by user agent to process media stream description from remote peer.
  // Returns true if description is processed succesfully. Otherwise method returns false.
  virtual bool          processSdpOffer(const resip::SdpContents::Session::Medium& media); 

  virtual void          thread();

  // Enqueues outgoing packet to sending queue
  void                  queueData(const void* bufferPtr, int bufferSize);
  
  void                  setBandwidth(unsigned int bytesPerSecond);
  unsigned int          bandwidth();
  
  // Checks if there is any received application data
  bool                  hasData();
  
  // Reads received data. If ptr is NULL - the length of available data is returned.
  unsigned              getData(void* ptr, unsigned capacity);

  void                  setEncryptionKey(void* ptr, unsigned length);

protected:
  // SDP's stream name
  std::string             mStreamName;

  // Transport stack
  ICEImpl::ReliableTransport mStack;  
  
  // Socket handles to operate
  DatagramSocket          mSocket4;
  DatagramSocket          mSocket6;

  // Destination IP4/6 address
  InternetAddress         mDestination;
  
  // Win32 exit signal
  HANDLE                  mExitSignal;
  
  // Win32 "new outgoing data" signal
  HANDLE                  mDataSignal;
  
  // Mutex to protect queuing/sending outgoing data
  resip::Mutex            mOutgoingMtx;
                          
  std::vector<std::string>          
                          mNewQueued;
  resip::Mutex            mNewQueuedGuard;
  resip::Mutex            mStackGuard;

  unsigned int            mBandwidth;
  std::string             mEncryptionKey;
  
#ifdef USE_CRYPTOPP
  CryptoPP::BlowfishEncryption mEncryptor;
  CryptoPP::BlowfishDecryption mDecryptor;
#endif

#ifdef USE_OPENSSL
  BF_KEY mCipher;
#endif

  ICEImpl::ICEByteBuffer  mIncomingData;

  // Returns block size for encryption algorythm
  int     blockSize();
      
  // Encrypts dataPtr buffer inplace. dataSize must be odd to GetBlockSize() returned value.
  void    encrypt(void* dataPtr, int dataSize);

  // Decrypts dataPtr buffer inplace. dataSize must be odd to GetBlockSize() returned value.
  void    decrypt(void* dataPtr, int dataSize);

  // Calculates CRC
  unsigned crc(const void* dataptr, int datasize);

  void sendOutgoing();
};
#endif