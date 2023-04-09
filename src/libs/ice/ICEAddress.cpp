/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#if defined(__MINGW32__)
#   include <w32api.h>

#   define WINVER                  WindowsVista
#   define _WIN32_WINDOWS          WindowsVista
#   define _WIN32_WINNT            WindowsVista
#endif

#include "ICEPlatform.h"
#include "ICEAddress.h"
#include "ICEError.h"
#include <assert.h>
#include <stdio.h>

#if defined(TARGET_WIN)
# include <WinSock2.h>
# include <Windows.h>
# include <ws2tcpip.h>
#else
# include <netinet/in.h>

# if /*defined(TARGET_LINUX) || */ defined(TARGET_ANDROID)
#  include <linux/in6.h>
# endif
#endif

std::ostream& operator << (std::ostream& s, const ice::NetworkAddress& addr)
{
  s << addr.toStdString().c_str();
  return s;
}

#ifdef TARGET_WIN
std::wostream& operator << (std::wostream& s, const ice::NetworkAddress& addr)
{
  s << addr.toStdWString();
  return s;
}
#endif

using namespace ice;

NetworkAddress NetworkAddress::LoopbackAddress4("127.0.0.1", 1000);
#ifdef TARGET_WIN
static in_addr6 la6 = {0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,1 };
#else
static in6_addr la6 = { { { 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,1 } } };
#endif
NetworkAddress NetworkAddress::LoopbackAddress6(la6, 1000);


NetworkAddress NetworkAddress::parse(const std::string& s)
{
  NetworkAddress result;
  result.mInitialized = !s.empty();
  if (result.mInitialized)
  {
    // Relayed or not
    result.mRelayed = s.find("relayed ") != std::string::npos;
    std::string::size_type ip4Pos = s.find("IPv4"), ip6Pos = s.find("IPv6");

    if (ip4Pos == std::string::npos && ip6Pos == std::string::npos)
    {
      // Parse usual IP[:port] pair
      std::string::size_type cp = s.find(":");
      if (cp == std::string::npos)
        result.setIp(cp);
      else
      {
        result.setIp(s.substr(0, cp));
        result.setPort(atoi(s.substr(cp + 1).c_str()));
      }
    }
    else
    {
      // Family
      result.mAddr4.sin_family = ip4Pos != std::string::npos ? AF_INET : AF_INET6;

      // IP:port
      std::string::size_type familyPos = ip4Pos != std::string::npos ? ip4Pos : ip6Pos;
      std::string addr = s.substr(familyPos + 5);

      // Find IP substring and port
      std::string::size_type colonPos = addr.find_last_of(":");
      if (colonPos != std::string::npos)
      {
        int port = atoi(addr.substr(colonPos+1).c_str());
        result.setPort(port);
        result.setIp(addr.substr(0, colonPos));
      }
    }
  }
  return result;
}

// NetworkAddress NetworkAddress::LoopbackAddress6("0:0:0:0:0:0:0:1", 1000);

NetworkAddress::NetworkAddress()
  :mInitialized(false), mRelayed(false)
{
  memset(&mAddr6, 0, sizeof(mAddr6));
  mAddr4.sin_family = AF_INET;
}

NetworkAddress::NetworkAddress(int stunType)
  :mInitialized(false), mRelayed(false)
{
  memset(&mAddr6, 0, sizeof(mAddr6));
  setStunType(stunType);
}

NetworkAddress::NetworkAddress(const in6_addr& addr6, unsigned short port)
:mInitialized(true), mRelayed(false)
{
  memset(&mAddr6, 0, sizeof(mAddr6));
  mAddr4.sin_family = AF_INET6;
  mAddr6.sin6_addr = addr6;
  mAddr6.sin6_port = htons(port);
}

NetworkAddress::NetworkAddress(const in_addr& addr4, unsigned short port)
:mInitialized(true), mRelayed(false)
{
  memset(&mAddr6, 0, sizeof(mAddr6));
  mAddr4.sin_family = AF_INET;
  mAddr4.sin_addr = addr4;
  mAddr4.sin_port = htons(port);
}

unsigned char NetworkAddress::stunType() const
{
  assert(mInitialized);
  switch (mAddr4.sin_family)
  {
  case AF_INET:
    return 1;

  case AF_INET6:
    return 2;

  default:
    assert(0);
  }
  return -1;
}

void NetworkAddress::setStunType(unsigned char st)
{
  switch (st)
  {
  case 1:
    mAddr4.sin_family = AF_INET;
    break;

  case 2:
    mAddr6.sin6_family = AF_INET6;
    break;

  default:
    assert(0);
  }
}

NetworkAddress::NetworkAddress(const std::string& ip, unsigned short port)
:mInitialized(true), mRelayed(false)
{
  memset(&mAddr6, 0, sizeof(mAddr6));
  setIp(ip);
  if (mAddr4.sin_family == AF_INET || mAddr4.sin_family == AF_INET6)
    setPort(port);
}

NetworkAddress::NetworkAddress(const char *ip, unsigned short port)
  :mInitialized(true), mRelayed(false)
{
  memset(&mAddr6, 0, sizeof(mAddr6));
  setIp(ip);
  if (mAddr4.sin_family == AF_INET || mAddr4.sin_family == AF_INET6)
    setPort(port);
}

NetworkAddress::NetworkAddress(uint32_t ip_4, uint16_t port)
    :mInitialized(true), mRelayed(false)
{
    memset(&mAddr6, 0, sizeof(mAddr6));
    mAddr4.sin_family = AF_INET;
    mAddr4.sin_addr.s_addr = ip_4;
    mAddr4.sin_port = port;
}

NetworkAddress::NetworkAddress(const uint8_t* ip_6, uint16_t port)
    :mInitialized(true), mRelayed(false)
{
    memset(&mAddr6, 0, sizeof(mAddr6));
    mAddr6.sin6_family = AF_INET6;
    memmove(&mAddr6.sin6_addr, ip_6, sizeof(mAddr6.sin6_addr));
    mAddr6.sin6_port = port;
}

NetworkAddress::NetworkAddress(const sockaddr& addr, unsigned addrLen)
:mInitialized(true), mRelayed(false)
{
  switch (addr.sa_family)
  {
    case AF_INET6:
      memset(&mAddr6, 0, sizeof(mAddr6));
      memcpy(&mAddr6, &addr, addrLen);
      break;
      
    case AF_INET:
      memset(&mAddr4, 0, sizeof(mAddr4));
      memcpy(&mAddr4, &addr, addrLen);
      break;
  }
}

NetworkAddress::NetworkAddress(const NetworkAddress& src)
:mInitialized(src.mInitialized), mRelayed(src.mRelayed)
{
  memset(&mAddr6, 0, sizeof(mAddr6));
  if (src.mAddr4.sin_family == AF_INET)
    memcpy(&mAddr4, &src.mAddr4, sizeof mAddr4);
  else
    memcpy(&mAddr6, &src.mAddr6, sizeof mAddr6);
}

NetworkAddress::~NetworkAddress()
{
}

int NetworkAddress::family() const
{
  assert(mInitialized == true);
  
  return this->mAddr4.sin_family;
}

sockaddr* NetworkAddress::genericsockaddr() const
{
  assert(mInitialized == true);
  switch (mAddr4.sin_family)
  {
  case AF_INET:
    return (sockaddr*)&mAddr4;

  case AF_INET6:
    return (sockaddr*)&mAddr6;

  default:
    assert(0);
  }
  return NULL;
}

unsigned NetworkAddress::sockaddrLen() const
{
  assert(mInitialized == true);
  switch (mAddr4.sin_family)
  {
  case AF_INET:
    return sizeof(mAddr4);

  case AF_INET6:
    return sizeof(mAddr6);

  default:
    assert(0);
  }
  return 0;
}

sockaddr_in* NetworkAddress::sockaddr4() const
{
  assert(mInitialized == true);
  switch (mAddr4.sin_family)
  {
  case AF_INET:
    return (sockaddr_in*)&mAddr4;

  default:
    assert(0);
  }
  return NULL;
}

sockaddr_in6* NetworkAddress::sockaddr6() const
{
  assert(mInitialized == true);
  switch (mAddr4.sin_family)
  {
  case AF_INET6:
    return (sockaddr_in6*)&mAddr6;

  default:
    assert(0);
  }
  return NULL;
}

void NetworkAddress::setIp(NetworkAddress ipOnly)
{
  // Save port
  mAddr4.sin_family = ipOnly.genericsockaddr()->sa_family;

  switch (ipOnly.family())
  {
  case AF_INET:
    memcpy(&mAddr4.sin_addr, &ipOnly.sockaddr4()->sin_addr, 4);
    break;

  case AF_INET6:
    memcpy(&mAddr4.sin_addr, &ipOnly.sockaddr6()->sin6_addr, 16);
    break;

  default:
    assert(0);
  }
}

void NetworkAddress::setIp(const std::string& ip)
{
#ifdef TARGET_WIN
  int addrSize = sizeof(mAddr6);
#endif
  
  if (inet_addr(ip.c_str()) != INADDR_NONE)
  {
    mAddr4.sin_family = AF_INET;
    mAddr4.sin_addr.s_addr = inet_addr(ip.c_str());
    if (mAddr4.sin_port)
      mInitialized = true;
  }
  else
  {
    mAddr6.sin6_family = AF_INET6;
#ifdef TARGET_WIN
    if (WSAStringToAddressA((char*)ip.c_str(), AF_INET6, NULL, (sockaddr*)&mAddr6, &addrSize) != 0)
#else
    std::string ip2;
    if (ip.find('[') == 0 && ip.find(']') == ip.length()-1)
      ip2 = ip.substr(1, ip.length()-2);
    else
      ip2 = ip;

    if (!inet_pton(AF_INET6, ip2.c_str(), &mAddr6.sin6_addr))
#endif
    {
      mInitialized = false;
      return;
    }
    
    mAddr6.sin6_family = AF_INET6;
    if (mAddr6.sin6_port)
      mInitialized = true;
  }

}

void NetworkAddress::setIp(unsigned long ip)
{
  mAddr4.sin_family = AF_INET;
  mAddr4.sin_addr.s_addr = ip;

  if (mAddr4.sin_port)
    mInitialized = true;
}

void NetworkAddress::setIp(const in_addr& ip)
{
	//memset(&mAddr4, 0, sizeof mAddr4);
	mAddr4.sin_family = AF_INET;
	mAddr4.sin_addr = ip;
}

void NetworkAddress::setIp(const in6_addr& ip)
{
	mAddr6.sin6_family = AF_INET6;
	mAddr6.sin6_addr = ip;
}
// 10.0.0.0 - 10.255.255.255
// 172.16.0.0 - 172.31.255.255
// 192.168.0.0 - 192.168.255.255

bool NetworkAddress::isSameLAN(const NetworkAddress& a1, const NetworkAddress& a2)
{
  if (a1.family() != a2.family())
		return false;
  if (a1.family() == AF_INET)
	{
		sockaddr_in* s1 = a1.sockaddr4();
		sockaddr_in* s2 = a2.sockaddr4();
#ifdef TARGET_WIN
		unsigned b1_0 = (s1->sin_addr.S_un.S_addr >> 0) & 0xFF;
		unsigned b1_1 = (s1->sin_addr.S_un.S_addr >> 8) & 0xFF;
		unsigned b2_0 = (s2->sin_addr.S_un.S_addr >> 0) & 0xFF;
		unsigned b2_1 = (s2->sin_addr.S_un.S_addr >> 8) & 0xFF;
#else
		unsigned b1_0 = (s1->sin_addr.s_addr >> 0) & 0xFF;
		unsigned b1_1 = (s1->sin_addr.s_addr >> 8) & 0xFF;
		unsigned b2_0 = (s2->sin_addr.s_addr >> 0) & 0xFF;
		unsigned b2_1 = (s2->sin_addr.s_addr >> 8) & 0xFF;
		
		if (b1_0 == b2_0 && b1_0 == 192 &&
				b1_1 == b2_1 && b1_1 == 168)
			return true;
		
		if (b1_0 == b2_0 && b1_0 == 10)
			return true;
		
		if (b1_0 == b2_0 && b1_0 == 172 &&
				b1_1 == b2_1 && (b1_1 < 32))
			return true;
		
#endif
	}
	return false;
}

void NetworkAddress::setPort(unsigned short port)
{
  switch(mAddr4.sin_family)
  {
    case AF_INET:
      mAddr4.sin_port = htons(port);
      mInitialized = true;
      break;

    case AF_INET6:
      mAddr6.sin6_port = htons(port);
      mInitialized = true;
      break;

    default:
      assert(0);
  }
}

std::string NetworkAddress::ip() const
{
  assert(mInitialized == true);

  char resultbuf[159], ip6[161]; resultbuf[0] = 0;

#ifdef TARGET_WIN
  DWORD resultsize = sizeof(resultbuf);
#endif

  switch (mAddr4.sin_family)
  {
  case AF_INET:
    return inet_ntoa(mAddr4.sin_addr);

  case AF_INET6:
#if defined(TARGET_WIN)
    InetNtopA(AF_INET6, (PVOID)&mAddr6.sin6_addr, resultbuf, sizeof(resultbuf));
#else
    inet_ntop(AF_INET6, &mAddr6.sin6_addr, resultbuf, sizeof(resultbuf));
#endif
    sprintf(ip6, "[%s]", resultbuf);
    return ip6;

  default:
    return "";
  }
  return "";
}

unsigned char* NetworkAddress::ipBytes() const
{
  switch (family())
  {
  case AF_INET:
#ifdef TARGET_WIN
    return (unsigned char*)&mAddr4.sin_addr.S_un.S_un_b;
#else
    return (unsigned char*)&mAddr4.sin_addr.s_addr;
#endif
  case AF_INET6:
#ifdef TARGET_WIN
    return (unsigned char*)mAddr6.sin6_addr.u.Byte;
#elif defined(TARGET_OSX) || defined(TARGET_IOS)
    return (unsigned char*)&mAddr6.sin6_addr.__u6_addr.__u6_addr8;
#elif defined(TARGET_OPENWRT) || defined(TARGET_MUSL)
    return (unsigned char*)&mAddr6.sin6_addr.__in6_union.__s6_addr;
#elif defined(TARGET_LINUX)
    return (unsigned char*)&mAddr6.sin6_addr.__in6_u.__u6_addr8;
#elif defined(TARGET_ANDROID)
    return (unsigned char*)&mAddr6.sin6_addr.in6_u.u6_addr8;
#endif
  }
  assert(0);
  return nullptr; // to avoid compiler warning
}

unsigned short NetworkAddress::port() const
{
  assert(mInitialized == true);

  return ntohs(mAddr4.sin_port);
}

std::string NetworkAddress::toStdString() const
{
  if (!mInitialized)
    return "";

  char temp[128];
  sprintf(temp, "%s%s %s:%u", mRelayed ? "relayed " : "", this->mAddr4.sin_family == AF_INET ? "IPv4" : "IPv6", ip().c_str(), (unsigned int)port());
  
  return temp;
}
#ifdef WIN32
std::wstring NetworkAddress::toStdWString() const
{
  if (!mInitialized)
    return L"";

  wchar_t temp[128];
  swprintf(temp, L"%s%s %S:%u", mRelayed ? L"relayed " : L"", this->mAddr4.sin_family == AF_INET ? L"IPv4" : L"IPv6", ip().c_str(), (unsigned int)port());

  return temp;
}
#endif

static const unsigned char localhost6[] =
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

bool NetworkAddress::isLoopback() const
{
  switch (mAddr4.sin_family)
  {
  case AF_INET:
    return (mAddr4.sin_addr.s_addr == htonl(INADDR_LOOPBACK));

  case AF_INET6:
    return memcmp(localhost6, &mAddr6.sin6_addr, sizeof mAddr6.sin6_addr) == 0;

  default:
    assert(0);
  }
  return false;
}

bool NetworkAddress::isIp(const std::string& str)
{
  if (inet_addr(str.c_str()) != INADDR_NONE)
    return true;
  const char* address = str.c_str();
  std::string temp;
  if (str.find('[') == 0 && str.find(']') == str.length()-1)
  {
    temp = str.substr(1, str.length()-2);
    address = temp.c_str();
  }
  sockaddr_in6 addr6;
  addr6.sin6_family = AF_INET6;

#ifdef _WIN32
  int addrSize = sizeof(addr6);
  if (WSAStringToAddressA((char*)address, AF_INET6, NULL, (sockaddr*)&addr6, &addrSize) != 0)
#else
  if (!inet_pton(AF_INET6, address, &addr6))
#endif
    return false;
  return true;
}

bool NetworkAddress::isLAN() const
{
  assert(mInitialized);

  switch (mAddr4.sin_family)
  {
    case AF_INET:
    {
      unsigned char b1 = mAddr4.sin_addr.s_addr & 0xFF;
      unsigned char b2 = (mAddr4.sin_addr.s_addr >> 8) & 0xFF;
      if (b1 == 192 && b2 == 168)
        return true;
      if (b1 == 10)
        return true;
      if (b1 == 172 && (b2 >= 16 && b2 <= 31))
        return true;
      
      // Link local addresses are not routable so report them here
      if (b1 == 169 && b2 == 254)
        return true;
      
      return false;
      
    }

  case AF_INET6:
      return false;

  default:
    assert(0);
  }
  return false;
}

bool NetworkAddress::isLinkLocal() const
{
  assert(mInitialized);

  switch (mAddr4.sin_family)
  {
  case AF_INET:
    {
      unsigned char b1 = mAddr4.sin_addr.s_addr & 0xFF;
      unsigned char b2 = (mAddr4.sin_addr.s_addr >> 8) & 0xFF;

      // Link local addresses are not routable so report them here
      if (b1 == 169 && b2 == 254)
        return true;

      //if (b1 == 100 && (b2 >= 64 && b2 <= 127))
      //  return true;
      
      return false;

    }

  case AF_INET6:
#ifdef WIN32
	return (mAddr6.sin6_addr.u.Byte[0] == 0xFE && mAddr6.sin6_addr.u.Byte[1] == 0x80);
#else
    return IN6_IS_ADDR_LINKLOCAL(&mAddr6.sin6_addr);
#endif
  default:
    assert(0);
  }
  return false;
}

void NetworkAddress::clear()
{
  memset(&mAddr6, 0, sizeof(mAddr6));
  mInitialized = false;
}

bool NetworkAddress::isEmpty() const
{
  return !mInitialized;
}

bool NetworkAddress::isZero() const
{
  if (!mInitialized)
    return false;
  switch (mAddr4.sin_family)
  {
    case AF_INET:
      return mAddr4.sin_addr.s_addr == 0;
      
    case AF_INET6:
#ifdef WIN32
      return !mAddr6.sin6_addr.u.Word[0] && !mAddr6.sin6_addr.u.Word[1] &&
        !mAddr6.sin6_addr.u.Word[2] && !mAddr6.sin6_addr.u.Word[3] &&
        !mAddr6.sin6_addr.u.Word[4] && !mAddr6.sin6_addr.u.Word[5] &&
        !mAddr6.sin6_addr.u.Word[6] && !mAddr6.sin6_addr.u.Word[7];
#else
      return IN6_IS_ADDR_UNSPECIFIED(&mAddr6.sin6_addr);
#endif
  }
  return false;
}

bool NetworkAddress::isV4() const
{
  return family() == AF_INET;
}

bool NetworkAddress::isV6() const
{
  return family() == AF_INET6;
}

bool NetworkAddress::operator == (const NetworkAddress& rhs) const
{
  return NetworkAddress::isSame(*this, rhs);
}

bool NetworkAddress::operator != (const NetworkAddress& rhs) const
{
  return !NetworkAddress::isSame(*this, rhs);
}

bool NetworkAddress::operator < (const NetworkAddress& rhs) const
{
  if (family() != rhs.family())
    return family() < rhs.family();

  if (port() != rhs.port())
    return port() < rhs.port();

  switch (family())
  {
  case AF_INET:
    return memcmp(sockaddr4(), rhs.sockaddr4(), sizeof(sockaddr_in)) < 0;

  case AF_INET6:
    return memcmp(sockaddr6(), rhs.sockaddr6(), sizeof(sockaddr_in6)) < 0;
  }

  return false;
}

bool NetworkAddress::operator > (const NetworkAddress& rhs) const
{
  if (family() != rhs.family())
    return family() > rhs.family();

  if (port() != rhs.port())
    return port() > rhs.port();

  switch (family())
  {
  case AF_INET:
    return memcmp(sockaddr4(), rhs.sockaddr4(), sizeof(sockaddr_in)) > 0;

  case AF_INET6:
    return memcmp(sockaddr6(), rhs.sockaddr6(), sizeof(sockaddr_in6)) > 0;
  }

  return false;
}

bool NetworkAddress::isPublic() const
{
  return !isLAN() && !isLinkLocal() && !isLoopback() && !isEmpty();
}

void NetworkAddress::setRelayed(bool relayed)
{
  mRelayed = relayed;
}
bool NetworkAddress::relayed() const
{
  return mRelayed;
}

bool NetworkAddress::hasHost(const std::vector<NetworkAddress>& hosts)
{
  for (unsigned i=0; i<hosts.size(); i++)
  {
    const NetworkAddress& a = hosts[i];
    if (mAddr4.sin_family != a.mAddr4.sin_family)
      continue;

    switch (mAddr4.sin_family)
    {
    case AF_INET:
      if (!memcmp(&mAddr4.sin_addr, &a.mAddr4.sin_addr, sizeof(mAddr4.sin_addr)))
        return true;
      break;

    case AF_INET6:
      if (!memcmp(&mAddr6.sin6_addr, &a.mAddr6.sin6_addr, sizeof(mAddr6.sin6_addr)))
        return true;
      break;
    }
  }
  return false;
}

bool NetworkAddress::isSameHost(const NetworkAddress& a1, const NetworkAddress& a2)
{
  return (a1.ip() == a2.ip());
}

bool NetworkAddress::isSame(const NetworkAddress& a1, const NetworkAddress& a2)
{
  if (!a1.mInitialized || !a2.mInitialized)
    return false;
  
  // Compare address families
  if (a1.mAddr4.sin_family != a2.mAddr4.sin_family)
    return false;
	
  if (a1.mRelayed != a2.mRelayed)
    return false;
  
  switch (a1.mAddr4.sin_family)
  {
    case AF_INET:
      return a1.mAddr4.sin_addr.s_addr == a2.mAddr4.sin_addr.s_addr && a1.mAddr4.sin_port == a2.mAddr4.sin_port;
      
    case AF_INET6:
      return memcmp(&a1.mAddr6.sin6_addr, &a2.mAddr6.sin6_addr, sizeof(a1.mAddr6.sin6_addr)) == 0 &&
             a1.mAddr6.sin6_port == a2.mAddr6.sin6_port;
      
    default:
      assert(0);
  }
  return false;
}

NetworkAddress& NetworkAddress::operator = (const NetworkAddress& src)
{
    this->mInitialized = src.mInitialized;
    this->mRelayed = src.mRelayed;
    this->mAddr6 = src.mAddr6;

    return *this;
}
