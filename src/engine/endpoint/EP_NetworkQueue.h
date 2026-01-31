/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __NETWORK_QUEUE_H
#define __NETWORK_QUEUE_H

#include "EP_Session.h"
#include <resip/dum/ClientSubscription.hxx>

class UserAgent;
class WatcherQueue
{
public:
    struct Item
    {
        enum State
        {
            State_None,
            State_Active,
            State_ScheduledToAdd,
            State_Adding,
            State_ScheduledToRefresh,
            State_Refreshing,
            State_ScheduledToDelete,
            State_Deleting
        };

        resip::ClientSubscriptionHandle mHandle;    // Subscription handle
        ResipSession* mSession = nullptr;
        State mState = State::State_None;
        std::string mTarget;                        // Target's address
        std::string mPackage;                       // Event package
        void* mTag = nullptr;                       // User tag
        int mId = 0;                                // Related session ID - it is always non-zero (zero is here for initialization only)

        Item()
        {}

        bool scheduled()
        {
            return mState == State_ScheduledToAdd || mState == State_ScheduledToDelete || mState == State_ScheduledToRefresh;
        }
    };
    WatcherQueue(UserAgent& agent);
    ~WatcherQueue();

    int add(const std::string& peer, const std::string& package, void* tag);
    void remove(int id);
    void refresh(int id);
    void clear();

    void onTerminated(int id, int code);
    void onEstablished(int id, int code);

protected:
    typedef std::vector<Item> ItemList;
    ItemList mItemList;
    ice::Mutex mGuard;
    UserAgent& mAgent;
    int mActiveId;

    void process();
    ItemList::iterator findById(int id);
};

#endif
