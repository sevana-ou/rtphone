/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICETransactionList.h"
#include "ICEAction.h"
#include "ICELog.h"

using namespace ice;

#define LOG_SUBSYSTEM "ICE"

TransactionList::TransactionList()
:mRegularIndex(0), mPrioritizedIndex(0)
{
	mRegularList.reserve(1024);
	mPrioritizedList.reserve(1024);
}

TransactionList::~TransactionList()
{
	for (unsigned i=0; i<mPrioritizedList.size(); i++)
		delete mPrioritizedList[i];
	for (unsigned i=0; i<mRegularList.size(); i++)
		delete mRegularList[i];
}

bool TransactionList::processIncoming(List& l, StunMessage& msg, NetworkAddress& address)
{
	unsigned index;
  for (index = 0; index < l.size(); index++)
  {
    bool finished = false;
    Transaction* t = l[index];
    if (t->removed())
      continue;
    
    finished = t->state() == Transaction::Success || t->state() == Transaction::Failed;
    if (!finished)
    {
      if (t->processData(msg, address))
      {
        finished = t->state() == Transaction::Success || t->state() == Transaction::Failed;
        if (finished && t->action())
          t->action()->finished(*t);

        // Previous action could restart transaction - so check transaction state again
        finished = t->state() == Transaction::Success || t->state() == Transaction::Failed;
        
        if (finished)
        {
          if (t->keepalive())
            t->restart();
          else
            t->setRemoved(true);
        }
        return true;
      }
    }
  }
  return false;
}

bool TransactionList::processIncoming(StunMessage& msg, NetworkAddress& address)
{
  if (processIncoming(mRegularList, msg, address))
    return true;
  if (processIncoming(mPrioritizedList, msg, address))
    return true;
  
  return false;
}

bool TransactionList::checkTimeout(List& l)
{
  // Iterate checks
	unsigned index;
  for (index=0; index<l.size(); index++)
  {
    // Check only running transactions
    Transaction* t = l[index];
    if (t->state() == Transaction::Running)
    {
      // Is transaction timeouted?
      if (t->isTimeout())
      {
        ICELogDebug(<< "Transaction " << t->comment() << " timeouted");
        if (t->action())
          t->action()->finished(*t);
        
				if (t->keepalive())
					t->restart();
				else
					t->setRemoved(true);
        return true;
      }
    }
  }
  return false;
}

bool TransactionList::checkTimeout()
{
  if (checkTimeout(mPrioritizedList))
    return true;
  if (checkTimeout(mRegularList))
    return true;

  return false;
}

void TransactionList::addRegular(Transaction* p)
{
  List::iterator regularIter = std::find(mRegularList.begin(), mRegularList.end(), p);
  List::iterator prioritizedIter = std::find(mPrioritizedList.begin(), mPrioritizedList.end(), p);
  
  if (regularIter == mRegularList.end() && prioritizedIter == mPrioritizedList.end())
    mRegularList.push_back(p);
}

void TransactionList::erase(Transaction* p)
{
  List::iterator it = std::find(mRegularList.begin(), mRegularList.end(), p);
  if (it != mRegularList.end())
    p->setRemoved(true);
  else
  {
    it = std::find(mPrioritizedList.begin(), mPrioritizedList.end(), p);
    if (it != mPrioritizedList.end())
      p->setRemoved(true);
  }
}

void TransactionList::erase(const TransactionCondition& cond)
{
  List::iterator iter;
  for (iter = mRegularList.begin();iter != mRegularList.end(); iter++)
  {
    if (cond.check(*iter))
      (*iter)->setRemoved(true);
  }
  
  for (iter = mPrioritizedList.begin(); iter != mPrioritizedList.end(); iter++)
  {
    if (cond.check(*iter))
      (*iter)->setRemoved(true);
  }
}

void TransactionList::eraseMatching(unsigned types)
{
  List::iterator iter;
  for (iter = mRegularList.begin();iter != mRegularList.end(); iter++)
  {
    if (types && (*iter)->type())
      (*iter)->setRemoved(true);
  }

  for (iter = mPrioritizedList.begin(); iter != mPrioritizedList.end(); iter++)
  {
    if (types && (*iter)->type())
			(*iter)->setRemoved(true);
  }
}

int TransactionList::copyMatching(const TransactionCondition& condition, std::vector<Transaction*>& result)
{
  List::iterator iter;
  int counter = (int)result.size();
  for (iter = mRegularList.begin();iter != mRegularList.end(); iter++)
  {
    if (condition.check(*iter))
      result.push_back(*iter);
  }
  
  for (iter = mPrioritizedList.begin(); iter != mPrioritizedList.end(); iter++)
  {
    if (condition.check(*iter))
			result.push_back(*iter);
  }
  
  return (int)result.size() - counter;
}

void TransactionList::prioritize(Transaction* p)
{
  // Check if this transaction is prioritized already
  List::iterator iter = std::find(mPrioritizedList.begin(), mPrioritizedList.end(), p);
  if (iter != mPrioritizedList.end())
    return;

  // Check if the transaction is in regular list
  iter = std::find(mRegularList.begin(), mRegularList.end(), p);
  if (iter == mRegularList.end())
    return;

  // Remove the transaction from regular list
  mRegularList.erase(iter);

  // And move to priority list
  mPrioritizedList.push_back(p);
}

Transaction* TransactionList::getNextTransaction()
{
	while (mPrioritizedIndex < mPrioritizedList.size() && mPrioritizedList[mPrioritizedIndex]->removed())
		mPrioritizedIndex++;
	if (mPrioritizedIndex < mPrioritizedList.size())
		return mPrioritizedList[mPrioritizedIndex++];
	
	while (mRegularIndex < mRegularList.size() && mRegularList[mRegularIndex]->removed())
		mRegularIndex++;
	if (mRegularIndex < mRegularList.size())
		return mRegularList[mRegularIndex++];
	
  mPrioritizedIndex = 0;
  mRegularIndex = 0;
  
	while (mPrioritizedIndex < mPrioritizedList.size() && mPrioritizedList[mPrioritizedIndex]->removed())
		mPrioritizedIndex++;
	if (mPrioritizedIndex < mPrioritizedList.size())
		return mPrioritizedList[mPrioritizedIndex++];
	
	while (mRegularIndex < mRegularList.size() && mRegularList[mRegularIndex]->removed())
		mRegularIndex++;
	if (mRegularIndex < mRegularList.size())
		return mRegularList[mRegularIndex++];
  
  return NULL;
}

void TransactionList::addPrioritized(Transaction* p)
{
  List::iterator regularIter = std::find(mRegularList.begin(), mRegularList.end(), p);
  List::iterator prioritizedIter = std::find(mPrioritizedList.begin(), mPrioritizedList.end(), p);
  
  if (regularIter != mRegularList.end())
    mRegularList.erase(regularIter);

  if (prioritizedIter == mPrioritizedList.end())
    mPrioritizedList.push_back(p);
}

int TransactionList::count()
{
  return (int)mRegularList.size() + (int)mPrioritizedList.size();
}

void TransactionList::clear()
{
  mRegularList.clear();
  mPrioritizedList.clear();
}

bool TransactionList::exists(Transaction* t)
{
  List::iterator regularIter = std::find(mRegularList.begin(), mRegularList.end(), t);
  List::iterator prioritizedIter = std::find(mPrioritizedList.begin(), mPrioritizedList.end(), t);
  
  bool removed = true;
  
  if (regularIter != mRegularList.end())
    removed &= (*regularIter)->removed();
    
  if (prioritizedIter != mPrioritizedList.end())
    removed &= (*prioritizedIter)->removed();

  return !removed;
}

bool TransactionList::exists(const TransactionCondition& condition)
{
  int counter = 0;
  List::iterator iter;
  for (iter = mRegularList.begin();iter != mRegularList.end(); iter++)
  {
    if (condition.check(*iter))
      counter++;
  }
  
  for (iter = mPrioritizedList.begin(); iter != mPrioritizedList.end(); iter++)
  {
    if (condition.check(*iter))
      counter++;
  }
  return counter > 0;
}
