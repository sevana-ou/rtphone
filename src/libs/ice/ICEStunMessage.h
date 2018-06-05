/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_STUN_MESSAGE_H
#define __ICE_STUN_MESSAGE_H

#include <string>
#include <map>
#include "ICEByteBuffer.h"

namespace ice
{
  class StunAttribute
  {
  public:
    enum Type
    {
      NotExist              = 0,
      MappedAddress         = 1,
      Username              = 6,
      MessageIntegrity      = 8,
      ErrorCode             = 9,
      UnknownAttributes     = 10,
      Realm                 = 20,
      Nonce                 = 21,
      XorMappedAddress      = 32,
      Server                = 0x8022,
      AlternateServer       = 0x8023,
      Fingerprint           = 0x8028,
      
      // TURN
      ChannelNumber         = 0x0c,
      Lifetime              = 0x0d,
      XorPeerAddress        = 0x12,
      Data                  = 0x13,
      XorRelayedAddress     = 0x16,
      RequestedAddressFamily = 0x17,
      RequestedTransport    = 0x19,
      ReservedToken         = 0x22,

      // ICE
      ControlledAttr         = 0x8029,
      ControllingAttr        = 0x802a,
      ICEPriority           = 0x0024,
      ICEUseCandidate       = 0x0025
    };

    StunAttribute();
    virtual ~StunAttribute();
    
    virtual int   type() const = 0;
    virtual void  buildPacket(BufferWriter& writer) = 0;
    virtual bool  parsePacket(BufferReader& reader) = 0;

    void          setDataOffset(size_t offset);
    size_t        dataOffset() const;
    virtual void  dump(std::ostream& output) = 0;

  protected:
    int mType;
    int mLength;
    size_t mDataOffset;
  };
  
  
  class Username;
  class Realm;
  class Nonce;
  class ErrorCode;
  class MessageIntegrity;
  class MappedAddress;
  class XorMappedAddress;
  class ControlledAttr;
  class ControllingAttr;
  class ICEPriority;
  class Lifetime;
  class XorRelayedAddress;
  class ChannelNumber;
  class XorPeerAddress;

  class StunMessage
  {
  public:
    struct TransactionID
    {
      unsigned char mValue[12];

      bool operator == (const TransactionID& rhs);
      bool operator != (const TransactionID& rhs);
      
      TransactionID();
      std::string toStdString();

      static TransactionID generateNew();
    };

    enum Class
    {
      RequestClass = 0,       //0b00,
      IndicationClass = 1,    //0b01,
      SuccessClass = 2,       //0b10,
      ErrorClass = 3          //0b11
    };

    enum Type
    {
      Binding =           1,
      Allocate =          3,
      Refresh =           4,
      Send =              6,
      Data =              7,
      CreatePermission =  8,
      ChannelBind =       9
    };

    StunMessage();
    virtual ~StunMessage();

    std::string       comment();
    void              setComment(std::string comment);

    void              setMessageType(Type type);
    Type              messageType();
  
    void              setMessageClass(Class value);
    Class             messageClass();

    unsigned int      magicCookie();
    bool              isMagicCookieValid();
    
    void              setTransactionId(TransactionID id);
    TransactionID     transactionId();

    // Builds STUN packet using specified password (if MessageIntegrity attribute is add before)
    void              buildPacket(ByteBuffer& buffer, const std::string& password);
    
    // Parses STUN packet but do not validates it
    bool              parsePacket(ByteBuffer& buffer);

    // Validate parsed packet. Must be called right after ParsePacket() method.
    // Returns true if validated ok, false otherwise.
    bool validatePacket(std::string key);
    
    // Adds attribute to STUN message. If attribute of the same type exists - new one will be add.
    void addAttribute(StunAttribute* attr);
    
    // Sets attribute to STUN message. If attribute of the same type exists - it will be overwritten.
    void setAttribute(StunAttribute* attr);
    
    // Checks if there is specified attribute
    bool hasAttribute(int attrType) const;

    // Gets reference to attribute object
    StunAttribute& attribute(int attrType);
  
    const StunAttribute& operator [] (int attribute) const;
    StunAttribute& operator[] (int attribute);
    
    Username&               usernameAttr();
    Realm&                  realmAttr();
    ErrorCode&              errorCodeAttr();
    Nonce&                  nonceAttr();
    MessageIntegrity&       messageIntegrityAttr();
    MappedAddress&          mappedAddressAttr();
    XorMappedAddress&       xorMappedAddressAttr();
    ControlledAttr&          iceControlledAttr();
    ControllingAttr&         iceControllingAttr();
    ICEPriority&            icePriorityAttr();
    Lifetime&               lifetimeAttr();
    XorRelayedAddress&      xorRelayedAddressAttr();   
    ChannelNumber&          channelNumberAttr();
    XorPeerAddress&         xorPeerAddressAttr();

    void dump(std::ostream& output);

  protected:
    unsigned int                  mMagicCookie;       //  STUN magic cookie predefined value
    Type                          mType;              //  Type of STUN message
    Class                         mClass;             //  Class of STUN message
    TransactionID                 mTransactionID;     //  Transaction ID of STUN message
    
    typedef std::multimap<int, StunAttribute*> AttributeMap;

    mutable AttributeMap          mAttrMap;           //  Attribute list
    ByteBuffer                 mPacket2Parse;      //  Incoming packet
    ByteBuffer                 mPacket2Send;       //  Outgoing packet

    std::string                   mComment;           //  Comment for this message

    //Helper method to get bit with specified index from 'value' parameter.
    inline char bit(unsigned int value, size_t index);

    //Helper method to set bit in specified 'result' parameter
    inline void setBit(unsigned int& result, size_t index, char value);
  };
}

#endif
