/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEPlatform.h"
#include "ICEStunMessage.h"
#include "ICECRC32.h"
#include "ICESHA1.h"
#include "ICEStunAttributes.h"
#include "ICEError.h"
#include "ICELog.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>

#ifndef _WIN32
# include <arpa/inet.h>
# include <alloca.h>
# include <stdexcept>
#endif

#define STUN_HEADER_SIZE 20
#define HMAC_DIGEST_SIZE 20
#define LOG_SUBSYSTEM "ICE"

using namespace ice;

bool StunMessage::TransactionID::operator == (const StunMessage::TransactionID& rhs)
{
  return memcmp(mValue, rhs.mValue, 12) == 0;
}

bool StunMessage::TransactionID::operator != (const TransactionID& rhs)
{
  return memcmp(mValue, rhs.mValue, 12) != 0;
}

StunMessage::TransactionID::TransactionID()
{
  for (int i=0; i<12; i++)
    mValue[i] = rand() & 0xFF;

  //memset(mValue, 0, 12);
}

std::string StunMessage::TransactionID::toStdString()
{
  char hex[3];
  std::string result;
  for (size_t i=0; i<12; i++)
  {
    sprintf(hex, "%2x", mValue[i]);
    result += hex;
  }
  
  return result;
}

StunMessage::TransactionID StunMessage::TransactionID::generateNew()
{
  TransactionID result;
  for (size_t i=0; i<12; i++)
    result.mValue[i] = (unsigned char)rand();

  return result;
}

//---------------------------  StunAttribute ----------------------
StunAttribute::StunAttribute()
{
  mLength = 0;
  mType = 0;
  mDataOffset = 0;
}

StunAttribute::~StunAttribute()
{
}

void StunAttribute::setDataOffset(size_t offset)
{
  mDataOffset = offset;
}

size_t StunAttribute::dataOffset() const
{
  return mDataOffset;
}

//----------------------------  StunMessage -----------------------
StunMessage::StunMessage()
:mMagicCookie(0x2112A442)
{

}

StunMessage::~StunMessage()
{
  // Iterate map to delete all attributes
  for (AttributeMap::iterator ait=mAttrMap.begin(); ait != mAttrMap.end(); ++ait)
    delete ait->second;
}

std::string StunMessage::comment()
{
  return mComment;
}

void StunMessage::setComment(std::string comment)
{
  mComment = comment;
}


void StunMessage::setMessageType(Type type)
{
  mType = type;
}


StunMessage::Type StunMessage::messageType()
{
  return mType;
}
  
void StunMessage::setMessageClass(Class value)
{
  mClass = value;
}

StunMessage::Class StunMessage::messageClass()
{
  return mClass;
}

unsigned int StunMessage::magicCookie()
{
  return mMagicCookie;
}

bool StunMessage::isMagicCookieValid()
{
  return mMagicCookie == 0x2112A442;
}
    
void StunMessage::setTransactionId(TransactionID id)
{
  mTransactionID = id;
}

StunMessage::TransactionID StunMessage::transactionId()
{
  return mTransactionID;
}

static void EmbedAttrBuffer(StunAttribute& attr, BufferWriter& writer)
{
  ByteBuffer attrBuffer; attrBuffer.resize(512);
  BufferWriter attrWriter(attrBuffer);
  attr.buildPacket(attrWriter);
  attrBuffer.resize(attrWriter.offset());

  // Enqueue attribute type
  writer.writeUShort(attr.type());
  
  // Enqueue length
  writer.writeUShort(attrBuffer.size());
  
  // Save data offset in generated buffer
  attr.setDataOffset(writer.offset() + 2); // 2 is for first 16 bits written by bitstream
  
  // Enqueue attribute's value
  if (attrBuffer.size() > 0)
    writer.writeBuffer(attrBuffer.data(), attrBuffer.size());
  
  if (attrBuffer.size() & 0x3)
  {
    size_t paddingLength = 4 - (attrBuffer.size() & 0x3);
    char paddingBytes[4] = { 0, 0, 0, 0 };
    writer.writeBuffer(paddingBytes, paddingLength);
  }
}

void StunMessage::buildPacket(ByteBuffer& buffer, const std::string& password)
{
  // Make enough space for packet
  buffer.resize(1024);
  
  // Write bits
  BitWriter bitstream(buffer);
  
  bitstream.writeBit(0)
           .writeBit(0);
  
  unsigned int msgtype = mType & 0xFFF;
  unsigned int msgclass = mClass & 0x3;

  // Enqueue last 5 bits of mtype
  for (size_t i=0; i<5; i++)
    bitstream.writeBit(bit(msgtype, 11 - i));

  // Enqueue last bit of msgclass
  bitstream.writeBit(bit(msgclass, 1));
  
  // Enqueue 3 bits of msgtype
  for (size_t i=0; i<3; i++)
    bitstream.writeBit(bit(msgtype, 6 - i));

  // Enqueue first bit of msgclass
  bitstream.writeBit(bit(msgclass, 0));

  // Enqueue 4 bits of msgtype
  for (size_t i=0; i<4; i++)
    bitstream.writeBit(bit(msgtype, 3-i));
  
  // Enqueue 2 bytes of length - now it is zero
  BufferWriter stream(buffer.mutableData() + bitstream.count() / 8);
  stream.writeUShort(0);

  // Enqueue magic cookie value
  unsigned int cookie = htonl(mMagicCookie);
  stream.writeBuffer(&cookie, 4);

  // Enqueue transaction ID
  //memset(mTransactionID.mValue, 0, sizeof(mTransactionID.mValue)); // For debugging only
  stream.writeBuffer(mTransactionID.mValue, 12);

  // Iterate attributes
  AttributeMap::iterator attrIter;
  for (attrIter = mAttrMap.begin(); attrIter != mAttrMap.end(); ++attrIter)
  {
    StunAttribute& attr = *attrIter->second;

    // Check if it is MessageIntegrity or Fingerprint - they comes last and skip them for now
    if (attr.type() == StunAttribute::MessageIntegrity || attr.type() == StunAttribute::Fingerprint)
      continue;

    EmbedAttrBuffer(attr, stream);
  }

  // Append MessageIntegrity attribute if exists
  AttributeMap::iterator miIter = mAttrMap.find(StunAttribute::MessageIntegrity);
  if ( miIter != mAttrMap.end())
  {
    EmbedAttrBuffer(*miIter->second, stream);
  }

  int lengthWithoutFingerprint = stream.offset() + 2;

  // Append Fingerprint attribute if exists
  AttributeMap::iterator fpIter = mAttrMap.find(StunAttribute::Fingerprint);
  if (fpIter != mAttrMap.end())
  {
    EmbedAttrBuffer(*fpIter->second, stream);
  }

  // Check for message integrity attribute
  miIter = mAttrMap.find(StunAttribute::MessageIntegrity);
  if (miIter != mAttrMap.end())
  {
      // Update length in header
      *((unsigned short*)buffer.mutableData() + 1) = htons(lengthWithoutFingerprint - STUN_HEADER_SIZE);
      
      // Prepare HMAC digest buffer
      unsigned char digest[HMAC_DIGEST_SIZE];
      
      // Get data offset for output digest
      size_t dataOffset = miIter->second->dataOffset();

      // Get digest
      hmacSha1Digest(buffer.data(), dataOffset - 4, digest, password.c_str(), password.length());

      // Copy digest to proper place
      memcpy((unsigned char*)buffer.data() + dataOffset, digest, HMAC_DIGEST_SIZE);
  }

  // Resize resulting buffer
  buffer.resize(stream.offset() + 2);

  // Put length in header
  *((unsigned short*)buffer.mutableData() + 1) = htons(buffer.size() - STUN_HEADER_SIZE);

  // Check for fingerprint attribute
  fpIter = mAttrMap.find(StunAttribute::Fingerprint);
  if (fpIter != mAttrMap.end())
  {
    // Get data offset for fingeprint attribute
    size_t dataOffset = fpIter->second->dataOffset();
      
    // Find CRC32
    CRC32 crc32;
    unsigned int crc = crc32.fullCrc((unsigned char*)buffer.data(), dataOffset - 4);
      
    // Update attribute value with CRC32 value
    memcpy((unsigned char*)buffer.data() + dataOffset, &crc, 4);
  }
  
}

#include "ICEStunAttributes.h"

bool StunMessage::parsePacket(ByteBuffer& buffer)
{
  // Save incoming packet
  mPacket2Parse = buffer;

  // Clear attribute list
  mAttrMap.clear();
  
  BufferReader stream(buffer);
  
  uint8_t firstByte = stream.readUChar();
  uint8_t secondByte = stream.readUChar();

  if (firstByte & ~0x3F)
  {
    return false;
  }

  int c1 = firstByte & 0x1;
  int m11 = firstByte & 0x20 ? 1 : 0;
  int m10 = firstByte & 0x10 ? 1 : 0;
  int m9 = firstByte & 0x8 ? 1 : 0;
  int m8 = firstByte & 0x4 ? 1 : 0;
  int m7 = firstByte & 0x2 ? 1 : 0;

  int m6 = secondByte & 0x80 ? 1 : 0;
  int m5 = secondByte & 0x40 ? 1 : 0;
  int m4 = secondByte & 0x20 ? 1 : 0;
  int c0 = secondByte & 0x10 ? 1 : 0;
  int m3 = secondByte & 0x8 ? 1 : 0;
  int m2 = secondByte & 0x4 ? 1 : 0;
  int m1 = secondByte & 0x2 ? 1 : 0;
  int m0 = secondByte & 0x1 ? 1 : 0;

  mType = (StunMessage::Type)((m11 << 11) + (m10 << 10) + (m9 << 9) + (m8 << 8) + (m7 << 7) + (m6 << 6) + (m5 << 5) + (m4 << 4) + (m3 << 3) + (m2 << 2) + (m1 << 1) + m0);
  mClass = (StunMessage::Class) ((c1 << 1) + c0);

  unsigned short length = stream.readUShort();
  if (length & 0x3)
  {
    return false;
  }

  // Dequeue magic cookie
  mMagicCookie = stream.readUInt();
  if (!isMagicCookieValid())
    return false;

  // Dequeue transaction id
  char id[12];
  stream.readBuffer(id, 12);
  
  memcpy(mTransactionID.mValue, id, 12);

  // Dequeue attributes
  while (stream.count() < buffer.size())
  {
    int attrType = stream.readUShort();
    StunAttribute* attr = NULL;
    switch (attrType)
    {
    case StunAttribute::Data:
      attr = new DataAttribute();
      break;

    case StunAttribute::MappedAddress:
      attr = new MappedAddress();
      break;

    case StunAttribute::XorMappedAddress:
      attr = new XorMappedAddress();
      break;

    case StunAttribute::ErrorCode:
      attr = new ErrorCode();
      break;

    case StunAttribute::AlternateServer:
      attr = new AlternateServer();
      break;

    case StunAttribute::UnknownAttributes:
      attr = new UnknownAttributes();
      break;

    case StunAttribute::Fingerprint:
      attr = new Fingerprint();
      break;

    case StunAttribute::MessageIntegrity:
      attr = new MessageIntegrity();
      break;

    case StunAttribute::Nonce:
      attr = new Nonce();
      break;

    case StunAttribute::Realm:
      attr = new Realm();
      break;

    case StunAttribute::Server:
      attr = new Server();
      break;

    case StunAttribute::Username:
      attr = new Username();
      break;

    case StunAttribute::ICEPriority:
      attr = new ICEPriority();
      break;

    case StunAttribute::ControlledAttr:
      attr = new ControlledAttr();
      break;

    case StunAttribute::ControllingAttr:
      attr = new ControllingAttr();
      break;

    case StunAttribute::ICEUseCandidate:
      attr = new ICEUseCandidate();
      break;

    case StunAttribute::XorRelayedAddress:
      attr = new XorRelayedAddress();
      break;
    
    case StunAttribute::XorPeerAddress:
      attr = new XorPeerAddress();
      break;

    case StunAttribute::Lifetime:
      attr = new Lifetime();
      break;
        
    case StunAttribute::RequestedTransport:
      attr = new RequestedTransport();
      break;
        
    case StunAttribute::RequestedAddressFamily:
      attr = new RequestedAddressFamily();
      break;
        
    default:
      attr = NULL;
    }

    // Get attribute value length
    int attrLen = stream.readUShort();
    
    unsigned int dataOffset = stream.count();

    // Get attribute buffer
    ByteBuffer attrBuffer;
    if (attrLen > 0)
    {
      attrBuffer.resize(attrLen);
      stream.readBuffer(attrBuffer.mutableData(), attrLen);
    }

    // Skip padding bytes
    if (attrLen & 0x3)
    {
      size_t paddingLength = 4 - (attrLen & 0x3);
      char paddingBytes[4];
      stream.readBuffer(paddingBytes, paddingLength);
    }

    // Parse attribute value
    if (attr)
    {
      // Parse attribute
      BufferReader attrReader(attrBuffer);
      if (!attr->parsePacket(attrReader))
        return false;
      
      // Set data offset
      attr->setDataOffset(dataOffset);

      // Add attribute
      addAttribute(attr);
    }
  }
  return true;
}

bool StunMessage::validatePacket(std::string key)
{
  // Iterate attributes
  if (hasAttribute(StunAttribute::MessageIntegrity))
  {
    // Get HMAC digest
    MessageIntegrity& mi = dynamic_cast<MessageIntegrity&>(attribute(StunAttribute::MessageIntegrity));

    unsigned char digest[HMAC_DIGEST_SIZE];
    
    // Reset length to length of packet without Fingerprint
    unsigned short* lengthPtr = ((unsigned short*)mPacket2Parse.mutableData() + 1);
    
    // Save old value - it is for backup purposes only. So there is no call to ntohs().
    unsigned short len = *lengthPtr;
    
    // Set fake length including MessageIntegrity attribute but excluding any attributes behind of it and excluding STUN header size
    *lengthPtr = htons(mi.dataOffset() + HMAC_DIGEST_SIZE - STUN_HEADER_SIZE);
    
    // Find HMAC-SHA1 again
    hmacSha1Digest(mPacket2Parse.data(), mi.dataOffset() - 4, digest, key.c_str(), key.size());
    
    // And restore old value
    *lengthPtr = len;
    
    if (memcmp(mi.value(), digest, HMAC_DIGEST_SIZE) != 0)
    {
      ICELogCritical(<< "Bad MessageIntegrity in STUN message");
      return false;
    }
  }
  
  /*
  if (hasAttribute(StunAttribute::Fingerprint))
  {
    // Get fingerpring attribute
    Fingerprint& fp = dynamic_cast<Fingerprint&>(attribute(StunAttribute::Fingerprint));

    // Find CRC32
    CRC32 crcObj;
    unsigned long crcValue = crcObj.fullCrc((const unsigned char*)mPacket2Parse.data(), fp.dataOffset() - 4);

    // Compare with saved one
    if (crcValue != fp.crc32())
    {
      ICELogCritical(<< "Bad CRC32 value in STUN message");
      return false;
    }
  }*/

  return true;
}

inline char StunMessage::bit(unsigned int value, size_t index)
{
  unsigned int mask = (1 << index);
  
  return value & mask ? 1 : 0;
}

inline void StunMessage::setBit(unsigned int& result, size_t index, char value)
{
  unsigned int mask = (1 << index);
  if (value != 0)
    result |= mask;
  else
    result &= ~mask;
}

void StunMessage::addAttribute(StunAttribute* attr)
{
  assert(NULL != attr);
  mAttrMap.insert(std::pair<int, StunAttribute*>(attr->type(), attr));
}

void StunMessage::setAttribute(StunAttribute *attr)
{
  assert(NULL != attr);
  
  AttributeMap::iterator attrIter = mAttrMap.find(attr->type());
  if (attrIter != mAttrMap.end())
  {
    delete attrIter->second;
    attrIter->second = attr;
  }
  else
    mAttrMap.insert(std::pair<int, StunAttribute*>(attr->type(), attr));
}

bool StunMessage::hasAttribute(int attrType) const
{
  return (mAttrMap.find(attrType) != mAttrMap.end());
}

 
StunAttribute& StunMessage::attribute(int attrType)
{
  AttributeMap::iterator attrIter = mAttrMap.find(attrType);
  if (attrIter != mAttrMap.end())
    return *attrIter->second;

  throw Exception(CANNOT_FIND_ATTRIBUTE, attrType);
}

const StunAttribute& 
StunMessage::operator [] (int attribute) const
{
  AttributeMap::iterator attrIter = mAttrMap.find(attribute);
  if (attrIter != mAttrMap.end())
    return *attrIter->second;

  throw Exception(CANNOT_FIND_ATTRIBUTE, attribute);
}

StunAttribute& 
StunMessage::operator[] (int attribute)
{
  AttributeMap::iterator attrIter = mAttrMap.find(attribute);
  if (attrIter != mAttrMap.end())
    return *attrIter->second;
  
  // Create attribute
  StunAttribute* attr = NULL;

  switch (attribute)
  {
  case StunAttribute::MappedAddress:
    attr = new MappedAddress();
    break;

  case StunAttribute::XorMappedAddress:
    attr = new XorMappedAddress();
    break;

  case StunAttribute::ErrorCode:
    attr = new ErrorCode();
    break;

  case StunAttribute::AlternateServer:
    attr = new AlternateServer();
    break;

  case StunAttribute::UnknownAttributes:
    attr = new UnknownAttributes();
    break;

  case StunAttribute::Fingerprint:
    attr = new Fingerprint();
    break;

  case StunAttribute::MessageIntegrity:
    attr = new MessageIntegrity();
    break;

  case StunAttribute::Nonce:
    attr = new Nonce();
    break;

  case StunAttribute::Realm:
    attr = new Realm();
    break;

  case StunAttribute::Server:
    attr = new Server();
    break;

  case StunAttribute::Username:
    attr = new Username();
    break;

  case StunAttribute::ICEPriority:
    attr = new ICEPriority();
    break;

  case StunAttribute::ControlledAttr:
    attr = new ControlledAttr();
    break;

  case StunAttribute::ControllingAttr:
    attr = new ControllingAttr();
    break;

  case StunAttribute::ICEUseCandidate:
    attr = new ICEUseCandidate();
    break;

  case StunAttribute::XorRelayedAddress:
    attr = new XorRelayedAddress();
    break;
  
  case StunAttribute::Lifetime:
    attr = new Lifetime();
    break;

  default:
    attr = NULL;
  }

  if (!attr)
    throw Exception(UNKNOWN_ATTRIBUTE, attribute);

  mAttrMap.insert(std::pair<int, StunAttribute*>(attribute, attr));
  return *attr;
}

Username& 
StunMessage::usernameAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<Username&>(self[StunAttribute::Username]);
}

Realm& 
StunMessage::realmAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<Realm&>(self[StunAttribute::Realm]);
}

ErrorCode&    
StunMessage::errorCodeAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<ErrorCode&>(self[StunAttribute::ErrorCode]);
}

Nonce&        
StunMessage::nonceAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<Nonce&>(self[StunAttribute::Nonce]);
}

MessageIntegrity&   
StunMessage::messageIntegrityAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<MessageIntegrity&>(self[StunAttribute::MessageIntegrity]);
}

MappedAddress&      
StunMessage::mappedAddressAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<MappedAddress&>(self[StunAttribute::MappedAddress]);
}

XorMappedAddress&   
StunMessage::xorMappedAddressAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<XorMappedAddress&>(self[StunAttribute::XorMappedAddress]);
}

ControlledAttr&      
StunMessage::iceControlledAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<ControlledAttr&>(self[StunAttribute::ControlledAttr]);
}

ControllingAttr&     
StunMessage::iceControllingAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<ControllingAttr&>(self[StunAttribute::ControllingAttr]);
}

ICEPriority&        
StunMessage::icePriorityAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<ICEPriority&>(self[StunAttribute::ICEPriority]);
}

Lifetime&           
StunMessage::lifetimeAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<Lifetime&>(self[StunAttribute::Lifetime]);
}

XorRelayedAddress&      
StunMessage::xorRelayedAddressAttr()
{
  StunMessage& self = *this;
  return dynamic_cast<XorRelayedAddress&>(self[StunAttribute::XorRelayedAddress]);
}

ChannelNumber&          
StunMessage::channelNumberAttr()
{
  StunMessage& self = *this;
  if (!self.hasAttribute(StunAttribute::ChannelNumber))
    self.addAttribute(new ChannelNumber());

  return dynamic_cast<ChannelNumber&>(self[StunAttribute::ChannelNumber]);
}

XorPeerAddress&         
StunMessage::xorPeerAddressAttr()
{
  StunMessage& self = *this;
  if (!self.hasAttribute(StunAttribute::XorPeerAddress))
    self.addAttribute(new XorPeerAddress());
  return dynamic_cast<XorPeerAddress&>(self[StunAttribute::XorPeerAddress]);
}

class CompareAttributesByOffsetFunctor
{
public:
  bool operator () (const StunAttribute* attr1, const StunAttribute* attr2)
  {
    return attr1->dataOffset() < attr2->dataOffset();
  }
};

void StunMessage::dump(std::ostream& output)
{
  switch (messageClass())
  {
  case RequestClass:    output << "Request"; break;
  case IndicationClass: output << "Indication"; break;
  case SuccessClass:    output << "Success"; break;
  case ErrorClass:      output << "Error"; break;
  default:
    output << "Invalid";
    break;
  }

  output << " / ";
  switch (messageType())
  {
  case Binding:           output << "Binding"; break;
  case Allocate:          output << "Allocate"; break;
  case Refresh:           output << "Refresh"; break;
  case Send:              output << "Send"; break;
  case Data:              output << "Data"; break;
  case CreatePermission:  output << "CreatePermission"; break;
  case ChannelBind:       output << "ChannelBind"; break;
  default:
    output << "Invalid";
    break;
  }

  output << std::endl;

  // Sort attribytes by data offset
  std::vector<StunAttribute*> attrList;
  for (AttributeMap::iterator attrIter = mAttrMap.begin(); attrIter != mAttrMap.end(); attrIter++)
    attrList.push_back(attrIter->second);

  std::sort(attrList.begin(), attrList.end(), CompareAttributesByOffsetFunctor());

  for (unsigned i=0; i<attrList.size(); i++)
  {
    output << "Offset " << attrList[i]->dataOffset() <<": ";
    attrList[i]->dump(output);
    output << std::endl;
  }
}
