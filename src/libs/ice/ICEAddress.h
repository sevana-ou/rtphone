/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_ADDRESS_H
#define __ICE_ADDRESS_H

#include "ICEPlatform.h"
#include <ostream>
#include <string>
#include <vector>

namespace ice
{
  class NetworkAddress
  {
  public:
    static NetworkAddress LoopbackAddress4;
    static NetworkAddress LoopbackAddress6;
    static bool isSameLAN(const NetworkAddress& a1, const NetworkAddress& a2);
    static bool isIp(const std::string& str);
    static NetworkAddress parse(const std::string& s);

    NetworkAddress();
    NetworkAddress(int stunType);
    NetworkAddress(const std::string& ip, unsigned short port);
    NetworkAddress(const char* ip, unsigned short port);

    // Both ip_4 and port are network byte order
    NetworkAddress(uint32_t ip_4, uint16_t port);

    // Both ip_6 and port are network byte order
    NetworkAddress(const uint8_t* ip_6, uint16_t port);

    NetworkAddress(const in6_addr& ip, unsigned short port);
    NetworkAddress(const in_addr& ip, unsigned short port);
    NetworkAddress(const sockaddr& addr, size_t addrLen);
    NetworkAddress(const NetworkAddress& src);
    ~NetworkAddress();

    // Returns AF_INET or AF_INET6
    int             family() const;

    unsigned char   stunType() const;
    void            setStunType(unsigned char st);

    sockaddr*       genericsockaddr() const;
    sockaddr_in*    sockaddr4() const;
    sockaddr_in6*   sockaddr6() const;
    unsigned        sockaddrLen() const;
    void            setIp(const std::string& ip);
    void            setIp(unsigned long ip);
    void            setIp(const in_addr& ip);
    void            setIp(const in6_addr& ip);
    void            setIp(NetworkAddress ipOnly);

    void            setPort(unsigned short port);
    std::string     ip() const;
    unsigned char*  ipBytes() const;
    
    unsigned short  port() const;
    std::string     toStdString() const;
#ifdef WIN32
    std::wstring    toStdWString() const;
#endif
    bool            isLoopback() const;
    bool            isLAN() const;
    bool            isLinkLocal() const;
    bool            isPublic() const;
    bool            isEmpty() const;
    bool            isZero() const;
    bool            isV4() const;
    bool            isV6() const;

    void            clear();

    void            setRelayed(bool relayed);
    bool            relayed() const;
    bool            hasHost(const std::vector<NetworkAddress>& hosts);

    static bool isSameHost(const NetworkAddress& a1, const NetworkAddress& a2);
    
    static bool isSame(const NetworkAddress& a1, const NetworkAddress& a2);

    NetworkAddress& operator = (const NetworkAddress& src);

    bool operator == (const NetworkAddress& rhs) const;
    bool operator != (const NetworkAddress& rhs) const;
    bool operator < (const NetworkAddress& rhs) const;
    bool operator > (const NetworkAddress& rhs) const;
  protected:
    union
    {
      sockaddr_in6  mAddr6;
      sockaddr_in   mAddr4;
    };
    bool            mRelayed;
    bool            mInitialized;
  };

}

std::ostream& operator << (std::ostream& s, const ice::NetworkAddress& addr);
#ifdef WIN32
std::wostream& operator << (std::wostream& s, const ice::NetworkAddress& addr);
#endif

#endif
