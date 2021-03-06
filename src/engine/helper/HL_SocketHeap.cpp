/* Copyright(C) 2007-2019 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../config.h"
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif
#include <set>
#include <assert.h>

#if !defined(TARGET_WIN)
# include <unistd.h> // Responsible for close() call on Linux
#endif

#include "HL_SocketHeap.h"
#include "HL_Log.h"
#include "HL_Sync.h"
#include "HL_Exception.h"

#define LOG_SUBSYSTEM "[SocketHeap]"

#ifndef WIN32
#define WSAGetLastError(X) errno
#define closesocket(X) close(X)
#define WSAEADDRINUSE EADDRINUSE
#endif


// ----------------------------- SocketSink -------------------------
SocketSink::~SocketSink()
{}

// ----------------------------- SocketHeap -------------------------

SocketHeap::SocketHeap(unsigned short start, unsigned short finish)
{
    mStart =  start;
    mFinish = finish;
}

SocketHeap::~SocketHeap()
{
    stop();
}

void SocketHeap::start()
{
    if (!mWorkerThread)
        mWorkerThread = std::make_shared<std::thread>(&SocketHeap::thread, this);
}

void SocketHeap::stop()
{
    if (mWorkerThread)
    {
        mShutdown = true;
        if (mWorkerThread->joinable())
            mWorkerThread->join();

        mWorkerThread.reset();
    }
}

void SocketHeap::setRange(unsigned short start, unsigned short finish)
{
    assert(mStart <= mFinish);

    Lock l(mGuard);
    mStart = start;
    mFinish = finish;
}

void SocketHeap::range(unsigned short &start, unsigned short &finish)
{
    Lock l(mGuard);

    start = mStart;
    finish = mFinish;
}

RtpPair<PDatagramSocket> SocketHeap::allocSocketPair(int family, SocketSink *sink, Multiplex m)
{
    PDatagramSocket rtp, rtcp;
    for (int attempt=0; (!rtp || !rtcp) && attempt < (mFinish - mStart)/2; attempt++)
    {
        // Allocate RTP
        try
        {
            rtp = allocSocket(family, sink);
            if (m == DoMultiplexing)
                rtcp = rtp;
            else
                rtcp = allocSocket(family, sink, rtp->localport() + 1);
        }
        catch(...)
        {}
    }

    if (!rtp || !rtcp)
    {
        if (rtp)
            freeSocket(rtp);
        if (rtcp)
            freeSocket(rtcp);
        throw Exception(ERR_NET_FAILED);
    }
    ICELogInfo(<< "Allocated socket pair " << (family == AF_INET ? "AF_INET" : "AF_INET6") << " "
               << rtp->socket() << ":" << rtcp->socket()
               << " at ports " << rtp->localport() << ":"<< rtcp->localport());

    return RtpPair<PDatagramSocket>(rtp, rtcp);
}

void SocketHeap::freeSocketPair(const RtpPair<PDatagramSocket> &p)
{
    freeSocket(p.mRtp);
    freeSocket(p.mRtcp);
}

PDatagramSocket SocketHeap::allocSocket(int family, SocketSink* sink, int port)
{
    Lock l(mGuard);
    SOCKET sock = ::socket(family, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
        // Return null socket
        PDatagramSocket result(new DatagramSocket());
        result->mLocalPort = port;
        result->mFamily = family;
        return result;
    }

    // Obtain port number
    sockaddr_in addr;
    sockaddr_in6 addr6;
    int result = 0;
    int testport;
    do
    {
        testport = port ? port : rand() % ((mFinish - mStart) / 2) * 2 + mStart;

        switch (family)
        {
        case AF_INET:
            memset(&addr, 0, sizeof addr);
            addr.sin_family = AF_INET;
            addr.sin_port = htons(testport);
            result = ::bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof addr);
            if (result)
                result = WSAGetLastError();
            break;

        case AF_INET6:
            memset(&addr6, 0, sizeof addr6);
            addr6.sin6_family = AF_INET6;
            addr6.sin6_port = htons(testport);
            result = ::bind(sock, reinterpret_cast<const sockaddr*>(&addr6), sizeof addr6);
            if (result)
                result = WSAGetLastError();
            break;
        }

    } while (result == WSAEADDRINUSE);

    if (result)
    {
        closesocket(sock);
        throw Exception(ERR_NET_FAILED, WSAGetLastError());
    }
    PDatagramSocket resultObject(new DatagramSocket());
    resultObject->mLocalPort = testport;
    resultObject->mHandle = sock;
    if (!resultObject->setBlocking(false))
    {
        resultObject->closeSocket();
        throw Exception(ERR_NET_FAILED, WSAGetLastError());
    }

    // Put socket object to the map
    mSocketMap[sock].mSink = sink;
    mSocketMap[sock].mSocket = resultObject;

    return resultObject;
}


void SocketHeap::freeSocket(PDatagramSocket socket)
{
    if (!socket)
        return;

    Lock l(mDeleteGuard);
    mDeleteVector.push_back(socket);
}

void SocketHeap::processDeleted()
{
    Lock l(mDeleteGuard);

    SocketVector::iterator socketIter = mDeleteVector.begin();
    while (socketIter != mDeleteVector.end())
    {
        // Find socket to delete in main socket map
        SocketMap::iterator itemIter = mSocketMap.find((*socketIter)->mHandle);

        if (itemIter != mSocketMap.end())
        {
            // If found - delete socket object from map
            mSocketMap.erase(itemIter);
        }

        socketIter++;
    }

    mDeleteVector.clear();
}

void SocketHeap::thread()
{
    /*#ifdef __linux__
    // TODO: make epoll implementation for massive polling
#else*/
    mThreadId = std::this_thread::get_id();

    while (!isShutdown())
    {
        // Define socket agreggator
        DatagramAgreggator agreggator;

        // Make a protected copy of sockets
        {
            Lock l(mGuard);

            // Remove deleted sockets from map and close them
            {
                processDeleted();
            }

            // Update socket set
            for (auto& socketIter: mSocketMap)
                agreggator.addSocket(socketIter.second.mSocket);

            /*      for (SocketMap::iterator socketIter = mSocketMap.begin(); socketIter != mSocketMap.end(); ++socketIter)
      {
        // Add handle to set
        agreggator.addSocket(socketIter->second.mSocket);
      } */
        }

        // If set is not empty
        if (agreggator.count() > 0)
        {
            if (agreggator.waitForData(10))
            {
                ICELogMedia(<< "There is data on UDP sockets");
                Lock l(mGuard);

                // Remove deleted sockets to avoid call non-existant sinks
                processDeleted();

                for (unsigned i=0; i<agreggator.count(); i++)
                {
                    if (agreggator.hasDataAtIndex(i))
                    {
                        //ICELogInfo(<<"Got incoming UDP packet at index " << (const int)i);
                        PDatagramSocket sock = agreggator.socketAt(i);

                        // Find corresponding data sink
                        SocketMap::iterator socketItemIter = mSocketMap.find(sock->mHandle);

                        if (socketItemIter != mSocketMap.end())
                        {
                            InternetAddress src;
                            unsigned received = sock->recvDatagram(src, mTempPacket, sizeof mTempPacket);

                            if ( received > 0 && received <= MAX_VALID_UDPPACKET_SIZE)
                                socketItemIter->second.mSink->onReceivedData(sock, src, mTempPacket, received);
                        }

                        // There is a call to ProcessDeleted() as OnReceivedData() could delete sockets
                        processDeleted();
                    }
                } //of for
            }
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    mShutdown = false;
    //#endif
}


static SocketHeap GRTPSocketHeap(20002, 25100);
SocketHeap& SocketHeap::instance()
{
    return GRTPSocketHeap;
}
