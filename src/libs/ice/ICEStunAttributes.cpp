/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEPlatform.h"
#include "ICEStunAttributes.h"
#include "ICENetworkHelper.h"
#include <stdexcept>
using namespace ice;



// --- MappedAddress ---
MappedAddress::MappedAddress()
{
}

MappedAddress::~MappedAddress()
{
}

int MappedAddress::type() const
{
  return StunAttribute::MappedAddress;
}

NetworkAddress& MappedAddress::address()
{
  return mAddress;
}

void MappedAddress::buildPacket(BufferWriter& writer)
{
  writer.writeUChar(0);
  writer.writeUChar(mAddress.stunType());
  writer.writeUShort(mAddress.port());
  
  switch (mAddress.stunType())
  {
  case IPv4:
    writer.writeBuffer(&mAddress.sockaddr4()->sin_addr, 4);
    break;

  case IPv6:
    writer.writeBuffer(&mAddress.sockaddr6()->sin6_addr, 16);
    break;

  default:
    assert(0);
  }
}

bool MappedAddress::parsePacket(BufferReader& reader)
{
  // Dequeue zero byte and ignore it
  /*uint8_t zeroByte = */reader.readUChar();

  // Dequeue family 
  mAddress.setStunType(reader.readUChar());

  // Dequeue port
  mAddress.setPort(reader.readUShort());
  
  // Deqeueue IP
  mAddress.setIp(reader.readIp(mAddress.stunType() == IPv4 ? AF_INET : AF_INET6));
  
  return true;
}

void MappedAddress::dump(std::ostream& output)
{
  output << "MappedAddress " << mAddress.toStdString();
}


//----------- XorMappedAddress ---------------
XorMappedAddress::XorMappedAddress()
{}

XorMappedAddress::~XorMappedAddress()
{}

int XorMappedAddress::type() const
{
  return StunAttribute::XorMappedAddress;
}


void XorMappedAddress::buildPacket(BufferWriter& writer)
{
  // Queue zero byte
  writer.writeUChar(0);
  
  // Queue family
  writer.writeUChar(mAddress.stunType());

  // Xor&queue port
  uint16_t port = mAddress.port() ^ 0x2112;
  writer.writeUShort(port);

  // Xor&queue ip
  unsigned int ip4 = 0;
  uint32_t ip6[4];
  switch (mAddress.stunType())
  {
  case IPv4:
    ip4 = mAddress.sockaddr4()->sin_addr.s_addr; //this gets ip in network byte order
    ip4 = ntohl(ip4); // get host byte order
    ip4 ^= 0x2112A442;
    writer.writeUInt(ip4); // It will convert again to network byte order
    break;

  case IPv6:
    // Get copy of address in network byte order
    memcpy(&ip6, &mAddress.sockaddr6()->sin6_addr, 16);
    
    for (int i=0; i<3; i++)
      ip6[i] ^= htonl(0x2112A442);
      
    writer.writeBuffer(ip6, 16);
    break;
  }

}

bool XorMappedAddress::parsePacket(BufferReader& reader)
{
  // Dequeue zero byte and ignore it
  reader.readUChar();

  // Dequeue family 
  mAddress.setStunType(reader.readUChar());

  // Dequeue port
  mAddress.setPort(reader.readUShort() ^ 0x2112);

  // Deqeueue IP
  unsigned int ip4;
  union
  {
    uint32_t ip6[4];
    in6_addr addr6;
  } v6;
  
  switch (mAddress.stunType())
  {
  case IPv4:
    ip4 = htonl(reader.readUInt() ^ 0x2112A442);
    mAddress.setIp(ip4);
    break;

  case IPv6:
    reader.readBuffer(v6.ip6, 16);
    // XOR buffer
    for (int i=0; i<3; i++)
      v6.ip6[i] ^= htonl(0x2112A442);
    mAddress.setIp(v6.addr6);
    break;

  default:
    assert(0);
  }

  return true;
}

void XorMappedAddress::dump(std::ostream &output)
{
  output << "XorMappedAddress " << mAddress.toStdString();
}

//------------ StringAttr -----------------------
StringAttr::StringAttr()
{
}
StringAttr::~StringAttr()
{
}

int StringAttr::type() const
{
  return StunAttribute::UnknownAttributes;
}

std::string StringAttr::value() const
{
  return mValue;
}

void StringAttr::setValue(const std::string& value)
{
  mValue = value;
}

void StringAttr::buildPacket(BufferWriter& writer)
{
  if (!mValue.empty())
    writer.writeBuffer(mValue.c_str(), mValue.length());
}

bool StringAttr::parsePacket(BufferReader& reader)
{
  char temp[1024]; memset(temp, 0, sizeof temp);
  reader.readBuffer(temp, sizeof temp);
  mValue = temp;

  return true;
}

void StringAttr::dump(std::ostream& output)
{
  output << "StringAttr " << mValue;
}

//----------- Username ---------------------------------
Username::Username()
{}

Username::~Username()
{}

int Username::type() const
{
  return StunAttribute::Username;
}

void Username::dump(std::ostream &output)
{
  output << "Username " << value();
}

//----------- MessageIntegrity -------------------------
MessageIntegrity::MessageIntegrity()
{
  memset(mValue, 0, sizeof(mValue));
}
MessageIntegrity::~MessageIntegrity()
{
}

int MessageIntegrity::type() const
{
  return StunAttribute::MessageIntegrity;
}

void MessageIntegrity::setValue(const void* data)
{
  if (data)
    memcpy(mValue, data, 20);
}

const void* MessageIntegrity::value() const
{
  return mValue;
}

void MessageIntegrity::buildPacket(BufferWriter& stream)
{
  stream.writeBuffer(mValue, 20);
}

bool MessageIntegrity::parsePacket(BufferReader& stream)
{
  try
  {
    stream.readBuffer(mValue, 20);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

void MessageIntegrity::dump(std::ostream &output)
{
  ByteBuffer buffer(mValue, 20);
  output << "MessageIntegrity " << buffer.hexstring();
}

//--------------- Fingerprint ----------------
Fingerprint::Fingerprint()
{
  mCRC32 = 0;
}

Fingerprint::~Fingerprint()
{
}

int Fingerprint::type() const
{
  return StunAttribute::Fingerprint;
}

void Fingerprint::setCrc32(unsigned int crc)
{
  mCRC32 = crc;
}

unsigned int Fingerprint::crc32() const
{
  return mCRC32;
}

void Fingerprint::buildPacket(BufferWriter& stream)
{
  stream.writeUInt(mCRC32);
}

bool Fingerprint::parsePacket(BufferReader& stream)
{
  try
  {
    mCRC32 = stream.readUInt();
  }
  catch(...)
  {
    return false;
  }
  
  return true;
}

void Fingerprint::dump(std::ostream &output)
{
  output << "Fingerprint " << mCRC32;
}

//---------------- ErrorCode ---------------
ErrorCode::ErrorCode()
{
  mErrorCode = 0;
}

ErrorCode::~ErrorCode()
{
}

int ErrorCode::type() const
{
  return StunAttribute::ErrorCode;
}

void ErrorCode::setErrorCode(int errorCode)
{
  mErrorCode = errorCode;
}

int  ErrorCode::errorCode() const
{
  return mErrorCode;
}

void ErrorCode::setErrorPhrase(const std::string& phrase)
{
  mErrorPhrase = phrase;
}

std::string ErrorCode::errorPhrase() const
{
  return mErrorPhrase;
}

void ErrorCode::buildPacket(BufferWriter& stream)
{
  stream.writeUShort(0);
  
  uint8_t b = 0;
  // Get hundreds digit
  int digit = mErrorCode / 100;
  b = (digit & 4 ? 1 : 0) << 2;
  b |= (digit & 2 ? 1 : 0) << 1;
  b |= (digit & 1 ? 1 : 0);
  stream.writeUChar(b);
  
  stream.writeUChar(mErrorCode % 100);

  if (!mErrorPhrase.empty())
    stream.writeBuffer(mErrorPhrase.c_str(), mErrorPhrase.length());
}

bool ErrorCode::parsePacket(BufferReader& stream)
{
  try
  {
    stream.readUShort();
    unsigned char _class = stream.readUChar();
    unsigned char _number = stream.readUChar();

    mErrorCode = _class * 100 + _number;
    char temp[1024]; memset(temp, 0, sizeof temp);
    stream.readBuffer(temp, sizeof temp);
    mErrorPhrase = std::string(temp);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

void ErrorCode::dump(std::ostream &output)
{
  output << "ErrorCode " << mErrorCode << " " << mErrorPhrase;
}

//----------------- Realm ------------------------
Realm::Realm()
{}

Realm::~Realm()
{}

int Realm::type() const
{
  return StunAttribute::Realm;
}

void Realm::dump(std::ostream &output)
{
  output << "Realm " << value();
}

//----------------- Nonce ------------------------
Nonce::Nonce()
{}

Nonce::~Nonce()
{}

int Nonce::type() const
{
  return StunAttribute::Nonce;
}

void Nonce::dump(std::ostream &output)
{
  output << "Nonce " << value();
}

//----------------- Server -----------------------
Server::Server()
{}

Server::~Server()
{}

int Server::type() const
{
  return StunAttribute::Server;
}

void Server::dump(std::ostream &output)
{
  output << "Server " << value();
}

//----------------- AlternateServer --------------
AlternateServer::AlternateServer()
{}

AlternateServer::~AlternateServer()
{}

int AlternateServer::type() const
{
  return StunAttribute::AlternateServer;
}

void AlternateServer::dump(std::ostream &output)
{
  output << "AlternateServer " << address().toStdString();
}

//----------------- UnknownAttributes ------------
UnknownAttributes::UnknownAttributes()
{
}

UnknownAttributes::~UnknownAttributes()
{
}

int UnknownAttributes::type() const
{
  return StunAttribute::UnknownAttributes;
}

void UnknownAttributes::buildPacket(BufferWriter& stream)
{
  char zeroBytes[8]; memset(zeroBytes, 0, sizeof(zeroBytes));
  stream.writeBuffer(zeroBytes, sizeof(zeroBytes));
}

bool UnknownAttributes::parsePacket(BufferReader& stream)
{
  try
  {
    char zeroBytes[8]; memset(zeroBytes, 0, sizeof(zeroBytes));
    stream.readBuffer(zeroBytes, sizeof(zeroBytes));
  }
  catch(...)
  {
    return false;
  }
  
  return true;
}

void UnknownAttributes::dump(std::ostream &output)
{
  output << "UnknownAttributes";
}

//---------------- ChannelNumber ----------------
ChannelNumber::ChannelNumber()
:mChannelNumber(0)
{
}

ChannelNumber::~ChannelNumber()
{
}

int ChannelNumber::type() const
{
  return StunAttribute::ChannelNumber;
}

void ChannelNumber::setChannelNumber(unsigned short value)
{
  mChannelNumber = value;
}

unsigned short ChannelNumber::channelNumber() const
{
  return mChannelNumber;
}

void  ChannelNumber::buildPacket(BufferWriter& stream)
{
  stream.writeUShort(mChannelNumber);
  stream.writeUChar(0);
  stream.writeUChar(0);
}

bool  ChannelNumber::parsePacket(BufferReader& stream)
{
  try
  {
    mChannelNumber = stream.readUShort();
    stream.readUChar();
    stream.readUChar();
  }
  catch(...)
  {
    return false;
  }

  return true;
}

void ChannelNumber::dump(std::ostream &output)
{
  output << "ChannelNumber " << mChannelNumber;
}

//--------------------------------- Lifetime -----------------------
Lifetime::Lifetime()
:mLifetime(0)
{
}

Lifetime::~Lifetime()
{
}

int Lifetime::type() const
{
  return StunAttribute::Lifetime;
}

void Lifetime::setLifetime(unsigned int value)
{
  mLifetime = value;
}

unsigned int Lifetime::lifetime() const
{
  return mLifetime;
}

void Lifetime::buildPacket(BufferWriter& stream)
{
  stream.writeUInt(mLifetime);
}

bool Lifetime::parsePacket(BufferReader& stream)
{
  try
  {
    mLifetime = stream.readUInt();
  }
  catch(...)
  {
    return false;
  }

  return true;
}

void Lifetime::dump(std::ostream &output)
{
  output << "Lifetime " << mLifetime;
}

//----------------------- DataAttribute ----------------------

DataAttribute::DataAttribute()
{
}

DataAttribute::~DataAttribute()
{
}

int DataAttribute::type() const
{
  return StunAttribute::Data;
}

void  DataAttribute::setData(ByteBuffer& buffer)
{
  mData = buffer;
}

ByteBuffer DataAttribute::data() const
{
  return mData;
}

void  DataAttribute::buildPacket(BufferWriter& stream)
{
  stream.writeBuffer(mData.data(), mData.size());
}

bool  DataAttribute::parsePacket(BufferReader& stream)
{
  mData.resize(1024);
  mData.resize(stream.readBuffer(mData.mutableData(), mData.size()));
  return true;
}

void DataAttribute::dump(std::ostream &output)
{
  output << "DataAttribute " << mData.hexstring();
}

//---------------------- XorRelayedTransport -------------------------
XorRelayedAddress::XorRelayedAddress()
{}

XorRelayedAddress::~XorRelayedAddress()
{}

int XorRelayedAddress::type() const
{
  return StunAttribute::XorRelayedAddress;
}

void XorRelayedAddress::dump(std::ostream &output)
{
  output << "XorRelayedAddress " << address().toStdString();
}

//---------------------- RequestedTransport ---------------------------
RequestedTransport::RequestedTransport()
:mRequestedTransport(UDP)
{
}

RequestedTransport::~RequestedTransport()
{
}

int RequestedTransport::type() const
{
  return StunAttribute::RequestedTransport;
}

void RequestedTransport::setRequestedTransport(RequestedTransport::TransportType value)
{
  mRequestedTransport = value;
}

unsigned char RequestedTransport::requestedTransport() const
{
  return mRequestedTransport;
}

void  RequestedTransport::buildPacket(BufferWriter& stream)
{
  stream.writeUChar(mRequestedTransport);
  stream.writeUChar(0);
  stream.writeUChar(0);
  stream.writeUChar(0);
}

bool RequestedTransport::parsePacket(BufferReader& stream)
{
  try
  {
    mRequestedTransport = stream.readUChar();
    stream.readUChar();
    stream.readUChar();
    stream.readUChar();
  }
  catch(...)
  {
    return false;
  }

  return true;
}

void RequestedTransport::dump(std::ostream &output)
{
  output << "RequestedTransport " << mRequestedTransport;
}

//--------------------------------- Controlled -----------------------
ControlledAttr::ControlledAttr()
{
}

ControlledAttr::~ControlledAttr()
{
}

int ControlledAttr::type() const
{
  return StunAttribute::ControlledAttr;
}

std::string ControlledAttr::tieBreaker() const
{
  return std::string((const char*)mTieBreaker, 8);
}

void ControlledAttr::setTieBreaker(const std::string& tieBreaker)
{
  assert(tieBreaker.length() == 8);

  memcpy(mTieBreaker, tieBreaker.c_str(), 8);
}

void ControlledAttr::buildPacket(BufferWriter& stream)
{
  stream.writeBuffer(mTieBreaker, 8);
}

bool ControlledAttr::parsePacket(BufferReader& stream)
{
  try
  {
    stream.readBuffer(mTieBreaker, 8);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

void ControlledAttr::dump(std::ostream &output)
{
  ByteBuffer b(mTieBreaker, 8);
  output << "ControlledAttr " << b.hexstring();
}

//------------------- ControllingAttr --------------
ControllingAttr::ControllingAttr()
{
  memset(mTieBreaker, 0, sizeof(mTieBreaker));
}

ControllingAttr::~ControllingAttr()
{
}

int ControllingAttr::type() const
{
  return StunAttribute::ControllingAttr;
}

std::string  ControllingAttr::tieBreaker() const
{
  return std::string((const char*)mTieBreaker, 8);
}

void ControllingAttr::setTieBreaker(const std::string& tieBreaker)
{
  memcpy(mTieBreaker, tieBreaker.c_str(), 8);
}

void ControllingAttr::buildPacket(BufferWriter& stream)
{
  stream.writeBuffer(mTieBreaker, 8);
}

bool ControllingAttr::parsePacket(BufferReader& stream)
{
  try
  {
    stream.readBuffer(mTieBreaker, 8);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

void ControllingAttr::dump(std::ostream &output)
{
  ByteBuffer b(mTieBreaker, 8);
  output << "ControllingAttr " << b.hexstring();
}

//----------- ICEPriority ---------------
ICEPriority::ICEPriority()
{
  mPriority = 0;
}

ICEPriority::~ICEPriority()
{
}

int ICEPriority::type() const
{
  return StunAttribute::ICEPriority;
}

unsigned int ICEPriority::priority() const
{
  return mPriority;
}

void ICEPriority::setPriority(unsigned int priority)
{
  mPriority = priority;
}

void ICEPriority::buildPacket(BufferWriter& stream)
{
  stream.writeUInt(mPriority);
}

bool ICEPriority::parsePacket(BufferReader& stream)
{
  try
  {
    mPriority = stream.readUInt();
  }
  catch(...)
  {
    return false;
  }

  return true;
}

void ICEPriority::dump(std::ostream &output)
{
  output << "ICEPriority " << mPriority;
}

//------------------ USE-CANDIDATE -----------------------
ICEUseCandidate::ICEUseCandidate()
{
}

ICEUseCandidate::~ICEUseCandidate()
{
}

int ICEUseCandidate::type() const
{
  return StunAttribute::ICEUseCandidate;
}

void ICEUseCandidate::buildPacket(BufferWriter& /*buffer*/)
{
}

bool ICEUseCandidate::parsePacket(BufferReader& /*buffer*/)
{
  return true;
}

void ICEUseCandidate::dump(std::ostream &output)
{
  output << "ICEUseCandidate";
}
// ------------- REQUESTED-ADDRESS-FAMILY -------------
RequestedAddressFamily::RequestedAddressFamily()
:mAddressFamily(IPv4)
{}

RequestedAddressFamily::RequestedAddressFamily(AddressFamily family)
  :mAddressFamily(family)
{}

RequestedAddressFamily::~RequestedAddressFamily()
{}

int RequestedAddressFamily::type() const
{
  return StunAttribute::RequestedAddressFamily;
}

AddressFamily RequestedAddressFamily::family() const
{
  return mAddressFamily;
}

void RequestedAddressFamily::buildPacket(BufferWriter& stream)
{
  stream.writeUChar(mAddressFamily);
  stream.writeUChar(0); stream.writeUChar(0); stream.writeUChar(0);
}

bool RequestedAddressFamily::parsePacket(BufferReader& stream)
{
  try
  {
    mAddressFamily = (AddressFamily)stream.readUChar();
  }
  catch(...)
  {
    return false;
  }
  return true;
}

void RequestedAddressFamily::dump(std::ostream &output)
{
  output << "RequestedAddressFamily " << (mAddressFamily == IPv4 ? "IPv4" : "IPv6");
}
