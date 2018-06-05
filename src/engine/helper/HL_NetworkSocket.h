/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __NETWORK_SOCKET_H
#define __NETWORK_SOCKET_H

#include "HL_InternetAddress.h"
#include <vector>
#include <memory>

class NetworkSocket
{
public:
  virtual int localport() = 0;

};

class DatagramSocket
{
friend class SocketHeap;
friend class DatagramAgreggator;
public:
  DatagramSocket();
  virtual ~DatagramSocket();

  virtual int localport();
  
  virtual void      sendDatagram(InternetAddress& dest, const void* packetData, unsigned packetSize);
  virtual unsigned  recvDatagram(InternetAddress& src, void* packetBuffer, unsigned packetCapacity);
  virtual void      closeSocket();
  virtual bool      isValid() const;
  virtual int       family() const;
  virtual bool      setBlocking(bool blocking);
  virtual SOCKET    socket() const;

  virtual void open(int family);
protected:
  int mFamily;
  SOCKET mHandle;
  int mLocalPort;
};
typedef std::shared_ptr<DatagramSocket> PDatagramSocket;

class DatagramAgreggator
{
public:
  DatagramAgreggator();
  ~DatagramAgreggator();

  void      addSocket(PDatagramSocket socket);
  unsigned  count();
  bool      hasDataAtIndex(unsigned index);
  PDatagramSocket socketAt(unsigned index);
  
  bool      waitForData(unsigned milliseconds);

protected:
  typedef std::vector<PDatagramSocket> SocketList;
  SocketList mSocketVector;
  fd_set mReadSet;
  SOCKET mMaxHandle;
};

#endif
