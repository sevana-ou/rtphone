/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEIosSupport.h"
#include "ICENetworkHelper.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#import <SystemConfiguration/SystemConfiguration.h>

#import <Foundation/Foundation.h>
#include "TargetConditionals.h"
#include "ICEAddress.h"
#include "ICELog.h"
#include "ICENetworkHelper.h"

#define LOG_SUBSYSTEM "ICE"

namespace ice
{

  typedef enum {
    ConnectionTypeUnknown,
    ConnectionTypeNone,
    ConnectionType3G,
    ConnectionTypeWiFi
  } ConnectionType;
  
  
  ConnectionType FindConnectionType(int family)
  {
    SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithName(NULL, family == AF_INET ? "8.8.8.8" : "2001:4860:4860::8888");
    SCNetworkReachabilityFlags flags;
    BOOL success = SCNetworkReachabilityGetFlags(reachability, &flags);
    CFRelease(reachability);
    if (!success) {
      return ConnectionTypeUnknown;
    }
    BOOL isReachable = ((flags & kSCNetworkReachabilityFlagsReachable) != 0);
    BOOL needsConnection = ((flags & kSCNetworkReachabilityFlagsConnectionRequired) != 0);
    BOOL isNetworkReachable = (isReachable && !needsConnection);
    
    if (!isNetworkReachable) {
      return ConnectionTypeNone;
    } else if ((flags & kSCNetworkReachabilityFlagsIsWWAN) != 0) {
      return ConnectionType3G;
    } else {
      return ConnectionTypeWiFi;
    }
  }
  
int fillIosInterfaceList(int family, int networkType, std::vector<ice::NetworkAddress>& output)
{
  if (networkType == NetworkHelper::NetworkType_None)
  {
    switch (FindConnectionType(family))
    {
      case ConnectionTypeNone:
        return 0;
        
      case ConnectionType3G:
        networkType = NetworkHelper::NetworkType_3G;
        break;
        
      case ConnectionTypeWiFi:
        networkType = NetworkHelper::NetworkType_WiFi;
        break;
        
      default:
        break;
    }
  }
  
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  
  struct ifaddrs *interfaces = NULL;
  struct ifaddrs *temp_addr = NULL;
  
  // retrieve the current interfaces - returns 0 on success
  if(!getifaddrs(&interfaces))
  {
    // Loop through linked list of interfaces
    for (temp_addr = interfaces; temp_addr != NULL; temp_addr = temp_addr->ifa_next)
    {
      sa_family_t sa_type = temp_addr->ifa_addr->sa_family;
      
      if (sa_type == family)
      {
        NSString* name = [NSString stringWithUTF8String: temp_addr->ifa_name];
        
        if (networkType != NetworkHelper::NetworkType_None)
        {
          bool wifi = [name rangeOfString: @"en"].location == 0;
          bool cell = [name rangeOfString: @"pdp_ip"].location == 0;
          /*bool vpn = [name rangeOfString: @"tun"].location == 0 || [name rangeOfString: @"tap"].location == 0;*/
          
          switch (networkType)
          {
            case NetworkHelper::NetworkType_3G:
              if (wifi) // Skip wifi addresses here. Use cell and vpn addresses here.
                continue;
              break;
            
            case NetworkHelper::NetworkType_WiFi:
              if (cell) // Skip cell addresses here. Use other addresses - wifi and vpn.
                continue;
              break;
            
            default:
              break;
          }
        }
        
        ice::NetworkAddress addr;
        if (sa_type == AF_INET6)
          addr = ice::NetworkAddress(((sockaddr_in6*)temp_addr->ifa_addr)->sin6_addr, 1000);
        else
          addr = ice::NetworkAddress(((sockaddr_in*)temp_addr->ifa_addr)->sin_addr, 1000);
        
        //ICELogDebug(<< "Found: " << addr.toStdString());
        
        if (!addr.isLoopback() && !addr.isLinkLocal())
          output.push_back(addr);
      }
      
    }
    // Free memory
    freeifaddrs(interfaces);
  }
  
  [pool release];
  return 0;
}

int getIosIp(int networkType, int family, char* address)
{
  assert(address);
  
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  
  struct ifaddrs *interfaces = NULL;
  struct ifaddrs *temp_addr = NULL;
  NSString *wifiAddress = nil;
  NSString *cellAddress = nil;
  
  // retrieve the current interfaces - returns 0 on success
  if(!getifaddrs(&interfaces))
  {
    // Loop through linked list of interfaces
    temp_addr = interfaces;
    while(temp_addr != NULL)
    {
      sa_family_t sa_type = temp_addr->ifa_addr->sa_family;
      
      if (sa_type == family)
      {
        NSString* name = [NSString stringWithUTF8String: temp_addr->ifa_name];
        char buffer[128];
        if (sa_type == AF_INET6)
          inet_ntop(AF_INET6, &((sockaddr_in6*)temp_addr->ifa_addr)->sin6_addr, buffer, sizeof(buffer));
        else
          inet_ntop(AF_INET, &((sockaddr_in*)temp_addr->ifa_addr)->sin_addr, buffer, sizeof(buffer));

        NSString* addr = [NSString stringWithUTF8String: buffer];
        
        if([name rangeOfString: @"en"].location == 0)
        {
          // Interface is the wifi connection on the iPhone
          wifiAddress = addr;
        }
        else
        if([name isEqualToString: @"pdp_ip0"])
        {
          // Interface is the cell connection on the iPhone
          cellAddress = addr;
        }
        else
        if ([name rangeOfString: @"tun"].location == 0 || [name rangeOfString: @"tap"].location == 0)
        {
          wifiAddress = addr;
        }
      }
      temp_addr = temp_addr->ifa_next;
    }
    // Free memory
    freeifaddrs(interfaces);
  }
  
  NSString* currentAddr = nil;
  switch (networkType)
  {
    case ice::NetworkHelper::NetworkType_None:
      currentAddr = wifiAddress ? wifiAddress : cellAddress;
      break;
    
    case ice::NetworkHelper::NetworkType_WiFi:
      currentAddr = wifiAddress;
      break;
      
    case ice::NetworkHelper::NetworkType_3G:
      currentAddr = cellAddress;
      break;
  }
  
  if (currentAddr)
    strcpy(address, [currentAddr UTF8String]);
  else
  if (wifiAddress)
    strcpy(address, [wifiAddress UTF8String]);
  else
  if (cellAddress)
    strcpy(address, [cellAddress UTF8String]);
  else
    strcpy(address, "127.0.0.1");
  
  [pool release];
  
  return 0;
}

}
