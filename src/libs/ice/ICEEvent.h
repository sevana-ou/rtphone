/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_EVENT_H
#define __ICE_EVENT_H

namespace ice
{
  class Stack;
  
  class StageHandler
  {
  public:
    // Fires when candidates are gathered
    virtual void onGathered(Stack* stack, void* tag) = 0;

    // Fires when connectivity checks finished ok
    virtual void onSuccess(Stack* stack, void* tag) = 0;

    // Fires when connectivity checks failed (timeout usually)
    virtual void onFailed(Stack* stack, void* tag) = 0;
  };

  class ChannelBoundCallback
  {
  public:
    virtual ~ChannelBoundCallback() {}
    virtual void onChannelBound(int /*stream*/, int /*component*/, int /*error*/) {}
  };
  
  class InstallPermissionsCallback
  {
  public:
    virtual ~InstallPermissionsCallback() {}
    virtual void onPermissionsInstalled(int /*stream*/, int /*component*/, int /*error*/) {}
  };

  class DeletePermissionsCallback
  {
  public:
    virtual ~DeletePermissionsCallback() {}
    virtual void onPermissionsDeleted(int /*stream*/, int /*component*/, int /*error*/) {}
  };

  class DeleteAllocationCallback
  {
  public:
    virtual ~DeleteAllocationCallback() {}
    virtual void onAllocationDeleted(int /*stream*/, int /*component*/, int /*error*/) {}
  };
  
  class RefreshAllocationCallback
  {
  public:
    virtual ~RefreshAllocationCallback();
    virtual void onAllocationRefreshed(int /*stream*/, int /*component*/, int /*error*/) {}
  };
}

#endif
