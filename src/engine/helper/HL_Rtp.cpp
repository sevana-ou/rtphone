/* Copyright(C) 2007-2026 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if defined(TARGET_WIN)
# include <WinSock2.h>
# include <Windows.h>
#endif

#if defined(TARGET_LINUX) || defined(TARGET_ANDROID) || defined(TARGET_OSX)
# include <arpa/inet.h>
#endif

#include "HL_Rtp.h"
#include "HL_Exception.h"
#include "HL_Log.h"

#include "jrtplib/src/rtprawpacket.h"
#include "jrtplib/src/rtpipv4address.h"

#include <stdexcept>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <chrono>

#define LOG_SUBSYSTEM "RtpDump"

static constexpr size_t MAX_RTP_PACKET_SIZE = 65535;
static const char RTPDUMP_SHEBANG[] = "#!rtpplay1.0";

// RTP fixed header (little-endian bit-field layout)
struct RtpHeader
{
    unsigned char cc:4;         /* CSRC count             */
    unsigned char x:1;          /* header extension flag  */
    unsigned char p:1;          /* padding flag           */
    unsigned char version:2;    /* protocol version       */
    unsigned char pt:7;         /* payload type           */
    unsigned char m:1;          /* marker bit             */
    unsigned short seq;         /* sequence number        */
    unsigned int ts;            /* timestamp              */
    unsigned int ssrc;          /* synchronization source */
};

struct RtcpHeader
{
    unsigned char rc:5;         /* reception report count */
    unsigned char p:1;          /* padding flag           */
    unsigned char version:2;    /* protocol version       */
    unsigned char pt;           /* payload type           */
    uint16_t len;               /* length                 */
    uint32_t ssrc;              /* synchronization source */
};

// --- IPv4 address helpers ---

static std::string ipToString(uint32_t ip)
{
    // ip in host byte order → dotted-decimal
    return std::to_string((ip >> 24) & 0xFF) + "." +
           std::to_string((ip >> 16) & 0xFF) + "." +
           std::to_string((ip >>  8) & 0xFF) + "." +
           std::to_string( ip        & 0xFF);
}

static uint32_t stringToIp(const std::string& s)
{
    unsigned a = 0, b = 0, c = 0, d = 0;
    if (std::sscanf(s.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4)
        return 0;
    if (a > 255 || b > 255 || c > 255 || d > 255)
        return 0;
    return (a << 24) | (b << 16) | (c << 8) | d;
}

// --- RtpHelper implementation ---

bool RtpHelper::isRtp(const void* buffer, size_t length)
{
    if (length < 12)
        return false;

    const RtpHeader* h = reinterpret_cast<const RtpHeader*>(buffer);
    if (h->version != 2)
        return false;

    unsigned char pt = h->pt;
    bool rtp = (pt >= 96 && pt <= 127) || (pt < 35);
    return rtp;
}

bool RtpHelper::isRtpOrRtcp(const void* buffer, size_t length)
{
    if (length < 12)
        return false;
    const RtcpHeader* h = reinterpret_cast<const RtcpHeader*>(buffer);
    return h->version == 2;
}

bool RtpHelper::isRtcp(const void* buffer, size_t length)
{
    return (isRtpOrRtcp(buffer, length) && !isRtp(buffer, length));
}

unsigned RtpHelper::findSsrc(const void* buffer, size_t length)
{
    if (isRtp(buffer, length))
        return ntohl(reinterpret_cast<const RtpHeader*>(buffer)->ssrc);
    else if (isRtpOrRtcp(buffer, length))
        return ntohl(reinterpret_cast<const RtcpHeader*>(buffer)->ssrc);
    return 0;
}

void RtpHelper::setSsrc(void* buffer, size_t length, uint32_t ssrc)
{
    if (isRtp(buffer, length))
        reinterpret_cast<RtpHeader*>(buffer)->ssrc = htonl(ssrc);
    else if (isRtpOrRtcp(buffer, length))
        reinterpret_cast<RtcpHeader*>(buffer)->ssrc = htonl(ssrc);
}

int RtpHelper::findPtype(const void* buffer, size_t length)
{
    if (isRtp(buffer, length))
        return reinterpret_cast<const RtpHeader*>(buffer)->pt;
    else
        return -1;
}

int RtpHelper::findPacketNo(const void *buffer, size_t length)
{
    if (isRtp(buffer, length))
        return ntohs(reinterpret_cast<const RtpHeader*>(buffer)->seq);
    else
        return -1;
}

int RtpHelper::findPayloadLength(const void* buffer, size_t length)
{
    if (!isRtp(buffer, length))
        return -1;

    const RtpHeader* h = reinterpret_cast<const RtpHeader*>(buffer);
    const uint8_t* p = static_cast<const uint8_t*>(buffer);

    // Fixed header (12 bytes) + CSRC list (4 * CC bytes)
    size_t offset = 12 + 4u * h->cc;
    if (offset > length)
        return -1;

    // Header extension
    if (h->x) {
        if (offset + 4 > length)
            return -1;
        uint16_t extWords = (static_cast<uint16_t>(p[offset + 2]) << 8) | p[offset + 3];
        offset += 4 + 4u * extWords;
        if (offset > length)
            return -1;
    }

    size_t payloadLen = length - offset;

    // Padding
    if (h->p && payloadLen > 0) {
        uint8_t padBytes = p[length - 1];
        if (padBytes > payloadLen)
            return -1;
        payloadLen -= padBytes;
    }

    return static_cast<int>(payloadLen);
}

std::chrono::microseconds RtpHelper::toMicroseconds(const jrtplib::RTPTime& t)
{
    return std::chrono::microseconds(uint64_t(t.GetDouble() * 1000000));
}

// --- RtpDump implementation ---

std::shared_ptr<jrtplib::RTPPacket> RtpDump::parseRtpData(const uint8_t* data, size_t len)
{
    if (!data || len < 12 || !RtpHelper::isRtp(data, len))
        return nullptr;

    try {
        // Both are heap-allocated; RTPRawPacket takes ownership and deletes them
        auto* addr = new jrtplib::RTPIPv4Address(uint32_t(0), uint16_t(0));
        uint8_t* dataCopy = new uint8_t[len];
        std::memcpy(dataCopy, data, len);

        jrtplib::RTPRawPacket raw(dataCopy, len, addr, jrtplib::RTPTime(0), true);
        auto packet = std::make_shared<jrtplib::RTPPacket>(raw);

        if (packet->GetCreationError() != 0)
            return nullptr;

        return packet;
    } catch (const std::exception& e) {
        ICELogInfo(<< "Failed to parse RTP packet: " << e.what());
        return nullptr;
    }
}

RtpDump::RtpDump(const char* filename)
    : mFilename(filename ? filename : "")
{
}

RtpDump::~RtpDump() = default;

void RtpDump::setSource(uint32_t ip, uint16_t port)
{
    mSourceIp = ip;
    mSourcePort = port;
}

void RtpDump::load()
{
    if (mFilename.empty())
        throw std::runtime_error("No filename specified");

    std::ifstream input(mFilename, std::ios::binary);
    if (!input.is_open())
        throw std::runtime_error("Failed to open RTP dump file: " + mFilename);

    mPacketList.clear();

    // --- 1. Text header: "#!rtpplay1.0 <ip>/<port>\n" ---
    std::string textLine;
    std::getline(input, textLine);
    if (textLine.compare(0, sizeof(RTPDUMP_SHEBANG) - 1, RTPDUMP_SHEBANG) != 0)
        throw std::runtime_error("Invalid rtpdump header: expected " + std::string(RTPDUMP_SHEBANG));

    // Parse source address from the text line
    size_t spacePos = textLine.find(' ');
    if (spacePos != std::string::npos) {
        std::string addrPart = textLine.substr(spacePos + 1);
        size_t slashPos = addrPart.find('/');
        if (slashPos != std::string::npos) {
            mSourceIp = stringToIp(addrPart.substr(0, slashPos));
            try {
                mSourcePort = static_cast<uint16_t>(std::stoi(addrPart.substr(slashPos + 1)));
            } catch (...) {
                mSourcePort = 0;
            }
        }
    }

    // --- 2. Binary file header (RD_hdr_t, 16 bytes) ---
    uint32_t buf32;
    uint16_t buf16;

    input.read(reinterpret_cast<char*>(&buf32), 4);
    mStartSec = ntohl(buf32);

    input.read(reinterpret_cast<char*>(&buf32), 4);
    mStartUsec = ntohl(buf32);

    input.read(reinterpret_cast<char*>(&buf32), 4);  // source IP (already NBO in file)
    // The binary header stores IP in network byte order; convert to host
    mSourceIp = ntohl(buf32);

    input.read(reinterpret_cast<char*>(&buf16), 2);
    mSourcePort = ntohs(buf16);

    input.read(reinterpret_cast<char*>(&buf16), 2);  // padding — discard

    if (!input.good())
        throw std::runtime_error("Failed to read rtpdump binary header");

    // --- 3. Packet records ---
    size_t packetCount = 0;

    while (input.good() && input.peek() != EOF) {
        // Packet header: length(2) + plen(2) + offset(4) = 8 bytes
        uint16_t recLength, plen;
        uint32_t offsetMs;

        input.read(reinterpret_cast<char*>(&recLength), 2);
        if (input.gcount() != 2) break;
        recLength = ntohs(recLength);

        input.read(reinterpret_cast<char*>(&plen), 2);
        if (input.gcount() != 2) break;
        plen = ntohs(plen);

        input.read(reinterpret_cast<char*>(&offsetMs), 4);
        if (input.gcount() != 4) break;
        offsetMs = ntohl(offsetMs);

        // All-zeros record signals end of file in some implementations
        if (recLength == 0 && plen == 0 && offsetMs == 0)
            break;

        if (plen == 0 || plen > MAX_RTP_PACKET_SIZE)
            throw std::runtime_error("Invalid packet payload length: " + std::to_string(plen));

        if (recLength < plen + 8)
            throw std::runtime_error("Record length (" + std::to_string(recLength) +
                                     ") smaller than payload + header (" + std::to_string(plen + 8) + ")");

        // Read body
        std::vector<uint8_t> body(plen);
        input.read(reinterpret_cast<char*>(body.data()), plen);
        if (static_cast<size_t>(input.gcount()) != plen)
            throw std::runtime_error("Incomplete packet data in rtpdump file");

        // Skip any padding between plen and recLength-8
        size_t pad = static_cast<size_t>(recLength) - 8 - plen;
        if (pad > 0)
            input.seekg(static_cast<std::streamoff>(pad), std::ios::cur);

        RtpData entry;
        entry.mRawData = std::move(body);
        entry.mOffsetMs = offsetMs;
        entry.mPacket = parseRtpData(entry.mRawData.data(), entry.mRawData.size());

        mPacketList.push_back(std::move(entry));
        packetCount++;
    }

    ICELogInfo(<< "Loaded " << packetCount << " packets from " << mFilename);
    mLoaded = true;
}

size_t RtpDump::count() const
{
    return mPacketList.size();
}

jrtplib::RTPPacket& RtpDump::packetAt(size_t index)
{
    if (index >= mPacketList.size())
        throw std::out_of_range("Packet index out of range: " + std::to_string(index));

    if (!mPacketList[index].mPacket)
        throw std::runtime_error("No parsed RTP data at index " + std::to_string(index));

    return *mPacketList[index].mPacket;
}

const std::vector<uint8_t>& RtpDump::rawDataAt(size_t index) const
{
    if (index >= mPacketList.size())
        throw std::out_of_range("Packet index out of range: " + std::to_string(index));

    return mPacketList[index].mRawData;
}

uint32_t RtpDump::offsetAt(size_t index) const
{
    if (index >= mPacketList.size())
        throw std::out_of_range("Packet index out of range: " + std::to_string(index));

    return mPacketList[index].mOffsetMs;
}

void RtpDump::add(const void* buffer, size_t len)
{
    if (!buffer || len == 0)
        return;

    uint32_t offsetMs = 0;
    auto now = std::chrono::steady_clock::now();

    if (!mRecording) {
        mRecording = true;
        mRecordStart = now;

        // Capture wall-clock start time
        auto wallNow = std::chrono::system_clock::now();
        auto epoch = wallNow.time_since_epoch();
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(epoch);
        auto usec = std::chrono::duration_cast<std::chrono::microseconds>(epoch - sec);
        mStartSec  = static_cast<uint32_t>(sec.count());
        mStartUsec = static_cast<uint32_t>(usec.count());
    } else {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - mRecordStart);
        offsetMs = static_cast<uint32_t>(elapsed.count());
    }

    add(buffer, len, offsetMs);
}

void RtpDump::add(const void* buffer, size_t len, uint32_t offsetMs)
{
    if (!buffer || len == 0)
        return;

    if (len > MAX_RTP_PACKET_SIZE)
        throw std::runtime_error("Packet too large: " + std::to_string(len));

    RtpData entry;
    entry.mRawData.assign(static_cast<const uint8_t*>(buffer),
                          static_cast<const uint8_t*>(buffer) + len);
    entry.mOffsetMs = offsetMs;
    entry.mPacket = parseRtpData(entry.mRawData.data(), entry.mRawData.size());

    mPacketList.push_back(std::move(entry));
}

void RtpDump::flush()
{
    if (mFilename.empty())
        throw std::runtime_error("No filename specified");

    std::ofstream output(mFilename, std::ios::binary);
    if (!output.is_open())
        throw std::runtime_error("Failed to open file for writing: " + mFilename);

    // --- 1. Text header ---
    std::string textLine = std::string(RTPDUMP_SHEBANG) + " " +
                           ipToString(mSourceIp) + "/" +
                           std::to_string(mSourcePort) + "\n";
    output.write(textLine.data(), static_cast<std::streamsize>(textLine.size()));

    // --- 2. Binary file header (16 bytes) ---
    uint32_t buf32;
    uint16_t buf16;

    buf32 = htonl(mStartSec);
    output.write(reinterpret_cast<const char*>(&buf32), 4);

    buf32 = htonl(mStartUsec);
    output.write(reinterpret_cast<const char*>(&buf32), 4);

    buf32 = htonl(mSourceIp);
    output.write(reinterpret_cast<const char*>(&buf32), 4);

    buf16 = htons(mSourcePort);
    output.write(reinterpret_cast<const char*>(&buf16), 2);

    buf16 = 0;  // padding
    output.write(reinterpret_cast<const char*>(&buf16), 2);

    // --- 3. Packet records ---
    size_t written = 0;

    for (const auto& pkt : mPacketList) {
        if (pkt.mRawData.empty())
            continue;

        uint16_t plen = static_cast<uint16_t>(pkt.mRawData.size());
        uint16_t recLength = static_cast<uint16_t>(plen + 8);

        buf16 = htons(recLength);
        output.write(reinterpret_cast<const char*>(&buf16), 2);

        buf16 = htons(plen);
        output.write(reinterpret_cast<const char*>(&buf16), 2);

        buf32 = htonl(pkt.mOffsetMs);
        output.write(reinterpret_cast<const char*>(&buf32), 4);

        output.write(reinterpret_cast<const char*>(pkt.mRawData.data()), plen);

        written++;
    }

    if (!output.good())
        throw std::runtime_error("Failed to write rtpdump file: " + mFilename);

    ICELogInfo(<< "Wrote " << written << " packets to " << mFilename);
}

void RtpDump::clear()
{
    mPacketList.clear();
    mLoaded = false;
    mRecording = false;
}
