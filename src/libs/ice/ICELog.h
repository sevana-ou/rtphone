/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_LOG_H
#define __ICE_LOG_H

#include <string>
#include <sstream>

#ifdef _WIN32
# include <winsock2.h>
# include <windows.h>
#endif

#include "ICEPlatform.h"
#include "ICETypes.h"
#include "ICEEvent.h"

namespace ice
{
  // Defines log levels
  enum LogLevel
  {
    LL_NONE = -1,
    LL_CRITICAL = 0,
    LL_INFO = 1,
    LL_DEBUG = 2,
    LL_MEDIA = 3
  };

  class LogHandler
  {
  public:
    virtual void onIceLog(LogLevel level, const std::string& filename, int line, const std::string& subsystem, const std::string& msg) = 0;
  };
  
#ifdef _WIN32
  class LogGuard
  {
  public:
    LogGuard()    {  ::InitializeCriticalSection(&mCS);  }
    ~LogGuard()   {  ::DeleteCriticalSection(&mCS);      }
    void Lock()   {  ::EnterCriticalSection(&mCS);       }
    void Unlock() {  ::LeaveCriticalSection(&mCS);       }

  protected:
    CRITICAL_SECTION mCS;
  };

  class LogLock
  {
  public:
    LogLock(LogGuard& g) :mGuard(g)   {  mGuard.Lock();   }
    ~LogLock()                        {  mGuard.Unlock(); }

  protected:
    LogGuard& mGuard;
  };
#else
  class LogGuard
  {
  public:
    LogGuard()    {  ::pthread_mutex_init(&mMutex, NULL);  }
    ~LogGuard()   {  ::pthread_mutex_destroy(&mMutex);     }
    void Lock()   {  ::pthread_mutex_lock(&mMutex);        }
    void Unlock() {  ::pthread_mutex_unlock(&mMutex);      }

  protected:
    pthread_mutex_t mMutex;
  };

  class LogLock
  {
  public:
    LogLock(LogGuard& g) :mGuard(g)   {  mGuard.Lock();   }
    ~LogLock()                        {  mGuard.Unlock(); }

  protected:
    LogGuard& mGuard;
  };

#endif

  class Logger
  {
  public:
    static const char* TabPrefix;

    Logger();
    ~Logger();
    
    void useDebugWindow(bool enable);
    void useFile(const char* filepath);
    void useDelegate(LogHandler* delegate_);
    void useNull();
    void closeFile();
    void openFile();
    
    LogGuard&             mutex();
    LogLevel              level();
    void                  setLevel(LogLevel level);
    void                  beginLine(LogLevel level, const char* filename, int linenumber, const char* subsystem);
    void                  endLine();

    Logger& operator << (const char* data);
    Logger& operator << (const wchar_t* data);
    Logger& operator << (const int data);
    Logger& operator << (const float data);
    Logger& operator << (const std::string& data);
    Logger& operator << (const int64_t data);
    Logger& operator << (const unsigned int data);
    Logger& operator << (const uint64_t data);

  protected:
    LogGuard            mGuard;
    FILE*               mFile;
    std::string         mLogPath;

    bool                mUseDebugWindow;
    LogHandler*         mDelegate;
    LogLevel            mLevel;
    std::ostringstream* mStream;
    
    LogLevel            mMsgLevel;
    std::string         mFilename;
    int                 mLine;
    std::string         mSubsystem;
  };


extern Logger GLogger;


#define ICELog(level_, subsystem_, args_)\
   {do\
   {\
      if (GLogger.level() >= level_)\
      {\
        LogLock l(GLogger.mutex());\
        GLogger.beginLine(level_, __FILE__, __LINE__, subsystem_);\
        GLogger args_;\
        GLogger.endLine();\
      }\
  } while (false);}

#define ICELogCritical(args_) ICELog(LL_CRITICAL, LOG_SUBSYSTEM, args_)
#define ICELogInfo(args_) ICELog(LL_INFO, LOG_SUBSYSTEM, args_)
#define ICELogDebug(args_) ICELog(LL_DEBUG, LOG_SUBSYSTEM, args_)
#define ICELogMedia(args_) ICELog(LL_MEDIA, LOG_SUBSYSTEM, args_)

#define ICELogCritical2(args_) ICELog(LogLevel_Critical, LogSubsystem.c_str(), args_)
#define ICELogInfo2(args_) ICELog(LogLevel_Info, LogSubsystem.c_str(), args_)
#define ICELogDebug2(args_) ICELog(LogLevel_Debug, LogSubsystem.c_str(), args_)
#define ICELogMedia2(args_) ICELog(LogLevel_Media, LogSubsystem.c_str(), args_)

#define DEFINE_LOGGING(subsystem) \
    static std::string LogSubsystem = subsystem; \
    static ice::LogLevel LogLevel_Critical = LL_CRITICAL; \
    static ice::LogLevel LogLevel_Info = LL_INFO; \
    static ice::LogLevel LogLevel_Debug = LL_DEBUG; \
    static ice::LogLevel LogLevel_Media = LL_MEDIA


/*
#define ICELogCritical(args_)
#define ICELogInfo(args_)
#define ICELogDebug(args_)
#define ICELogMedia(args_)
*/

} //end of namespace

#endif
