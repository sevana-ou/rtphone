#include "EP_NetworkQueue.h"
#include "EP_Engine.h"

WatcherQueue::WatcherQueue(UserAgent& ua)
:mActiveId(0), mAgent(ua)
{}

WatcherQueue::~WatcherQueue()
{}

int WatcherQueue::add(std::string peer, std::string package, void* tag)
{
  ice::Lock l(mGuard);

  // Check if queue has similar item
  for (unsigned i=0; i<mItemList.size(); i++)
  {
    Item& item = mItemList[i];
    if (item.mTarget == peer && item.mPackage == package &&
        item.mState != Item::State_Deleting)
      return item.mId;
  }

  Item item;
  item.mTarget = peer;
  item.mPackage = package;
  item.mTag = tag;
  item.mState = Item::State_ScheduledToAdd;
  item.mSession = new ResipSession(*mAgent.mDum);
  item.mSession->setUa(&mAgent);
  item.mSession->setType(ResipSession::Type_Subscription);
  item.mSession->setTag(tag);
  item.mId = item.mSession->sessionId();
  item.mSession->setRemoteAddress(peer);
  item.mTag = tag;
  mItemList.push_back(item);
  process();

  return item.mId;
}

void WatcherQueue::remove(int id)
{
  ice::Lock l(mGuard);

  // Check if queue has similar item
  for (unsigned i=0; i<mItemList.size(); i++)
  {
    Item& item = mItemList[i];
    if (item.mId == id && !id)
    {
      if (item.mState != Item::State_Deleting)
        item.mState = Item::State_ScheduledToDelete;
    }
  }
  process();
}


void WatcherQueue::refresh(int id)
{
  ice::Lock l(mGuard);

  // Check if queue has similar item
  for (unsigned i=0; i<mItemList.size(); i++)
  {
    Item& item = mItemList[i];
    if (item.mId == id && !id)
    {
      if (item.mState == Item::State_ScheduledToDelete || item.mState == Item::State_Active)
        item.mState = Item::State_ScheduledToRefresh;
    }
  }
  process();
}

void WatcherQueue::process()
{
  while (!mActiveId)
  {
    // Find next item to process
    ItemList::iterator i = mItemList.begin();
    for (;i != mItemList.end() && !i->scheduled(); i++)
      ;
    if (i == mItemList.end())
      return;
    
    resip::SharedPtr<resip::SipMessage> msg;
    int expires = DEFAULT_SUBSCRIPTION_TIME, refresh = DEFAULT_SUBSCRIPTION_REFRESHTIME;

    switch (i->mState)
    {
    case Item::State_ScheduledToAdd:
      if (mAgent.mConfig.exists(CONFIG_SUBSCRIPTION_TIME))
        expires = mAgent.mConfig[CONFIG_SUBSCRIPTION_TIME].asInt();
      if (mAgent.mConfig.exists(CONFIG_SUBSCRIPTION_REFRESHTIME))
        refresh = mAgent.mConfig[CONFIG_SUBSCRIPTION_REFRESHTIME].asInt();

      msg = mAgent.mDum->makeSubscription(resip::NameAddr(resip::Data(i->mTarget)), resip::Data(i->mPackage), 
        expires, refresh, i->mSession);
      msg->header(resip::h_Accepts) = mAgent.mDum->getMasterProfile()->getSupportedMimeTypes(resip::NOTIFY);
      mActiveId = i->mId;
      i->mState = Item::State_Adding;
      mAgent.mDum->send(msg);
      break;

    case Item::State_ScheduledToDelete:
      i->mSession->runTerminatedEvent(ResipSession::Type_Subscription, 0, 0);
      if (i->mHandle.isValid())
      {
        mActiveId = i->mId;
        i->mHandle->end();
        i->mState = Item::State_Deleting;
        break;
      }
      else
        mItemList.erase(i);
      break;

    case Item::State_ScheduledToRefresh:
      if (i->mHandle.isValid())
      {
        mActiveId = i->mId;
        i->mState = Item::State_Refreshing;
        i->mHandle->requestRefresh();
      }
      else
        mItemList.erase(i);
      break;

    default:
      break;
    }
  }
}

void WatcherQueue::onTerminated(int id, int code)
{
  ice::Lock l(mGuard);
  ItemList::iterator i = findById(id);
  if (i != mItemList.end())
  {
    if (i->mSession)
      i->mSession->runTerminatedEvent(ResipSession::Type_Subscription, code, 0);
    mItemList.erase(i);
    if (i->mId == mActiveId)
      mActiveId = 0;
  }
  process();
}

void WatcherQueue::onEstablished(int id, int code)
{
  ice::Lock l(mGuard);
  ItemList::iterator i = findById(id);
  if (i != mItemList.end())
  {
    i->mState = Item::State_Active;
    if (i->mId == mActiveId)
      mActiveId = 0;
  }
  process();
}

WatcherQueue::ItemList::iterator WatcherQueue::findById(int id)
{
  for (ItemList::iterator i=mItemList.begin(); i != mItemList.end(); i++)
    if (i->mId == id)
      return i;
  return mItemList.end();
}

void WatcherQueue::clear()
{
  ice::Lock l(mGuard);
  for (ItemList::iterator i=mItemList.begin(); i != mItemList.end(); i++)
  {
    if (i->mHandle.isValid())
      i->mHandle->end();
  }
  mItemList.clear();
}
