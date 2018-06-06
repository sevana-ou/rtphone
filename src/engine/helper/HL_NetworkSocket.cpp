/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if defined(TARGET_LINUX) || defined(TARGET_ANDROID)
# include <asm/ioctls.h>
#endif

#include "../config.h"
#include "HL_NetworkSocket.h"

#if defined(TARGET_OSX) || defined(TARGET_LINUX)
# include <fcntl.h>
#endif

#if !defined(TARGET_WIN)
# include <unistd.h>
#endif
#include <assert.h>

DatagramSocket::DatagramSocket()
:mFamily(AF_INET), mHandle(INVALID_SOCKET), mLocalPort(0)
{
  
}

DatagramSocket::~DatagramSocket()
{
  closeSocket();
}

void DatagramSocket::open(int family)
{
  if (mHandle != INVALID_SOCKET || mFamily != family)
    closeSocket();

  assert(family == AF_INET || family == AF_INET6);
  mFamily = family;
  mHandle = ::socket(mFamily, SOCK_DGRAM, IPPROTO_UDP);
  if (mHandle != INVALID_SOCKET)
  {
    sockaddr_in addr4; sockaddr_in6 addr6;
    memset(&addr4, 0, sizeof(addr4)); memset(&addr6, 0, sizeof(addr6));
    socklen_t l = mFamily == AF_INET ? sizeof(addr4) : sizeof(addr6);
    int retcode = getsockname(mHandle, (mFamily == AF_INET ? (sockaddr*)&addr4 : (sockaddr*)&addr6), &l);
    if (!retcode)
    {
      mLocalPort = ntohs(mFamily == AF_INET ? addr4.sin_port : addr6.sin6_port);
    }
  }
}

int DatagramSocket::localport()
{
  return mLocalPort;
}

void DatagramSocket::sendDatagram(InternetAddress &dest, const void *packetData, unsigned int packetSize)
{
  if (mHandle == INVALID_SOCKET)
    return;

  int sent = ::sendto(mHandle, (const char*)packetData, packetSize, 0, dest.genericsockaddr(), dest.sockaddrLen());
}

unsigned DatagramSocket::recvDatagram(InternetAddress &src, void *packetBuffer, unsigned packetCapacity)
{
  if (mHandle == INVALID_SOCKET)
    return 0;

  sockaddr_in sourceaddr;
#ifdef WIN32
  int addrlen = sizeof(sourceaddr);
#else
  socklen_t addrlen = sizeof(sourceaddr);
#endif
  int received = ::recvfrom(mHandle, (char*)packetBuffer, packetCapacity, 0, (sockaddr*)&sourceaddr, &addrlen);
  if (received > 0)
  {
    src = InternetAddress((sockaddr&)sourceaddr, addrlen);
    return received;
  }
  else
    return 0;
}

void DatagramSocket::closeSocket()
{
  if (mHandle != INVALID_SOCKET)
  {
#ifdef WIN32
    ::closesocket(mHandle);
#else
      close(mHandle);
#endif
      mHandle = INVALID_SOCKET;
  }
}

bool DatagramSocket::isValid() const
{
  return mHandle != INVALID_SOCKET;
}

int DatagramSocket::family() const
{
  return mFamily;
}

bool DatagramSocket::setBlocking(bool blocking)
{
#if defined(TARGET_WIN)
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(mHandle, FIONBIO, &mode) == 0) ? true : false;
#endif
#if defined(TARGET_OSX) || defined(TARGET_LINUX)
   int flags = fcntl(mHandle, F_GETFL, 0);
   if (flags < 0)
       return false;
   flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
   return (fcntl(mHandle, F_SETFL, flags) == 0) ? true : false;
#endif
#if defined(TARGET_ANDROID)
  unsigned long mode = blocking ? 0 : 1;
  return (ioctl(mHandle, FIONBIO, &mode) == 0) ? true : false;
#endif
   return false;
}

SOCKET DatagramSocket::socket() const
{
  return mHandle;
}

DatagramAgreggator::DatagramAgreggator()
{
  FD_ZERO(&mReadSet);
  mMaxHandle = 0;
}

DatagramAgreggator::~DatagramAgreggator()
{
}

void DatagramAgreggator::addSocket(PDatagramSocket socket)
{
  if (socket->mHandle == INVALID_SOCKET)
    return;

  FD_SET(socket->mHandle, &mReadSet);
  if (socket->mHandle > mMaxHandle)
    mMaxHandle = socket->mHandle;

  mSocketVector.push_back(socket);
}

unsigned DatagramAgreggator::count()
{
  return mSocketVector.size();
}

bool DatagramAgreggator::hasDataAtIndex(unsigned index)
{
  PDatagramSocket socket = mSocketVector[index];
  return (FD_ISSET(socket->mHandle, &mReadSet) != 0);
}

PDatagramSocket DatagramAgreggator::socketAt(unsigned index)
{
  return mSocketVector[index];
}

bool DatagramAgreggator::waitForData(unsigned milliseconds)
{
  timeval tv;
  tv.tv_sec = milliseconds / 1000;
  tv.tv_usec = (milliseconds % 1000) * 1000;

  int rescode = ::select(mMaxHandle, &mReadSet, NULL, NULL, &tv);
  return rescode > 0;
}
