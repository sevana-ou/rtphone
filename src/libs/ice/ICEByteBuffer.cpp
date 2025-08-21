/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define NOMINMAX

#include "ICEPlatform.h"
#include "ICEByteBuffer.h"
#include "ICEError.h"
#include <algorithm>
#include <assert.h>

using namespace ice;
ByteBuffer::ByteBuffer()
{
    initEmpty();
    mData.reserve(512);
}

ByteBuffer::ByteBuffer(size_t initialCapacity)
{
    initEmpty();
    mData.reserve(initialCapacity);
}

ByteBuffer::ByteBuffer(const ByteBuffer& src)
    :mData(src.mData), mComponent(src.mComponent), mTag(nullptr),
      mRelayed(src.mRelayed), mCopyBehavior(src.mCopyBehavior),
      mDataPtr(src.mDataPtr), mDataSize(src.mDataSize)
{
    if (mCopyBehavior == CopyBehavior::CopyMemory && mData.size())
        mDataPtr = &mData[0];
}

ByteBuffer::ByteBuffer(const void* packetPtr, size_t packetSize, CopyBehavior behavior)
    :mComponent(-1), mTag(nullptr), mRelayed(false), mCopyBehavior(behavior), mDataPtr(nullptr), mDataSize(packetSize)
{
    switch (behavior)
    {
    case CopyBehavior::CopyMemory:
        mData.resize(packetSize);
        memcpy(&mData[0], packetPtr, packetSize);
        mDataPtr = &mData[0];
        break;

    case CopyBehavior::UseExternal:
        mDataPtr = reinterpret_cast<uint8_t*>(const_cast<void*>(packetPtr));
        break;

    default:
        break;
    }
}

ByteBuffer::ByteBuffer(const std::span<const uint8_t>& packet, CopyBehavior behavior)
    : ByteBuffer(packet.data(), packet.size(), behavior)
{}

ByteBuffer::~ByteBuffer()
{
    if (mCopyBehavior == CopyBehavior::CopyMemory)
        memset(mDataPtr, 0, mDataSize);
}

ByteBuffer& ByteBuffer::operator = (const ByteBuffer& src)
{
    mRelayed = src.mRelayed;
    mComment = src.mComment;
    mComponent = src.mComponent;
    mRemoteAddress = src.mRemoteAddress;
    mTag = src.mTag;

    if (src.mCopyBehavior == CopyBehavior::CopyMemory)
    {
        mData = src.mData;
        mCopyBehavior = CopyBehavior::CopyMemory;
        syncPointer();
    }
    else
    {
        mDataPtr = src.mDataPtr;
        mDataSize = src.mDataSize;
        mCopyBehavior = CopyBehavior::UseExternal;
    }

    return *this;
}

void ByteBuffer::clear()
{
    mData.resize(0);
    mDataSize = 0;
    mDataPtr = nullptr;
}

size_t    ByteBuffer::size() const
{
    return mDataSize;
}

const uint8_t* ByteBuffer::data() const
{
    return reinterpret_cast<const uint8_t*>(mDataPtr);
}

uint8_t* ByteBuffer::mutableData()
{
    return mDataPtr;
}

std::span<const uint8_t> ByteBuffer::span()
{
    return {mDataPtr, mDataSize};
}

NetworkAddress& ByteBuffer::remoteAddress()
{
    return mRemoteAddress;
}

void ByteBuffer::setRemoteAddress(const NetworkAddress& addr)
{
    mRemoteAddress = addr;
}

void ByteBuffer::setComment(std::string comment)
{
    mComment = comment;
}

std::string ByteBuffer::comment()
{
    return mComment;
}

std::string ByteBuffer::hexstring()
{
    return toHex(mDataPtr, mDataSize);
}

std::string ByteBuffer::toHex(const void* bytes, size_t len)
{
    std::string result;
    result.resize(len * 2, (char)0xCC);
    for (std::vector<uint8_t>::size_type index = 0; index < len; index++)
    {
        char value[3];
        sprintf(value, "%02X", (unsigned char)((const unsigned char*)bytes)[index]);
        result[index*2] = value[0];
        result[index*2+1] = value[1];
    }
    return result;
}

void ByteBuffer::reserve(size_t capacity)
{
    mData.reserve(capacity);
    syncPointer();
}

void ByteBuffer::insertTurnPrefix(unsigned short prefix)
{
    assert(mCopyBehavior == CopyBehavior::CopyMemory);

    mData.insert(mData.begin(), 2, 32);
    unsigned short nprefix = htons(prefix);
    mData[0] = nprefix & 0xFF;
    mData[1] = (nprefix & 0xFF00) >> 8;
    syncPointer();
}

int  ByteBuffer::component()
{
    return mComponent;
}

void ByteBuffer::setComponent(int component)
{
    mComponent = component;
}

void ByteBuffer::truncate(size_t newsize)
{
    assert (mCopyBehavior == CopyBehavior::CopyMemory);

    mData.erase(mData.begin() + newsize, mData.end());
    syncPointer();
}

void ByteBuffer::erase(size_t p, size_t l)
{
    assert (mCopyBehavior == CopyBehavior::CopyMemory);

    mData.erase(mData.begin() + p, mData.begin() + p + l);
    syncPointer();
}

void ByteBuffer::resize(size_t newsize)
{
    assert (mCopyBehavior == CopyBehavior::CopyMemory);

    std::vector<uint8_t>::size_type sz = mData.size();
    mData.resize(newsize);
    if (newsize > sz)
        memset(&mData[sz], 0, newsize - sz);

    syncPointer();
}

void ByteBuffer::appendBuffer(const void *data, size_t size)
{
    assert (mCopyBehavior == CopyBehavior::CopyMemory);

    size_t len = mData.size();
    mData.resize(len + size);
    memmove(mData.data() + len, data, size);
    syncPointer();
}

void* ByteBuffer::tag()
{
    return mTag;
}

void  ByteBuffer::setTag(void* tag)
{
    mTag = tag;
}

bool ByteBuffer::relayed()
{
    return mRelayed;
}

void ByteBuffer::setRelayed(bool value)
{
    mRelayed = value;
}

void ByteBuffer::initEmpty()
{
    mDataPtr = nullptr;
    mDataSize = 0;
    mCopyBehavior = CopyBehavior::CopyMemory;
    mRelayed = false;
    mComponent = -1;
    mTag = nullptr;
}

void ByteBuffer::syncPointer()
{
    mDataPtr = mData.empty() ? nullptr : mData.data();
    mDataSize = mData.size();
}

uint8_t ByteBuffer::operator[](int index) const
{
    return mDataPtr[index];
}

uint8_t& ByteBuffer::operator[](int index)
{
    return mDataPtr[index];
}

// ----------------- BitReader -------------------
BitReader::BitReader(const ByteBuffer &buffer)
    :mStream(buffer.data()), mStreamLen(buffer.size()), mStreamOffset(0), mCurrentBit(0)
{
    init();
}

BitReader::BitReader(const void *input, size_t bytes)
    :mStream(reinterpret_cast<const uint8_t*>(input)), mStreamLen(bytes), mStreamOffset(0), mCurrentBit(0)
{
    init();
}

void BitReader::init()
{
    mStreamOffset = mStreamLen << 3;
    mCurrentPosition = 0;//mStreamOffset - 1;
    mCurrentBit = 0;
}

// Check for valid position
uint8_t BitReader::readBit()
{
    uint8_t value = 0x00;
    if (mCurrentPosition < mStreamOffset)
    {
        // Read single BIT
        size_t currentByte = mCurrentPosition >> 3;
        uint8_t currentBit = (uint8_t)(mCurrentPosition % 8);
        value = ((uint8_t)(mStream[currentByte] << currentBit) >> 7);
        mCurrentPosition = std::max((size_t)0, std::min(mStreamOffset-1, mCurrentPosition+1));
    }

    return value;
}

uint32_t BitReader::readBits(size_t nrOfBits)
{
    assert (nrOfBits <= 32);

    uint32_t result = 0;
    BitWriter bw(&result);
    for (int i=0; i<static_cast<int>(nrOfBits); i++)
        bw.writeBit(readBit());

    return result;
}

size_t BitReader::readBits(void *output, size_t nrOfBits)
{
    // Check how much bits available
    nrOfBits = std::min(nrOfBits, mStreamOffset - mCurrentPosition);

    BitWriter bw(output);
    for (int i=0; i<static_cast<int>(nrOfBits); i++)
        bw.writeBit(readBit());

    return nrOfBits;
}

size_t BitReader::count() const
{
    return mStreamOffset;
}

size_t BitReader::position() const
{
    return mCurrentPosition;
}

// ----------------- BitWriter -------------------
BitWriter::BitWriter(ByteBuffer &buffer)
    :mStream(buffer.mutableData()), mStreamLen(0), mStreamOffset(0), mCurrentBit(0)
{
    init();
}

BitWriter::BitWriter(void *output)
    :mStream((uint8_t*)output), mStreamLen(0), mStreamOffset(0), mCurrentBit(0)
{
    init();
}

void BitWriter::init()
{
    mStreamOffset = mStreamLen << 3;
    mCurrentPosition = 0;//mStreamOffset - 1;
    mCurrentBit = 0;
}

BitWriter& BitWriter::writeBit(int bit)
{
    bit = bit ? 1 : 0;

    // Check for current bit offset
    if ((mCurrentBit % 8) == 0)
    {
        // Write new zero byte to the end of stream
        mCurrentBit = 0;
        mStreamLen++;
        mStream[mStreamLen-1] = 0;
    }

    // Write single BIT
    mStream[mStreamLen-1] <<= 1;
    mStream[mStreamLen-1] |= bit;
    mStreamOffset++;
    mCurrentPosition = mStreamOffset - 1;
    mCurrentBit++;

    return *this;
}

size_t BitWriter::count() const
{
    return mStreamOffset;
}

// ----------------- BufferReader ----------------
BufferReader::BufferReader(const ByteBuffer &buffer)
    :mData(buffer.data()), mSize(buffer.size()), mIndex(0)
{}

BufferReader::BufferReader(const void *input, size_t bytes)
    :mData(reinterpret_cast<const uint8_t*>(input)), mSize(bytes), mIndex(0)
{}


uint32_t BufferReader::readUInt()
{
    uint32_t nresult = 0;
    readBuffer(&nresult, 4);

    return ntohl(nresult);
}

uint32_t BufferReader::readNativeUInt()
{
    uint32_t nresult = 0;
    readBuffer(&nresult, 4);

    return nresult;
}

uint16_t BufferReader::readUShort()
{
    uint16_t result = 0;
    readBuffer(&result, 2);

    return ntohs(result);
}

uint16_t BufferReader::readNativeUShort()
{
    uint16_t result = 0;
    readBuffer(&result, 2);

    return result;
}

uint8_t BufferReader::readUChar()
{
    uint8_t result = 0;
    readBuffer(&result, 1);

    return result;
}

NetworkAddress BufferReader::readIp(int family)
{
    if (family == AF_INET)
    {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = 0;
        readBuffer(&addr.sin_addr.s_addr, 4);

        return NetworkAddress(reinterpret_cast<sockaddr&>(addr), sizeof(addr));
    }
    else
        if (family == AF_INET6)
        {
            sockaddr_in6 addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin6_family = AF_INET6;
            addr.sin6_port = 0;
            readBuffer(&addr.sin6_addr, 16);

            return NetworkAddress(reinterpret_cast<sockaddr&>(addr), sizeof(addr));
        }
    return NetworkAddress();
}

size_t BufferReader::readBuffer(void* dataPtr, size_t dataSize)
{
    if (dataSize > 0)
    {
        size_t available = mSize - mIndex;
        if (available < dataSize)
            dataSize = available;
        if (nullptr != dataPtr)
            memcpy(dataPtr, mData + mIndex, dataSize);

        mIndex += dataSize;
        return dataSize;
    }
    return 0;
}

size_t BufferReader::readBuffer(ByteBuffer& bb, size_t dataSize)
{
    if (dataSize > 0)
    {
        // Find how much data are available in fact
        size_t available = mSize - mIndex;
        if (available < dataSize)
            dataSize = available;

        // Extend byte buffer
        size_t startIndex = bb.size();
        bb.resize(bb.size() + dataSize);
        memcpy(bb.mutableData() + startIndex, mData + mIndex, dataSize);
        mIndex += dataSize;
        return dataSize;
    }
    return 0;
}

size_t BufferReader::count() const
{
    return mIndex;
}

// -------------- BufferWriter ----------------------
BufferWriter::BufferWriter(ByteBuffer &buffer)
    :mData(buffer.mutableData()), mIndex(0)
{}

BufferWriter::BufferWriter(void *output)
    :mData(reinterpret_cast<uint8_t*>(output)), mIndex(0)
{}


void BufferWriter::writeUInt(uint32_t value)
{
    // Convert to network order bytes
    uint32_t nvalue = htonl(value);

    writeBuffer(&nvalue, 4);
}


void BufferWriter::writeUShort(uint16_t value)
{
    uint16_t nvalue = htons(value);
    writeBuffer(&nvalue, 2);
}

void BufferWriter::writeUChar(uint8_t value)
{
    writeBuffer(&value, 1);
}

void BufferWriter::writeIp(const NetworkAddress& ip)
{
    switch (ip.stunType())
    {
    case 1/*IPv4*/:
        writeBuffer(&ip.sockaddr4()->sin_addr, 4);
        break;

    case 2/*IPv6*/:
        writeBuffer(&ip.sockaddr6()->sin6_addr, 16);
        break;

    default:
        assert(0);
    }
}

void BufferWriter::writeBuffer(const void* dataPtr, size_t dataSize)
{
    memmove(mData + mIndex, dataPtr, dataSize);
    mIndex += dataSize;
}

void BufferWriter::rewind()
{
    mIndex = 0;
}

void BufferWriter::skip(int count)
{
    mIndex += count;
}

size_t BufferWriter::offset() const
{
    return mIndex;
}
