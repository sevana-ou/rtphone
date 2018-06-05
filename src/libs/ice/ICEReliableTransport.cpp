/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEReliableTransport.h"
#include "ICETypes.h"
#include <algorithm>
using namespace ICEImpl;

//------------------- ReliableTransport::Packet --------------------------
ReliableTransport::Packet::Packet()
{
  mType = AppData;
  mSeqno = 0;
  mAppSeqno = 0;
}

ReliableTransport::Packet::~Packet()
{
}

bool            
ReliableTransport::Packet::parse(const void* dataptr, int datasize, Encryption* encryption)
{
  assert(encryption != NULL);
  assert(dataptr != NULL);
  assert(datasize != 0);

  // 1) Check if packet has mEncryption->GetBlockSize() bytes boundary
  if (datasize % encryption->blockSize())
    return false;
  
  // 2) Save data
  mData.enqueueBuffer(dataptr, datasize);

  // 2) Decrypt it using mEncryption provider
  encryption->decrypt(mData.mutableData(), mData.size());

  // 3) Check CRC from first 4 bytes (it is for little endian only)
  unsigned crc = mData.dequeueUInt();
  if (crc != encryption->crc(mData.data(), mData.size()))
    return false;

  // 4) Check next byte for signature
  unsigned char signature = mData.dequeueUChar();
  if (signature != RTSignature)
    return false;

  // 5) Next 2 bits are type of packet - AppData, Confirmation, Request or Ack
  unsigned short ts = mData.dequeueUShort();
  
  mType = (Packet::Type)((ts & 0xC000) >> 14);
  
  // 6) Next 14 bits are real size of packet
  unsigned short realsize = ts & 0x3FFF;

  // 7) Seqno
  mSeqno = mData.dequeueUShort();

  // 7) Check if we deal with AppData
  if (mType == AppData)
    mAppSeqno = mData.dequeueUShort();
  
  // 8) Truncate app data to real size
  mData.truncate(realsize);
  return true;
}

void
ReliableTransport::Packet::build(const void* dataptr, int datasize, Encryption* encryption)
{
  assert(encryption && dataptr && datasize);

  mData.clear();

  // Reserve place for CRC
  mData.enqueueUInt(0);
  
  // Signature
  mData.enqueueUChar(RTSignature);

  // Real size and type of packet
  unsigned short ts = (unsigned short)datasize;
  ts |= mType << 14;

  // Enqueue real size and type of packet
  mData.enqueueUShort(ts);

  // Enqueue sequence number
  mData.enqueueUShort(mSeqno);

  // Enqueue application sequence number if needed
  if (mType == Packet::AppData)
    mData.enqueueUShort(mAppSeqno);

  // Enqueue payload data
  mData.enqueueBuffer(dataptr, datasize);
  
  // Padding with zero bytes
  int tail = mData.size() % encryption->blockSize();
  if (tail)
  {
    for (int i=0; i < encryption->blockSize() - tail; i++)
      mData.enqueueUChar(0);
  }
  
  // Get CRC on packet
  unsigned crc = encryption->crc((const char*)mData.data() + 4, mData.size() - 4);
  
  crc = htonl(crc); //It is here as corresponding DequeueUInt does ntohl
  memcpy(mData.mutableData(), &crc, 4);

  // Encrypt
  encryption->encrypt(mData.mutableData(), mData.size());
}

void ReliableTransport::Packet::setType(Type packetType)
{
  mType = packetType;
}

ReliableTransport::Packet::Type  ReliableTransport::Packet::type()
{
  return mType;
}

void ReliableTransport::Packet::setSeqno(unsigned short seqno)
{
  mSeqno = seqno;
}

unsigned short ReliableTransport::Packet::seqno()
{
  return mSeqno;
}

void ReliableTransport::Packet::setAppSeqno(unsigned short seqno)
{
  mAppSeqno = seqno;
}

unsigned short ReliableTransport::Packet::appSeqno()
{
  return mAppSeqno;
}

ICEByteBuffer& ReliableTransport::Packet::data()
{
  return mData;
}
//-------------------- ReliableTransport --------------------

ReliableTransport::ReliableTransport()
{
  mEncryption = NULL;
  mSeqno = 0;
}

ReliableTransport::~ReliableTransport()
{
}

void ReliableTransport::setMaxDatagram(int size)
{
  mMaxDatagramSize = size;
}


int ReliableTransport::maxDatagram()
{
  return mMaxDatagramSize;
}

bool ReliableTransport::processIncoming(const void* dataptr, int datasize)
{
  Packet* p = new Packet();
  if (p->parse(dataptr, datasize, mEncryption))
  {
    switch (p->type())
    {
    case Packet::AppData: 
      processAppData(p);
      break;

    case Packet::Confirmation:
      processConfirmation(p);
      break;

    case Packet::Ack:
      processAck(p);
      break;

    case Packet::Request:
      processRequest(p);
      break;
    }
    
    return true;
  }
  else
  {
    delete p;
    return false;
  }
}

void
ReliableTransport::queueOutgoing(const void *dataPtr, int dataSize)
{
  // Split enqueued data to datagrams using mMaxDatagramSize value.
  // Do not forget about service bytes - seqno, appseqno, CRC, signature...
  int payloadsize = this->mMaxDatagramSize -  (2 /*CRC*/ + 1 /* signature */ + 2 /* type and size */);
  
  // Find packet count
  int packetcount = dataSize / payloadsize;
  if (dataSize % payloadsize)
    packetcount++;
  
  // Case input pointer
  const char* dataIn = (const char*)dataPtr;

  // Split data to packets
  for (int i=0; i<packetcount; i++)
  {
    Packet *p = new Packet();
    p->setSeqno(mSeqno++);
    p->setAppSeqno(i);
    if (i == packetcount-1 && dataSize % payloadsize)
      p->build(dataIn + i * payloadsize, dataSize % payloadsize, mEncryption);
    else
      p->build(dataIn + i * payloadsize, payloadsize, mEncryption);

    mOutgoingData.push_back(p);
  }
}

void 
ReliableTransport::processAck(Packet* p)
{
  // Ack received for confirmation
  int count = p->data().dequeueUShort();
  for (int i=0; i<count; i++)
  {
    // Extract Confirmation packet seqno
    int seqno = p->data().dequeueUShort();

    // Find corresponding confirmation packet and remove it
    removePacket(seqno);
  }

  delete p;
}

void 
ReliableTransport::processConfirmation(Packet* p)
{
  int count = p->data().dequeueUShort();
  for (int i=0; i<count; i++)
  {
    // Extract AppData packet seqno
    int seqno = p->data().dequeueUShort();
    
    // Find corresponding AppData packet and remove it
    removePacket(seqno);
  }
  
  // Create Ack packet
  mOutgoingData.push_back(makeAckPacket(p->seqno()));

  delete p;
}

void 
ReliableTransport::processRequest(Packet* p)
{
  int count = p->data().dequeueUShort();
  for (int i=0; i<count; i++)
  {
    // Extract AppData packet seqno
    int seqno = p->data().dequeueUShort();

    // Find specified by seqno packet and move to top of list
    prioritizePacket(seqno);
  }

  delete p;
}

void 
ReliableTransport::processAppData(Packet* p)
{
  // 1) Add seqno to confirmation list
  mConfirmationData.push_back(p->seqno());

  // 2) Check if confirmation list if big enough to transmit confirmation packet
  if (mConfirmationData.size() >= (unsigned)MaxConfirmationQueue)
  {
    mOutgoingData.push_back(makeConfirmationPacket(mConfirmationData));
    mConfirmationData.clear();
  }
  
  // 3) Move packet to mIncomingAppData with optional packet assembling
  mIncomingAppData.push_back(p);
}

void 
ReliableTransport::removePacket(Packet::Seqno seqno)
{
  PacketList::iterator pit = mOutgoingData.begin();
  while (pit != mOutgoingData.end())
  {
    Packet* p = *pit;

    if (p->seqno() == seqno)
    {
      pit = mOutgoingData.erase(pit);
      delete p;
    }
    else
      pit++;
  }
}

void 
ReliableTransport::prioritizePacket(Packet::Seqno seqno)
{
  PacketList::iterator pit = mOutgoingData.begin();
  while (pit != mOutgoingData.end())
  {
    Packet* p = *pit;

    if (p->seqno() == seqno)
    {
      // Remove pointer from old index
      pit = mOutgoingData.erase(pit);

      // Insert at beginning of queue
      mOutgoingData.insert(mOutgoingData.begin(), p);
      
      // Exit from loop
      break;
    }
    else
      pit++;
  }
}

ReliableTransport::Packet*
ReliableTransport::makeAckPacket(Packet::Seqno seqno)
{
  // Create packet
  Packet* p = new Packet();
  
  // Set type
  p->setType(Packet::Ack);
  
  // Convert seqno number to network byte order
  unsigned short value = htons(seqno);

  // Build packet
  p->build(&value, sizeof(value), mEncryption);

  // Return packet
  return p;
}

ReliableTransport::Packet*
ReliableTransport::makeConfirmationPacket(std::vector<Packet::Seqno> seqnovector)
{
  // Convert seqno values to network byte order
  std::vector<Packet::Seqno> value;
  for (unsigned i=0; i<seqnovector.size(); i++)
    value.push_back(htons(seqnovector[i]));

  // Create packet
  Packet* p = new Packet();
  
  // Set type
  p->setType(Packet::Ack);
  
  // Build packet
  p->build(&value[0], value.size() * sizeof(Packet::Seqno), mEncryption);
  
  // Return packet
  return p;
}

    
bool 
ReliableTransport::hasAppData()
{
  return !mIncomingAppData.empty();
}

unsigned ReliableTransport::appData(void* ptr)
{
  if (mIncomingAppData.empty())
    return 0;
  
  Packet& p = *mIncomingAppData.front();
  unsigned length = p.data().size();
  if (!ptr)
    return length;
  
  memcpy(ptr, p.data().data(), length);
  mIncomingAppData.erase(mIncomingAppData.begin());
  return length;
}

bool 
ReliableTransport::hasPacketToSend()
{
  return !mOutgoingData.empty();
}

void 
ReliableTransport::getPacketToSend(void* outputptr, int& outputsize)
{
  if (mOutgoingData.empty())
  {
    outputsize = 0;
  }
  else
  {
    if (!outputptr)
    {
      outputsize = mOutgoingData.front()->data().size();
    }
    else
    {
      Packet& p = *mOutgoingData.front();

      unsigned tocopy = MINVALUE((int)outputsize, (int)p.data().size());
      memcpy(outputptr, p.data().data(), p.data().size());
      outputsize = tocopy;

      mOutgoingData.erase(mOutgoingData.begin());
    }
  }
}

void ReliableTransport::setEncryption(ICEImpl::ReliableTransport::Encryption *encryption)
{
  mEncryption = encryption;
}
