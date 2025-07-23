/* Copyright(C) 2007-2016 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_BYTE_BUFFER_H
#define __ICE_BYTE_BUFFER_H

#include "ICEPlatform.h"
#include "ICETypes.h"

#include <vector>
#include <string>
#include <memory>
#include <span>
#include "ICETypes.h"
#include "ICEAddress.h"

namespace ice 
{
  class ByteBuffer
  {
  public:
    enum class CopyBehavior
    {
      CopyMemory,
      UseExternal
    };

    ByteBuffer();
    ByteBuffer(size_t initialCapacity);
    ByteBuffer(const ByteBuffer& src);
    ByteBuffer(const void* packetPtr, size_t packetSize, CopyBehavior behavior = CopyBehavior::CopyMemory);
    ByteBuffer(const std::span<const uint8_t>& packet, CopyBehavior behavior = CopyBehavior::CopyMemory);

    ~ByteBuffer();
    
    ByteBuffer& operator = (const ByteBuffer& src);
    
    void          clear();

    size_t        size() const;
    const uint8_t* data() const;
    uint8_t*      mutableData();
    std::span<const uint8_t> span();

    NetworkAddress&   remoteAddress();
    void          setRemoteAddress(const NetworkAddress& addr);
    int           component();
    void          setComponent(int component);
    void          setComment(std::string comment);
    std::string   comment();
    std::string   hexstring();

    static std::string toHex(const void* bytes, size_t len);

    void          reserve(size_t capacity);
    void          insertTurnPrefix(unsigned short prefix);
    void          truncate(size_t newsize);
    void          erase(size_t p, size_t l);
    void          resize(size_t newsize);
    void          appendBuffer(const void* data, size_t size);
    void*         tag();
    void          setTag(void* tag);
    void          trim();
    bool          relayed();
    void          setRelayed(bool value);
    void          syncPointer();

    uint8_t       operator[](int index) const;
    uint8_t&      operator[](int index);


  protected:
    std::vector<uint8_t>        mData;            // Internal storage
    uint8_t*                    mDataPtr;         // Pointer to internal storage or external data
    size_t                      mDataSize;        // Used only for mCopyBehavior == UseExternal
    NetworkAddress              mRemoteAddress;   // Associates buffer with IP address
    int                         mComponent;       // Associates buffer with component ID
    std::string                 mComment;         // Comment's for this buffer - useful in debugging
    void*                       mTag;             // User tag
    bool                        mRelayed;         // Marks if buffer was received via relay
    CopyBehavior                mCopyBehavior;    // Determines if buffer manages internal or external data

    void initEmpty();
    int bitsInBuffer(uint8_t bufferMask);
  };


  typedef std::shared_ptr<ByteBuffer> PByteBuffer;

  class BitReader
  {
  protected:
    const uint8_t* mStream;
    size_t mStreamLen;
    size_t mStreamOffset;
    size_t mCurrentPosition;
    size_t mCurrentBit;

    void init();

  public:
    BitReader(const void* input, size_t bytes);
    BitReader(const ByteBuffer& buffer);
    ~BitReader();

    uint8_t readBit();
    uint32_t readBits(size_t nrOfBits);
    size_t readBits(void* output, size_t nrOfBits);

    size_t count() const;
    size_t position() const;
  };

  class BitWriter
  {
  protected:
    uint8_t* mStream;
    size_t mStreamLen;
    size_t mStreamOffset;
    size_t mCurrentPosition;
    size_t mCurrentBit;

    void init();
  public:
    BitWriter(void* output);
    BitWriter(ByteBuffer& buffer);
    ~BitWriter();

    // Bit must be 0 or 1
    BitWriter& writeBit(int bit);
    size_t count() const;
  };

  class BufferReader
  {
  protected:
    const uint8_t* mData;
    size_t mSize;
    size_t mIndex;

  public:
    BufferReader(const void* input, size_t bytes);
    BufferReader(const ByteBuffer& buffer);

    // This methods reads uint32_t from stream and converts network byte order to host byte order.
    uint32_t      readUInt();

    // This method reads uint32_t from stream. No conversion between byte orders.
    uint32_t      readNativeUInt();

    // This method reads uint16_t from stream and converts network byte order to host byte order.
    uint16_t      readUShort();

    // This method reads uint16_t. No conversion between byte orders.
    uint16_t      readNativeUShort();
    uint8_t       readUChar();

    // Reads in_addr or in6_addr from stream and wraps it to NetworkAddress
    NetworkAddress  readIp(int family);

    // Reads to plain memory buffer
    size_t        readBuffer(void* dataPtr, size_t dataSize);

    // Read directly to byte buffer. New data will be appended to byte buffer
    size_t        readBuffer(ByteBuffer& bb, size_t dataSize);

    size_t count() const;
  };

  class BufferWriter
  {
  protected:
    uint8_t* mData;
    size_t mIndex;

  public:
    BufferWriter(void* output);
    BufferWriter(ByteBuffer& buffer);

    void          writeUInt(uint32_t value);
    void          writeUShort(uint16_t value);
    void          writeUChar(uint8_t value);
    void          writeIp(const NetworkAddress& ip);
    void          writeBuffer(const void* dataPtr, size_t dataSize);
    void          rewind();
    void          skip(int count);

    size_t offset() const;
  };
}
#endif
