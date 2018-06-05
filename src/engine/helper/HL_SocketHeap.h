/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __SOCKET_HEAP_H
#define __SOCKET_HEAP_H

#include "../config.h"
#include <map>
#include <vector>
#include <algorithm>

#ifdef USE_RESIP_INTEGRATION
# include "resiprocate/rutil/ThreadIf.hxx"
#else
# include <thread>
#endif

#include "HL_NetworkSocket.h"
#include "HL_Sync.h"
#include "HL_Rtp.h"

// Class is used to process incoming datagrams
class SocketSink
{
public:
  virtual void onReceivedData(PDatagramSocket socket, InternetAddress& src, const void* receivedPtr, unsigned receivedSize) = 0;
};

// Class allocates new UDP sockets and tracks incoming packets on them. It runs in separate thread
#ifdef USE_RESIP_INTEGRATION
class SocketHeap: public resip::ThreadIf
#else
class SocketHeap: public std::thread
#endif
{
public:
  enum Multiplex
  {
    DoMultiplexing,
    DontMultiplexing
  };

  SocketHeap(unsigned short start, unsigned short finish);
  ~SocketHeap();

  static SocketHeap& instance();

  void start();
  void stop();

  // Specifies ne\ port number range. The sockets will be allocated in range [start..finish]
  void setRange(unsigned short start, unsigned short finish);

  // Returns used port number range
  void range(unsigned short& start, unsigned short& finish);
  
  // Attempts to allocate and return socket + allocated port number. REQUIRES pointer to data sink - it will be used to process incoming datagrams
  PDatagramSocket allocSocket(int family, SocketSink* sink, int port = 0);
  RtpPair<PDatagramSocket> allocSocketPair(int family, SocketSink* sink,  Multiplex m);

  // Stops receiving data for specified socket and frees socket itself.
  void freeSocket(PDatagramSocket socket);
  void freeSocketPair(const RtpPair<PDatagramSocket>& p);

  // Sends data to specified address on specified socket.
  void sendData(DatagramSocket& socket, InternetAddress& dest, const void* dataPtr, int dataSize);
  
protected:
  struct SocketItem
  {
    // Local port number for socket
    PDatagramSocket  mSocket;
    
    // Data sink pointer
    SocketSink* mSink;
    
    SocketItem()
      :mSink(NULL)
    { }

    SocketItem(unsigned short portnumber, SocketSink* sink)
      :mSink(sink)
    { 
      mSocket->mLocalPort = portnumber;
    }
    
    ~SocketItem()
    { }
  };

  typedef std::map<SOCKET, SocketItem> SocketMap;
  typedef std::vector<unsigned short> PortVector;
  typedef std::vector<PDatagramSocket> SocketVector;

  Mutex           mGuard;
  SocketMap       mSocketMap;
  PortVector      mPortVector;
  unsigned short  mStart, 
                  mFinish;
  SocketVector    mDeleteVector;
  Mutex           mDeleteGuard;

  char            mTempPacket[MAX_UDPPACKET_SIZE];
  volatile bool   mShutdown = false;

  int             mId = 0;
  bool isShutdown() const { return mShutdown; }
  virtual void thread(); 
  
  // Processes mDeleteVector -> updates mSocketMap, removes socket items and closes sockets specified in mDeleteVector
  void processDeleted();

};

#endif
