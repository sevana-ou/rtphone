/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICENetworkHelper.h"
#include "ICEPlatform.h"
#include "ICELog.h"
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <assert.h>

#ifdef TARGET_WIN
# include <tchar.h>
# if defined(WINDOWS_RT)
#   include "ICEWinRtSupport.h"
# endif
#else
#if defined(TARGET_IOS)
# include "ICEIosSupport.h"
#endif
# include <unistd.h>
# if defined(TARGET_ANDROID)/* || defined(TARGET_LINUX) */
#  include <linux/in6.h>
# endif
#endif

#include "ICEError.h"
using namespace ice;

#define LOG_SUBSYSTEM "ICE"

#if defined(TARGET_WIN) && !defined(WINDOWS_RT)
class IPHlpApi
{
public:
  HMODULE mHandle;
  DWORD (WINAPI *pGetBestInterfaceEx) (struct sockaddr* pDestAddr, PDWORD pdwBestIfIndex);
  DWORD (WINAPI *pGetBestInterface) (IPAddr dwDestAddr, PDWORD pdwBestIfIndex);
  ULONG (WINAPI *pGetAdaptersAddresses) (ULONG Family, ULONG Flags, PVOID Reserved, PIP_ADAPTER_ADDRESSES AdapterAddresses, PULONG SizePointer);

  void ResolveProc(const char* name, FARPROC& proc)
  {
    proc = GetProcAddress(mHandle, name);
  }

public:
  IPHlpApi()
  {
    pGetBestInterfaceEx = NULL;
    pGetBestInterface = NULL;
    pGetAdaptersAddresses = NULL;
    mHandle = ::LoadLibraryW(L"iphlpapi.dll");
    if (mHandle)
    {
      ResolveProc("GetBestInterfaceEx", (FARPROC&)pGetBestInterfaceEx);
      ResolveProc("GetBestInterface", (FARPROC&)pGetBestInterface);
      ResolveProc("GetAdaptersAddresses", (FARPROC&)pGetAdaptersAddresses);
    }
  }

  ~IPHlpApi()
  {
    if (mHandle)
      FreeLibrary(mHandle);
  }

};
IPHlpApi IPHelper;
#endif

NetworkHelper::NetworkHelper()
{
  reload(NetworkType_None);
}

NetworkHelper::~NetworkHelper()
{
}

void NetworkHelper::reload(int networkType)
{
  Lock l(mInstanceGuard);
  
  // Get list of interfaces
#if defined(TARGET_WIN) && !defined(WINDOWS_RT)
  mInterfaceList.clear();
  mIP2IndexList.clear();

  DWORD dwRet, dwSize;
  DWORD flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
  IP_ADAPTER_ADDRESSES* ipAdapters = NULL;
  dwRet = IPHelper.pGetAdaptersAddresses(AF_INET, flags, NULL, NULL, &dwSize);
  if (dwRet == ERROR_BUFFER_OVERFLOW)
  {
    ipAdapters = (IP_ADAPTER_ADDRESSES*)malloc(dwSize);
    if (!ipAdapters)
      return;

    dwRet = IPHelper.pGetAdaptersAddresses(AF_INET, flags, NULL, ipAdapters, &dwSize);
    if (dwRet != ERROR_SUCCESS) 
    {
      free(ipAdapters);
      throw Exception(CANNOT_FIND_INTERFACES);
    }
    processAdaptersList(ipAdapters);
    free(ipAdapters);
  }

#ifdef ICE_IPV6SUPPORT
  dwRet = IPHelper.pGetAdaptersAddresses(AF_INET6, flags, NULL, NULL, &dwSize);
  if (dwRet == ERROR_BUFFER_OVERFLOW)
  {
    ipAdapters = (IP_ADAPTER_ADDRESSES*)malloc(dwSize);
    if (!ipAdapters)
      return;

    dwRet = IPHelper.pGetAdaptersAddresses(AF_INET6, flags, NULL, ipAdapters, &dwSize);
    if (dwRet != ERROR_SUCCESS) 
    {
      free(ipAdapters);
      throw NetworkAddress(CANNOT_FIND_INTERFACES);
    }
    processAdaptersList(ipAdapters);
    free(ipAdapters);
  }
#endif

  for (unsigned i=0; i<mIP2IndexList.size(); i++)
  {
    bool handleIt = true;
    NetworkAddress ip = mIP2IndexList[i].mIP;
#ifndef ICE_LOOPBACK_SUPPORT
    handleIt = !ip.isLoopback();
#endif
#ifdef ICE_SKIP_LINKLOCAL
    handleIt &= !ip.isLinkLocal();
#endif
    if (handleIt)
      mInterfaceList.push_back(ip);
  }
#else
  mIPList.clear();
#if defined(TARGET_OS_IPHONE)
  ICELogDebug(<< "Obtaining IPv4 interfaces.");
  fillIosInterfaceList(AF_INET, networkType, mIPList);
  ICELogDebug(<< "Obtaining IPv6 interfaces.");
  fillIosInterfaceList(AF_INET6, networkType, mIPList);
  for (auto& addr: mIPList)
    ICELogDebug(<< "  " << addr.toStdString());
#elif defined(WINDOWS_RT)
  fillUwpInterfaceList(AF_INET, networkType, mIPList);
  fillUwpInterfaceList(AF_INET6, networkType, mIPList);
#else
  struct ifaddrs* il = NULL;
  if (getifaddrs(&il))
    throw Exception(GETIFADDRS_FAILED, errno);
  if (il)
  {
    struct ifaddrs* current = il;
    while (current)
    {
      //char ipbuffer[64];
      NetworkAddress addr;
      addr.setPort(1000); // Set fake address to keep NetworkAddress initialized
      bool handleIt = true;
      switch(current->ifa_addr->sa_family)
      {
      case AF_INET:
        // just for debug
        // printf("%s", inet_ntoa(reinterpret_cast<sockaddr_in*>(current->ifa_addr)->sin_addr));
        addr.setIp(reinterpret_cast<sockaddr_in*>(current->ifa_addr)->sin_addr);
#ifndef ICE_LOOPBACK_SUPPORT
        handleIt = !addr.isLoopback();
#endif
#ifdef ICE_SKIP_LINKLOCAL
        handleIt &= !addr.isLinkLocal();
#endif
        if (handleIt)
          mIPList.push_back(addr);
        break;

#ifdef ICE_IPV6SUPPORT
      case AF_INET6:
        addr.setIp(reinterpret_cast<sockaddr_in6*>(current->ifa_addr)->sin6_addr);
#ifndef ICE_LOOPBACK_SUPPORT
        if (!addr.isLoopback())
#endif
          mIPList.push_back(addr);
        break;
#endif
      }
      current = current->ifa_next;
    }
    freeifaddrs(il);
  }
#endif
#endif
}

#if defined(TARGET_WIN) && !defined(WINDOWS_RT)
void NetworkHelper::processAdaptersList(IP_ADAPTER_ADDRESSES* addresses)
{
  IP_ADAPTER_ADDRESSES *AI;
  int i;
  for (i = 0, AI = addresses; AI != NULL; AI = AI->Next, i++) 
  {
    for (PIP_ADAPTER_UNICAST_ADDRESS unicast = AI->FirstUnicastAddress; unicast; unicast = unicast->Next)
    {
#ifdef ICE_IPV6SUPPORT
      if (unicast->Address.lpSockaddr->sa_family != AF_INET && unicast->Address.lpSockaddr->sa_family != AF_INET6)
        continue;
#else
      if (unicast->Address.lpSockaddr->sa_family != AF_INET)
        continue;
#endif
      IP2Index rec;
      rec.mIP = NetworkAddress(*unicast->Address.lpSockaddr, unicast->Address.iSockaddrLength);
      rec.mIndex = AI->IfIndex;
      mIP2IndexList.push_back(rec);
    }
  }
}

#endif

Mutex& NetworkHelper::guard()
{
  return mInstanceGuard;
}

std::vector<NetworkAddress>& NetworkHelper::interfaceList()
{
#if defined(TARGET_WIN) && !defined(WINDOWS_RT)
  return mInterfaceList;
#else
  return mIPList;
#endif
}



NetworkAddress NetworkHelper::sourceInterface(const NetworkAddress& remoteIP)
{
  //Lock l(mInstanceGuard);
  
  if (remoteIP.isLoopback())
    return remoteIP.family() == AF_INET ? NetworkAddress::LoopbackAddress4 : NetworkAddress::LoopbackAddress6;

#if defined(TARGET_WIN) && !defined(WINDOWS_RT)
  if (IPHelper.pGetBestInterfaceEx)
  {
    DWORD index = 0;
    DWORD rescode = IPHelper.pGetBestInterfaceEx(remoteIP.genericsockaddr(), &index);
    if (rescode == NO_ERROR)
      return ipInterfaceByIndex(index);
  }
  else
  if (IPHelper.pGetBestInterface)
  {
    DWORD index = 0;
    DWORD rescode = IPHelper.pGetBestInterface(remoteIP.sockaddr4()->sin_addr.S_un.S_addr, &index);
    if (rescode == NO_ERROR)
      return ipInterfaceByIndex(index);
  }
  #ifdef ICE_LOOPBACK_SUPPORT
  return NetworkAddress::LoopbackAddress4;
  #else
  return remoteIP;
  #endif

#elif defined(WINDOWS_RT)
  std::vector<NetworkAddress>& il = interfaceList();
  if (il.size())
    return il.front();
  else
    return NetworkAddress();
#else
  if (remoteIP.isLoopback())
    return remoteIP;

/*#if defined(TARGET_OS_IPHONE) || defined(TARGET_IPHONE_SIMULATOR)
  // There is only one interface - return it
  std::vector<NetworkAddress>& il = interfaceList();
  if (il.size())
  {
    if (!il.front().isZero())
      return il.front();
  }
    
#endif*/
    
  // Here magic goes.
  // 1) Check if remoteIP is IP4 or IP6
  // 2) Check if it is LAN or loopback or internet address
  // 3) For LAN address - try to find interface from the same network
  //    For Loopback address - return loopback address
  //    For internet address - find default interface

	//printf("remote ip %s\n", remoteIP.GetIP().c_str());
  if (remoteIP.isLAN())
  {
    for (unsigned i=0; i<mIPList.size(); i++)
		{
			//printf("local ip %s\n", mIPList[i].GetIP().c_str());
      if (NetworkAddress::isSameLAN(mIPList[i], remoteIP))
        return mIPList[i];
		}
  }
  #ifdef ICE_LOOPBACK_SUPPORT
  NetworkAddress result = remoteIP.type() == AF_INET ? NetworkAddress::LoopbackAddress4 : NetworkAddress::LoopbackAddress6;
  #else
  NetworkAddress result = remoteIP;
  #endif

  // Find default interface - this operation costs, so the result must be cached
  SOCKET s = INVALID_SOCKET;
  int rescode = 0;
  sockaddr_in ipv4;
#ifdef ICE_IPV6SUPPORT
  sockaddr_in6 ipv6;
#endif
  socklen_t addrLen = 0;
  switch (remoteIP.family())
  {
  case AF_INET:
    s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s != INVALID_SOCKET)
    {
      rescode = ::connect(s, remoteIP.genericsockaddr(), remoteIP.sockaddrLen());
      if (rescode != -1)
      {
        addrLen = sizeof(ipv4);
        if (::getsockname(s, (sockaddr*)&ipv4, &addrLen) != -1)
          result = NetworkAddress((sockaddr&)ipv4, addrLen);
      }
      ::close(s);
    }
		break;

#ifdef ICE_IPV6SUPPORT
  case AF_INET6:
    s = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (s != INVALID_SOCKET)
    {
      rescode = ::connect(s, remoteIP.genericsockaddr(), remoteIP.sockaddrLen());
      if (rescode != -1)
      {
        addrLen = sizeof(ipv6);
        if (::getsockname(s, (sockaddr*)&ipv6, &addrLen) != -1)
          result = NetworkAddress((sockaddr&)ipv6, addrLen);
      }
      ::close(s);
    }
		break;
#endif

  default:

    assert(0);
  }
	
	if (result.isEmpty())
	{
		// Get first available interface
		for (unsigned i=0; i<mIPList.size() && result.isEmpty(); i++)
		{
      if (!mIPList[i].isLoopback() && mIPList[i].family() == remoteIP.family())
				result = mIPList[i];
		}
	}
  return result;
#endif
}

#if defined(TARGET_WIN) && !defined(WINDOWS_RT)
NetworkAddress NetworkHelper::ipInterfaceByIndex(int index)
{
  for (unsigned i=0; i<mIP2IndexList.size(); i++)
  {
    if (mIP2IndexList[i].mIndex == index)
      return mIP2IndexList[i].mIP;
  }

  return NetworkAddress();
}

#endif

NetworkHelper& NetworkHelper::instance()
{
  mGuard.lock();
  try
  {
    if (!mInstance)
      mInstance = new NetworkHelper();
  }
  catch(...)
  {
    ICELogError(<< "Failed to create NetworkHelper instance");
  }
  mGuard.unlock();
  
  return *mInstance;
}

void NetworkHelper::destroyInstance()
{
  delete mInstance;
}

bool NetworkHelper::hasIPv4() const
{
  // Check interface list and see if it has LAN or public IP
#if defined(TARGET_WIN) && !defined(WINDOWS_RT)
  for (const auto& item: mInterfaceList)
#else
  for (const auto& item: mIPList)
#endif
    if (item.family() == AF_INET && (item.isLAN() || item.isPublic()))
      return true;
  return false;
}

bool NetworkHelper::hasIPv6() const
{
#if defined(TARGET_WIN) && !defined(WINDOWS_RT)
  for (const auto& item : mInterfaceList)
#else
  for (const auto& item: mIPList)
#endif
    if (item.family() == AF_INET6 && (item.isLAN() || item.isPublic()))
      return true;
  return false;
}

void NetworkHelper::NetworkToHost(const in6_addr& addr6, uint32_t* output)
{
  for (int i=0; i<4; i++)
#if defined(TARGET_WIN)
    output[i] = ntohl(((uint32_t*)addr6.u.Byte[0])[i]);
#elif defined(TARGET_IOS) || defined(TARGET_OSX)
    output[i] = ntohl(addr6.__u6_addr.__u6_addr32[i]);
#elif defined(TARGET_OPENWRT) || defined(TARGET_MUSL)
      output[i] = ntohl(addr6.__in6_union.__s6_addr32[i]);
#elif defined(TARGET_LINUX)
    output[i] = ntohl(addr6.__in6_u.__u6_addr32[i]);
#elif defined(TARGET_ANDROID)
    output[i] = ntohl(addr6.in6_u.u6_addr32[i]);
#endif
}

void NetworkHelper::HostToNetwork(const uint32_t* input, in6_addr& output)
{
  for (int i=0; i<4; i++)
#if defined(TARGET_WIN)
    ((uint32_t*)&output.u.Byte[0])[i] = htonl(input[i]);
#elif defined(TARGET_OSX) || defined(TARGET_IOS)
    output.__u6_addr.__u6_addr32[i] = htonl(input[i]);
#elif defined(TARGET_OPENWRT) || defined(TARGET_MUSL)
      output.__in6_union.__s6_addr32[i] = htonl(input[i]);
#elif defined(TARGET_LINUX)
    output.__in6_u.__u6_addr32[i] = htonl(input[i]);
#elif defined(TARGET_ANDROID)
    output.in6_u.u6_addr32[i] = htonl(input[i]);
#endif
}

NetworkHelper* NetworkHelper::mInstance = NULL;
Mutex NetworkHelper::mGuard;

//-------------------------------------- INHDestroy ---------------------------------
class INHDestroy
{
public:
  INHDestroy()
  {
  }
  
  ~INHDestroy()
  {
    NetworkHelper::destroyInstance();
  }
};

INHDestroy GINHDestroyer;
