/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HL_ASYNCCOMMAND_H
#define HL_ASYNCCOMMAND_H

class AsyncCommand
{
public:
  AsyncCommand();
  virtual ~AsyncCommand();

  virtual void run(void* environment) = 0;
  virtual bool finished() = 0;
};

#endif // HL_ASYNCCOMMAND_H
