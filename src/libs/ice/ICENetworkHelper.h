/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_NETWORK_HELPER_H
#define __ICE_NETWORK_HELPER_H

#include "ICEPlatform.h"
#include "ICESync.h"
#include "ICEAddress.h"
#include <string>
#include <vector>

#ifdef _WIN32
# include <winsock2.h>
# include <Iphlpapi.h>
#else
# include <ifaddrs.h>
#endif

namespace ice 
{

  class NetworkHelper
  {
  public:
    NetworkHelper();
    ~NetworkHelper();
    
    enum
    {
      NetworkType_None,
      NetworkType_WiFi,
      NetworkType_3G
    };
    
    Mutex& guard();
    
    /* Returns list of IP4 interfaces installed in system. */
    std::vector<NetworkAddress>& interfaceList();
    
    bool hasIPv4() const;
    bool hasIPv6() const;
    
    /* Finds source interface for specified remote address. */
    NetworkAddress  sourceInterface(const NetworkAddress& remoteIP);

    void reload(int networkType);

    static NetworkHelper&  instance();
    static void            destroyInstance();
    
    static void NetworkToHost(const in6_addr& addr6, uint32_t* output);
    static void HostToNetwork(const uint32_t* input, in6_addr& output);
    
  protected:
#ifdef _WIN32
    struct IP2Index
    {
      NetworkAddress mIP;
      DWORD mIndex;
    };
    std::vector<IP2Index>     mIP2IndexList;
    std::vector<NetworkAddress>   mInterfaceList;

    void processAdaptersList(IP_ADAPTER_ADDRESSES* addresses);
    NetworkAddress ipInterfaceByIndex(int index);
#else
    std::vector<NetworkAddress>   mIPList;
#endif
    static NetworkHelper*  mInstance;
    static Mutex           mGuard;
    Mutex mInstanceGuard;
  };

};

#endif
