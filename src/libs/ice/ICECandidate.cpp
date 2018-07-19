/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICECandidate.h"
#include "ICEError.h"
#include "ICELog.h"
#include <stdio.h>
#include <string.h>

#define LOG_SUBSYSTEM "ICE"

using namespace ice;

void Candidate::setLocalAndExternalAddresses(std::string& ip, unsigned short portNumber)
{
  mLocalAddr.setIp(ip); 
  mLocalAddr.setPort(portNumber);
  mExternalAddr.setIp(ip);
  mExternalAddr.setPort(portNumber);
}

void Candidate::setLocalAndExternalAddresses(NetworkAddress& addr)
{
  mLocalAddr = addr;
  mExternalAddr = addr;
}

void Candidate::setLocalAndExternalAddresses(NetworkAddress &addr, unsigned short altPort)
{
  mLocalAddr = addr;
  mExternalAddr = addr;
  mLocalAddr.setPort(altPort);
  mExternalAddr.setPort(altPort);
}

void Candidate::computePriority(int* typepreflist)
{
  mPriority = (typepreflist[mType] << 24) + (mInterfacePriority << 8) + (256 - mComponentId);
}

void Candidate::computeFoundation()
{
  sprintf(mFoundation, "%u", unsigned((mType << 24) + inet_addr(mLocalAddr.ip().c_str())));
}

const char* Candidate::type()
{
  switch (mType)
  {
  case Candidate::Host:
    return "host";

  case Candidate::ServerReflexive:
    return "srflx";

  case Candidate::ServerRelayed:
    return "relay";

  case Candidate::PeerReflexive:
    return "prflx";

  default:
    ICELogError(<< "Bad candidate type, reverted to Host.");
    return "host";
  }
}

std::string Candidate::createSdp()
{
  char buffer[512];
  
  unsigned port = (unsigned)mExternalAddr.port();

#define SDP_CANDIDATE_FORMAT "%s %u %s %u %s %u typ %s"

  sprintf(buffer, SDP_CANDIDATE_FORMAT, strlen(mFoundation) ? mFoundation : "16777000" , mComponentId, "UDP", mPriority, mExternalAddr.ip().c_str(), port, type());
  
  if (mType != Candidate::Host)
  {
    char relattr[64];
    sprintf(relattr, " raddr %s rport %u", mLocalAddr.ip().c_str(), (unsigned int)mLocalAddr.port());
    strcat(buffer, relattr);
  }
  
  return buffer;
}

Candidate Candidate::parseSdp(const char* sdp)
{
  Candidate result(Candidate::Host);

  char protocol[32], externalIP[128], candtype[32], raddr[64];
  unsigned int remoteport = 0;
  
  unsigned int externalPort = 0;
  
  // Look for "typ" string
  const char* formatstring;
  if (strstr(sdp, "typ"))
    formatstring = "%s %u %s %u %s %u typ %s raddr %s rport %u";
  else
    formatstring = "%s %u %s %u %s %u %s raddr %s rport %u";
  int wasread = sscanf(sdp, formatstring, &result.mFoundation, &result.mComponentId, protocol, 
                    &result.mPriority, externalIP, &externalPort, candtype, raddr, &remoteport);

  if (wasread >= 7)
  {
    // Save external address
    result.mExternalAddr.setIp( externalIP );
    result.mExternalAddr.setPort( externalPort );
    
    result.mLocalAddr = result.mExternalAddr;

    // Check the protocol (UDP4/6 is supported only)
#ifdef _WIN32
    _strupr(protocol); _strupr(candtype);
#else
    strupr(protocol); strupr(candtype);
#endif
    if (strcmp(protocol, "UDP") != 0)
      throw Exception(UDP_SUPPORTED_ONLY);

    // Save candidate type
    result.mType = typeFromString(candtype);
  }

  return result;
}

Candidate::Type  Candidate::typeFromString(const char* candtype)
{
  if (!strcmp(candtype, "HOST"))
    return Candidate::Host;
  else
  if (!strcmp(candtype, "SRFLX"))
    return Candidate::ServerReflexive;
  else
  if (!strcmp(candtype, "PRFLX"))
    return Candidate::PeerReflexive;
  else
  if (!strcmp(candtype, "RELAY"))
    return Candidate::ServerRelayed;
  else
  {
    ICELogError(<< "Bad candidate type in parser. Reverted to Host");
    return Candidate::Host;
  }
}


bool Candidate::equal(Candidate& cand1, Candidate& cand2)
{
  if (cand1.mType != cand2.mType)
    return false;
  switch (cand1.mType)
  {
  case Candidate::Host:
    return (cand1.mLocalAddr == cand2.mLocalAddr);

  case Candidate::ServerReflexive:
  case Candidate::PeerReflexive:
  case Candidate::ServerRelayed:
    return (cand1.mExternalAddr == cand2.mExternalAddr);
      
  }
  
  ICELogError(<< "Bad candidate type, comparing as Host");
  return cand1.mLocalAddr == cand2.mLocalAddr;
}

bool Candidate::operator == (const Candidate& rhs) const
{
  bool relayed1 = mType == ServerRelayed, relayed2 = rhs.mType == ServerRelayed;

  return relayed1 == relayed2 && mLocalAddr == rhs.mLocalAddr && mExternalAddr == rhs.mExternalAddr;
}

void Candidate::dump(std::ostream& output)
{
  output << Logger::TabPrefix<< Logger::TabPrefix << createSdp().c_str() << std::endl;
}

int Candidate::component()
{
  return mComponentId;
}
