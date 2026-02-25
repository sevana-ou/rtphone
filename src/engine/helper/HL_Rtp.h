/* Copyright(C) 2007-2026 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __HL_RTP_H
#define __HL_RTP_H

#include "jrtplib/src/rtppacket.h"

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

// Class to carry rtp/rtcp socket pair
template<class T>
struct RtpPair
{
    T mRtp;
    T mRtcp;

    RtpPair()
    {}

    RtpPair(const T& rtp, const T& rtcp)
        :mRtp(rtp), mRtcp(rtcp)
    {}

    bool multiplexed() const { return mRtp == mRtcp; }
};

class RtpHelper
{
public:
    static bool     isRtp(const void* buffer, size_t length);
    static int      findPtype(const void* buffer, size_t length);
    static int      findPacketNo(const void *buffer, size_t length);
    static bool     isRtpOrRtcp(const void* buffer, size_t length);
    static bool     isRtcp(const void* buffer, size_t length);
    static unsigned findSsrc(const void* buffer, size_t length);
    static void     setSsrc(void* buffer, size_t length, uint32_t ssrc);
    static int      findPayloadLength(const void* buffer, size_t length);

    static std::chrono::microseconds toMicroseconds(const jrtplib::RTPTime& t);
};

/**
 * @brief Standard rtpdump file format (rtptools / Wireshark compatible)
 *
 * Conforms to the rtpdump format defined by rtptools:
 *   https://formats.kaitai.io/rtpdump/
 *
 * File layout:
 *   1. Text header line:
 *        "#!rtpplay1.0 <source_ip>/<source_port>\n"
 *
 *   2. Binary file header (RD_hdr_t, 16 bytes, all big-endian):
 *        uint32_t start_sec    - recording start time, seconds since epoch
 *        uint32_t start_usec   - recording start time, microseconds
 *        uint32_t source_ip    - source IP address (network byte order)
 *        uint16_t source_port  - source port
 *        uint16_t padding      - always 0
 *
 *   3. Packet records (repeated until EOF):
 *      Per-packet header (RD_packet_t, 8 bytes, all big-endian):
 *        uint16_t length       - total record length (this 8-byte header + plen)
 *        uint16_t plen         - RTP/RTCP payload length in bytes
 *        uint32_t offset       - milliseconds since recording start
 *      Followed by plen bytes of RTP/RTCP packet data.
 *
 * Maximum single packet payload: 65535 bytes (enforced for safety).
 */
class RtpDump
{
protected:
    struct RtpData
    {
        std::shared_ptr<jrtplib::RTPPacket> mPacket;
        std::vector<uint8_t> mRawData;
        uint32_t mOffsetMs = 0;
    };

    typedef std::vector<RtpData> PacketList;
    PacketList mPacketList;
    std::string mFilename;
    bool mLoaded = false;

    // File header fields
    uint32_t mSourceIp = 0;
    uint16_t mSourcePort = 0;
    uint32_t mStartSec = 0;
    uint32_t mStartUsec = 0;

    // Auto-compute packet offsets during recording
    bool mRecording = false;
    std::chrono::steady_clock::time_point mRecordStart;

    std::shared_ptr<jrtplib::RTPPacket> parseRtpData(const uint8_t* data, size_t len);

public:
    explicit RtpDump(const char* filename);
    ~RtpDump();

    /** Set source address for the file header (host byte order). */
    void setSource(uint32_t ip, uint16_t port);
    uint32_t sourceIp() const { return mSourceIp; }
    uint16_t sourcePort() const { return mSourcePort; }

    /**
     * @brief Load packets from an rtpdump file
     * @throws std::runtime_error on file/format error
     */
    void load();
    bool isLoaded() const { return mLoaded; }

    size_t count() const;

    /**
     * @brief Get parsed RTP packet at index
     * @throws std::out_of_range if index is invalid
     * @throws std::runtime_error if packet could not be parsed as RTP
     */
    jrtplib::RTPPacket& packetAt(size_t index);

    /** @brief Get raw packet bytes at index */
    const std::vector<uint8_t>& rawDataAt(size_t index) const;

    /** @brief Get packet time offset in milliseconds */
    uint32_t offsetAt(size_t index) const;

    /** @brief Add a packet; time offset is auto-computed from first add() call */
    void add(const void* data, size_t len);

    /** @brief Add a packet with an explicit millisecond offset */
    void add(const void* data, size_t len, uint32_t offsetMs);

    /**
     * @brief Write all packets to file in rtpdump format
     * @throws std::runtime_error on file error
     */
    void flush();

    void clear();
    const std::string& filename() const { return mFilename; }
};

#endif
