/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __RELIABLE_TRANSPORT_H
#define __RELIABLE_TRANSPORT_H

#include "ICEPlatform.h"

#include <string>
#include <vector>
#include "ICEByteBuffer.h"


namespace ICEImpl {

  // ReliableTransport is protocol stack to provide reliable transport over UDP protocol. It remains packet based protocol, not stream.
  // But it guarantees integrity and order of packets.
  class ReliableTransport
  {
  public:
    // ReliableTransport encryption interface
    class Encryption
    {
    public:
      // Returns block size for encryption algorythm
      virtual int       blockSize() = 0;
      
      // Encrypts dataPtr buffer inplace. dataSize must be odd to GetBlockSize() returned value.
      virtual void      encrypt(void* dataPtr, int dataSize) = 0;

      // Decrypts dataPtr buffer inplace. dataSize must be odd to GetBlockSize() returned value.
      virtual void      decrypt(void* dataPtr, int dataSize) = 0;

      // Calculates CRC
      virtual unsigned  crc(const void* dataptr, int datasize) = 0;
    };

    // Signature byte value
    static const int RTSignature = 123;

    // Maximal count of confirmation items
    static const int MaxConfirmationQueue = 50;

    ReliableTransport();
    ~ReliableTransport();
    
    // Sets maximal datagram size. This value must be odd to Encryption::GetBlockSize() returned value.
    void setMaxDatagram(int size);
    
    // Returns maximal datagram size
    int  maxDatagram();

    // Sets encryption interface
    void setEncryption(Encryption* encryption);
    
    // Returns current encryption interface. Default is NULL.
    Encryption* encryption();
    
    // Process incoming UDP packet. Returns true if packet is recognized. Otherwise it returns false.
    bool processIncoming(const void* dataPtr, int dataSize);
    
    // Enqueue outgoing packet
    void queueOutgoing(const void* dataPtr, int dataSize);
    
    // Checks if there are incoming application data
    bool hasAppData();

    // Returns incoming application data. ptr may be NULL - in this case method will return packet size.
    unsigned appData(void* ptr);
    
    // Checks if there are raw packet(s) to send
    bool hasPacketToSend();

    // Returns raw packet data to send.
    void getPacketToSend(void* outputPtr, int& outputSize);

  protected:
    
    class Packet
    {
    public:
      enum Type
      {
        Request = 0,
        Confirmation = 1,
        Ack = 2,
        AppData = 3
      };
      
      typedef unsigned short Seqno;
      typedef unsigned short AppSeqno;
      
      Packet(); 
      ~Packet();

      bool            parse(const void* dataptr, int datasize, Encryption* encryption);
      void            build(const void* dataptr, int datasize, Encryption* encryption);
      void            setType(Type packetType);
      Type            type();
      void            setSeqno(Seqno seqno);
      Seqno           seqno();
      void            setAppSeqno(AppSeqno seqno);
      AppSeqno        appSeqno();
      ByteBuffer&     data();

    protected:
      Type            mType;
      Seqno           mSeqno;
      AppSeqno        mAppSeqno;
      ByteBuffer      mData;
    };
    
    // List of UDP packets
    typedef std::vector<Packet*> PacketList;

    // Incoming UDP packets
    PacketList        mIncomingData;

    // Outgoing UDP packets
    PacketList        mOutgoingData;
    
    // Used encryption provider
    Encryption*       mEncryption;
    
    // Maximal datagram size
    int               mMaxDatagramSize;
    
    // Vector of packet numbers to confirm
    std::vector<Packet::Seqno> mConfirmationData;
    
    // Vector of packet numbers to ack
    std::vector<Packet::Seqno> mAckData;
    
    // Incoming decrypted application data packets
    PacketList         mIncomingAppData;

    // Seqno generator
    Packet::Seqno      mSeqno;

    // Process incoming Ack packets
    void processAck(Packet* p);
    
    // Process confirmation packets
    void processConfirmation(Packet* p);
    
    // Process requests packets
    void processRequest(Packet* p);

    // Process app. data
    void processAppData(Packet* p);

    // Remove packet from outgoing queue
    void removePacket(Packet::Seqno seqno);

    // Prioritizes packet with specified seqno
    void prioritizePacket(Packet::Seqno seqno);
    
    // Creates Ack packet for specified seqno index
    Packet* makeAckPacket(Packet::Seqno seqno);

    // Creates Confirm packet for specified seqno indexes
    Packet* makeConfirmationPacket(std::vector<unsigned short> seqnovector);
  };
} // end of namespace ICEImpl

#endif
