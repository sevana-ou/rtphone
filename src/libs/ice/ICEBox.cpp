/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEBox.h"
#include "ICEBoxImpl.h"
#include <time.h>

using namespace ice;

Stack::~Stack()
{
}

void Stack::initialize()
{
}

void Stack::finalize()
{
  ;
}

Stack* Stack::makeICEBox(const ServerConfig& config)
{
  return new StackImpl(config);
}

bool Stack::isDataIndication(ByteBuffer& source, ByteBuffer* plain)
{
  return Session::isDataIndication(source, plain);
}

bool Stack::isStun(ByteBuffer& source)
{
  return Session::isStun(source);
}

bool Stack::isRtp(ByteBuffer& data)
{
  return Session::isRtp(data);
}

bool Stack::isChannelData(ByteBuffer& data, TurnPrefix prefix)
{
  return Session::isChannelData(data, prefix);
}

ByteBuffer Stack::makeChannelData(TurnPrefix prefix, const void* data, unsigned datasize)
{
  ByteBuffer result;
  result.resize(4 + datasize);
  BufferWriter writer(result);
  writer.writeUShort(prefix);
  writer.writeUShort(datasize);
  writer.writeBuffer(data, datasize);
  
  return result;
}
