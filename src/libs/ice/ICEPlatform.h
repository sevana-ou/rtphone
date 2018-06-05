/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_PLATFORM_H
#define __ICE_PLATFORM_H

#define NoIndex 0xFFFFFFFF

enum AddressFamily
{
  IPv4 = 1,
  IPv6 = 2
};

#ifdef _WIN32
# include <winsock2.h>
# include <windows.h>
# include <ws2tcpip.h>
# include <time.h>
# define MINVALUE(X,Y) (((X)<(Y)) ? (X) : (Y))
# define MAXVALUE(X,Y) (((X)>(Y)) ? (X) : (Y))
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <stdlib.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <string.h>
# include <errno.h>
# include <algorithm>
# define MINVALUE(X,Y) (((X)<(Y)) ? (X) : (Y))
# define MAXVALUE(X,Y) (((X)>(Y)) ? (X) : (Y))

#ifndef SOCKET
typedef int SOCKET;
#endif

extern char* strupr(char*);

#ifndef INVALID_SOCKET
#	define INVALID_SOCKET -1
#endif

#endif

// TURN channel prefix
typedef unsigned short TurnPrefix;


// Limit of candidates per SIP offer/answer
#define ICE_CANDIDATE_LIMIT           64

// Limit of connection checks
#define ICE_CONNCHECK_LIMIT           100

// Connection checks timer interval (in milliseconds)
#define ICE_SCHEDULE_TIMER_INTERVAL   5

#define ICE_PERMISSIONS_REFRESH_INTERVAL 240

// Turns on OutputDebugString() logging
#define ICE_REALTIME_LOG

// Enables keep-alive packets. It MUST be defined! There is only 1 reason to undefine it - debugging.
#define ICE_ENABLE_KEEPALIVE

#define ICE_SKIP_LINKLOCAL
#define ICE_SKIP_RELAYED_CHECKS

//#define ICE_IPV6_SUPPORT
//#define ICE_LOOPBACK_SUPPORT

//#define ICE_DISABLE_KEEP

// Makes check list shorter. Most checks are triggered in this case.
#define ICE_SMART_PRUNE_CHECKLIST

// Simulates symmetric NAT behavior - stack rejects all data coming not from specified STUN/TURN servers.
//#define ICE_EMULATE_SYMMETRIC_NAT

// Publishes more reflexive candidates than was gathered. 
// The virtual candidates will have port number +1 +2 +3 ... +ICE_VIRTUAL_CANDIDATES
// 
#define ICE_VIRTUAL_CANDIDATES (0)

// Use simple model to schedule connectivity checks
//#define ICE_SIMPLE_SCHEDULE

// Limit of packet retransmission during ice checks
#define ICE_TRANSACTION_RTO_LIMIT (10)

#define ICE_POSTPONE_RELAYEDCHECKS

// #define ICE_AGGRESSIVE
// Use aggressive nomination + treat requests as confirmations
// #define ICE_VERYAGGRESSIVE

// Define to emulate network problem
//#define ICE_TEST_VERYAGGRESSIVE

// Use this define to avoid gathering reflexive/relayed candidates for public IP address 
//#define ICE_REST_ON_PUBLICIP

// #define ICE_DONT_CANCEL_CHECKS_ON_SUCCESS

// Defines the waiting time to accumulate valid pairs to choose best of them later. Can be zero.
#define ICE_NOMINATION_WAIT_INTERVAL (50)

#define MAX_CLIENTCHANNELBIND_ATTEMPTS (1)

#define ICE_CACHE_REALM_NONCE

// Should be 1100 in normal conditions. 1000000 is for debugging
#define ICE_TIMEOUT_VALUE 1100


#endif
