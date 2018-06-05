/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_ACTION__H
#define __ICE_ACTION__H

#include "ICESmartPtr.h"
#include "ICECandidatePair.h"

namespace ice
{
  class   Transaction;
  struct  Stream;
  class   Logger;
  struct  StackConfig;

  class Action
  {
  protected:
    Stream& mStream;
    StackConfig& mConfig;

  public:
    Action(Stream& stream, StackConfig& config)
      :mStream(stream), mConfig(config)
    {
    }
    
    virtual ~Action()
    {
    }
     
    virtual void finished(Transaction& transaction) = 0;
  };
  
  typedef SmartPtr<Action> PAction;

}

#endif