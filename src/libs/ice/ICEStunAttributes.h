/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_STUN_ATTRIBUTES_H
#define __ICE_STUN_ATTRIBUTES_H

#include "ICEStunMessage.h"
#include <string>

namespace ice
{
  class MappedAddress: public StunAttribute
  {
  public:
    MappedAddress();
    virtual ~MappedAddress();

    virtual int type() const override;
    NetworkAddress&     address();
    virtual void  buildPacket(BufferWriter& buffer) override;
    virtual bool  parsePacket(BufferReader& buffer) override;
    virtual void  dump(std::ostream& output) override;

  protected:
    NetworkAddress      mAddress;
  };

  class XorMappedAddress: public MappedAddress
  {
  public:
    XorMappedAddress();
    virtual ~XorMappedAddress();
    virtual int  type() const override;
    
    virtual void  buildPacket(BufferWriter& writer) override;
    virtual bool  parsePacket(BufferReader& reader) override;
    virtual void  dump(std::ostream& output) override;
  };
  
  class StringAttr: public StunAttribute
  {
  public:  
    StringAttr();
    virtual ~StringAttr();

    virtual int     type() const override;
    std::string     value() const;
    void            setValue(const std::string& value);

    virtual void    buildPacket(BufferWriter& writer) override;
    virtual bool    parsePacket(BufferReader& reader) override;
    virtual void    dump(std::ostream& output) override;

  protected:
    std::string mValue;
  };
  
  class Username: public StringAttr
  {
  public:  
    Username();
    ~Username();
    int type() const;
    void dump(std::ostream& output);
  };

  class MessageIntegrity: public StunAttribute
  {
  public:
    MessageIntegrity();
    ~MessageIntegrity();

    int             type() const override;
    void            setValue(const void* data);
    const void*     value() const;
  
    void    buildPacket(BufferWriter& stream) override;
    bool    parsePacket(BufferReader& stream) override;
    void    dump(std::ostream& output) override;

  protected:
    unsigned char mValue[20];
  };

  class Fingerprint: public StunAttribute
  {
  public:
    Fingerprint();
    ~Fingerprint();

    int             type() const override;
    void            setCrc32(unsigned int crc);
    unsigned int    crc32() const;

    void    buildPacket(BufferWriter& stream) override;
    bool    parsePacket(BufferReader& stream) override;
    void    dump(std::ostream& output) override;

  protected:
    unsigned int    mCRC32;
  };

  class ErrorCode: public StunAttribute
  {
  public:
    ErrorCode();
    ~ErrorCode();

    virtual int     type() const override;
    void            setErrorCode(int errorCode);
    int             errorCode() const;

    void            setErrorPhrase(const std::string& phrase);
    std::string     errorPhrase() const;
  
    void buildPacket(BufferWriter& stream) override;
    bool parsePacket(BufferReader& stream) override;
    void dump(std::ostream& output) override;

  protected:
    int             mErrorCode;
    std::string     mErrorPhrase;
  };

  class Realm: public StringAttr
  {
  public:
    Realm();
    ~Realm();
    int type() const override;
    void dump(std::ostream& output) override;
  };

  class Nonce: public StringAttr
  {
  public:
    Nonce();
    ~Nonce();
    
    int type() const override;
    void dump(std::ostream& output) override;
  };

  class UnknownAttributes: public StunAttribute
  {
  public:
    UnknownAttributes();
    ~UnknownAttributes();

    int type() const override;
    void    buildPacket(BufferWriter& stream) override;
    bool    parsePacket(BufferReader& stream) override;
    void    dump(std::ostream& output) override;
  };

  class Server: public StringAttr
  {
  public:
    Server();
    ~Server();
    int type() const override;
    void dump(std::ostream& output) override;
  };

  class AlternateServer: public MappedAddress
  {
  public:
    AlternateServer();
    ~AlternateServer();
    int type() const override;
    void dump(std::ostream& output) override;
  };


  class ChannelNumber: public StunAttribute
  {
  public:
    ChannelNumber();
    ~ChannelNumber();
    
    int             type() const override;
    void            setChannelNumber(unsigned short value);
    unsigned short  channelNumber() const;

    void    buildPacket(BufferWriter& stream) override;
    bool    parsePacket(BufferReader& stream) override;
    void    dump(std::ostream& output) override;
  protected:
    unsigned short  mChannelNumber;
  };

  class Lifetime: public StunAttribute
  {
  public:
    Lifetime();
    ~Lifetime();

    int             type() const override;
    void            setLifetime(unsigned int value);
    unsigned int    lifetime() const;

    void    buildPacket(BufferWriter& writer) override;
    bool    parsePacket(BufferReader& reader) override;
    void    dump(std::ostream& output) override;
  protected:
    unsigned int    mLifetime;
  };

  class XorPeerAddress: public XorMappedAddress
  {
  public:
    int type() const override
    {
      return StunAttribute::XorPeerAddress;
    }
  };

  class DataAttribute: public StunAttribute
  {
  public:
    DataAttribute();
    virtual ~DataAttribute();

    int             type() const override;
    void            setData(ByteBuffer& buffer);
    ByteBuffer   data() const;

    void    buildPacket(BufferWriter& stream) override;
    bool    parsePacket(BufferReader& stream) override;
    void    dump(std::ostream &output) override;

  protected:
    ByteBuffer   mData;
  };

  class XorRelayedAddress: public XorMappedAddress
  {
  public:
    XorRelayedAddress();
    ~XorRelayedAddress();
    int     type() const override;
    void    dump(std::ostream& output) override;
  };
  
  class RequestedTransport: public StunAttribute
  {
  public:
    RequestedTransport();
    virtual ~RequestedTransport();
    
    enum TransportType
    {
      UDP = 17
    };

    int             type() const override;
    void            setRequestedTransport(TransportType value);
    unsigned char   requestedTransport() const;

    void    buildPacket(BufferWriter& stream) override;
    bool    parsePacket(BufferReader& stream) override;
    void    dump(std::ostream& output) override;
  protected:
    unsigned char mRequestedTransport; //always 17!
  };

  class ReservedToken: public StunAttribute
  {
  public:
    ReservedToken();
    ~ReservedToken();

    int             type() const override;
    void            setToken(const std::string& token);
    std::string     token() const;

    void    buildPacket(BufferWriter& buffer) override;
    bool    parsePacket(BufferReader& buffer) override;
    void    dump(std::ostream& output) override;
  protected:
    std::string     mToken;
  };

  class ControlledAttr: public StunAttribute
  {
  public:
    ControlledAttr();
    ~ControlledAttr();

    int type() const override;

    std::string     tieBreaker() const;
    void            setTieBreaker(const std::string& tieBreaker);
    void            buildPacket(BufferWriter& stream) override;
    bool            parsePacket(BufferReader& stream) override;
    void            dump(std::ostream& output) override;
  protected:
    unsigned char   mTieBreaker[8];
  };

  class ControllingAttr: public StunAttribute
  {
  public:
    ControllingAttr();
    ~ControllingAttr();

    int             type() const override;
    std::string     tieBreaker() const;
    void            setTieBreaker(const std::string& tieBreaker);

    void    buildPacket(BufferWriter& stream) override;
    bool    parsePacket(BufferReader& stream) override;
    void    dump(std::ostream& output) override;
  protected:
    unsigned char   mTieBreaker[8];
  };

  class ICEPriority: public StunAttribute
  {
  public:
    ICEPriority();
    ~ICEPriority();

    int             type() const override;
    unsigned int    priority() const;
    void            setPriority(unsigned int priority);
    void            buildPacket(BufferWriter& stream) override;
    bool            parsePacket(BufferReader& stream) override;
    void            dump(std::ostream& output) override;
  protected:
    unsigned int    mPriority;
  };

  class ICEUseCandidate: public StunAttribute
  {
  public:
    ICEUseCandidate();
    virtual ~ICEUseCandidate();
    int     type() const override;
    void    buildPacket(BufferWriter& stream) override;
    bool    parsePacket(BufferReader& stream) override;
    void    dump(std::ostream& output) override;
  };

  class RequestedAddressFamily: public StunAttribute
  {
  public:
    RequestedAddressFamily();
    RequestedAddressFamily(AddressFamily family);
    ~RequestedAddressFamily();
    
    int type() const override;
    AddressFamily family() const;
    void buildPacket(BufferWriter& stream) override;
    bool parsePacket(BufferReader& stream) override;
    void dump(std::ostream& output) override;
  protected:
    AddressFamily mAddressFamily;
  };
}
#endif
