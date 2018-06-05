/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_TRANSACTION_LIST_H
#define __ICE_TRANSACTION_LIST_H

#include <vector>
#include "ICEStunTransaction.h"

namespace ice
{
  class TransactionCondition
  {
  public:
    virtual bool check(Transaction* t) const = 0;
  };
  
  class TransactionList
  {
  protected:
    typedef std::vector<Transaction*> List;
    List  mRegularList, mPrioritizedList;
    unsigned mRegularIndex, mPrioritizedIndex;

    bool processIncoming(List& l, StunMessage& msg, NetworkAddress& address);
    bool checkTimeout(List& l);
  public:
    TransactionList();
    ~TransactionList();
    
    bool processIncoming(StunMessage& msg, NetworkAddress& address);
    bool checkTimeout();
    void addRegular(Transaction* p);
    void addPrioritized(Transaction* p);
    void prioritize(Transaction* p);
    void erase(Transaction* p);
    void erase(const TransactionCondition& condition);
    void eraseMatching(unsigned types);
    int copyMatching(const TransactionCondition& condition, std::vector<Transaction*>& result);
    Transaction* getNextTransaction();
    int count();
    void clear();
    bool exists(Transaction* t);
    bool exists(const TransactionCondition& condition);
  };
}

#endif