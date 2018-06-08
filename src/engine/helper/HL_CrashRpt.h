#ifndef __CRASH_RPT_H
#define __CRASH_RPT_H

#include <string>

#if defined(TARGET_WIN)
# include <WinSock2.h>
# include <Windows.h>
#endif

// Helper class to translate SEH exceptions to C++ - sometimes it is needed
#if defined(TARGET_WIN)

class SE_Exception
{
private:
  unsigned int nSE;
public:
  SE_Exception() {}
  SE_Exception(unsigned int n) : nSE(n) {}
  ~SE_Exception() {}
  unsigned int getSeNumber() { return nSE; }
};

extern void SEHToCpp(unsigned int, EXCEPTION_POINTERS*);

// Although better way is to have _set_se_translator set - in our case we do not call it.
// The cause is usage of CrashRpt libraries - it gives better control on exception reporting.
# define SET_SEH_TO_CPP 
//_set_se_translator(&SEHToCpp)
#else
# define SET_SEH_TO_CPP
#endif

class CrashReporter
{
public:
  static void init(const std::string& appname, const std::string& version = "", const std::string& url = "");
  static void free();
  static void initThread();
  static void freeThread();
  static bool isLoaded();

#ifdef TARGET_WIN
  static BOOL WINAPI Callback(LPVOID /*lpvState*/);
#endif
};

// RAII class to notify crash reporter about thread start/stop
class CrashReporterThreadPoint
{
public:
  CrashReporterThreadPoint();
  ~CrashReporterThreadPoint();
};

class CrashReporterGuard
{
public:
  CrashReporterGuard();
  ~CrashReporterGuard();
};

#endif
