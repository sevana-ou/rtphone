/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __OS_VERSION_H
#define __OS_VERSION_H

#ifdef TARGET_WIN

enum
{
  Win_Xp = 0,
  Win_Vista = 1,
  Win_Seven = 2,
  Win_Eight = 3,
  Win_Ten = 4
};

extern int winVersion();

class CrashMiniDump
{
public:
  static void registerHandler();
  static void unregisterHandler();
};

extern void writeMiniDump();

#endif

#ifdef TARGET_IOS
int iosVersion();
#endif


#if defined(TARGET_LINUX) || defined(TARGET_OSX)

#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#if defined(TARGET_LINUX)
// # include <stropts.h>
#endif

extern int _kbhit();

#endif

#if defined(TARGET_WIN)
# include <conio.h>
#endif
#endif
