/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HL_OsVersion.h"

#ifdef TARGET_WIN

#include <windows.h>

#if defined(USE_MINIDUMP)
# include <DbgHelp.h>
#endif

int winVersion()
{
  DWORD dwVersion = 0; 
  DWORD dwMajorVersion = 0;
  DWORD dwMinorVersion = 0; 
  DWORD dwBuild = 0;

  dwVersion = GetVersion();

  // Get the Windows version.

  dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
  dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

  // Get the build number.

  if (dwVersion < 0x80000000)              
      dwBuild = (DWORD)(HIWORD(dwVersion));
  
  if (dwMajorVersion == 5)
    return Win_Xp;

  if (dwMinorVersion == 1)
    return Win_Seven;
  else
    return Win_Vista;
 }

// ----------------- CrashMiniDump -----------------
#if defined(USE_MINIDUMP)
static LONG WINAPI MyExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
  // Open the file
  HANDLE hFile = CreateFile( L"MiniDump.dmp", GENERIC_READ | GENERIC_WRITE,
    0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

  if( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) )
  {
    // Create the minidump
    MINIDUMP_EXCEPTION_INFORMATION mdei;

    mdei.ThreadId           = GetCurrentThreadId();
    mdei.ExceptionPointers  = ExceptionInfo;
    mdei.ClientPointers     = FALSE;

    MINIDUMP_TYPE mdt       = MiniDumpWithFullMemory;

    BOOL rv = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(),
      hFile, mdt, (ExceptionInfo != 0) ? &mdei : 0, 0, 0 );

    // Close the file
    CloseHandle( hFile );
  }
  else
  {
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

static LPTOP_LEVEL_EXCEPTION_FILTER OldExceptionHandler = nullptr;

void CrashMiniDump::registerHandler()
{
  OldExceptionHandler = ::SetUnhandledExceptionFilter(&MyExceptionHandler);
}

void CrashMiniDump::unregisterHandler()
{
  ::SetUnhandledExceptionFilter(nullptr);
}

#endif

#endif

#ifdef TARGET_IOS
int iosVersion()
{
    return 4; // Stick with this for now
}
#endif

#if defined(TARGET_LINUX) || defined(TARGET_OSX)
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int _kbhit()
{
    /*
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized)
    {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;*/
    static const int STDIN_FILENO = 0;
    struct termios oldt, newt;
     int ch;
     int oldf;

     tcgetattr(STDIN_FILENO, &oldt);
     newt = oldt;
     newt.c_lflag &= ~(ICANON | ECHO);
     tcsetattr(STDIN_FILENO, TCSANOW, &newt);
     oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
     fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

     ch = getchar();

     tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
     fcntl(STDIN_FILENO, F_SETFL, oldf);

     if(ch != EOF)
     {
       ungetc(ch, stdin);
       return 1;
     }

     return 0;
}

#endif
