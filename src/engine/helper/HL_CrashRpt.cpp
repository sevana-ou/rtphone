#include "HL_CrashRpt.h"
#include "HL_String.h"


// Define this if CrashRpt has to be deployed as .dll
// #define CRASHRPT_DYNAMIC

// --- CrashReporer ---
#if defined(TARGET_WIN)
#include "CrashRpt.h"

BOOL WINAPI CrashReporter::Callback(LPVOID arg)
{
  return TRUE;
}

typedef int(__stdcall *CrInstallProc)(__in PCR_INSTALL_INFOW pInfo);
static CrInstallProc CrInstall = nullptr;

typedef int(__stdcall *CrUninstallProc)();
static CrUninstallProc CrUninstall = nullptr;

typedef int(__stdcall *CrInstallIntoCurrentThreadProc)(DWORD dwFlags);
static CrInstallIntoCurrentThreadProc CrInstallIntoCurrentThread = nullptr;

typedef int(__stdcall *CrUninstallFromCurrentThreadProc)();
static CrUninstallFromCurrentThreadProc CrUninstallFromCurrentThread = nullptr;

typedef int(__stdcall *CrGetLastErrorMsgProc)(LPWSTR buffer, UINT size);
static CrGetLastErrorMsgProc CrGetLastErrorMsg = nullptr;

static HMODULE CrLibraryHandle = NULL;
#endif

void CrashReporter::init(const std::string& appname, const std::string& version, const std::string& url)
{
#if defined(TARGET_WIN)
#if defined(CRASHRPT_DYNAMIC)
  // Check if DLL functions are here
  if (CrLibraryHandle)
    return; // Library is loaded already - so initialized already

  CrLibraryHandle = ::LoadLibrary(TEXT("crashrpt.dll"));
  if (!CrLibraryHandle)
    return; // No logging here - initialization happens on very first stages, no chance to log anything
  
  CrInstall = (CrInstallProc)::GetProcAddress(CrLibraryHandle, "crInstallW");
  CrUninstall = (CrUninstallProc)::GetProcAddress(CrLibraryHandle, "crUninstall");
  CrInstallIntoCurrentThread = (CrInstallIntoCurrentThreadProc)::GetProcAddress(CrLibraryHandle, "crInstallToCurrentThread2");
  CrUninstallFromCurrentThread = (CrUninstallFromCurrentThreadProc)::GetProcAddress(CrLibraryHandle, "crUninstallFromCurrentThread");
  CrGetLastErrorMsg = (CrGetLastErrorMsgProc)::GetProcAddress(CrLibraryHandle, "crGetLastErrorMsgW");
#else
  CrInstall = &crInstallW;
  CrUninstall = &crUninstall;
  CrInstallIntoCurrentThread = &crInstallToCurrentThread2;
  CrUninstallFromCurrentThread = &crUninstallFromCurrentThread;
  CrGetLastErrorMsg = &crGetLastErrorMsgW;
#endif

  if (!isLoaded())
    return;

  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO)); 
  info.cb = sizeof(CR_INSTALL_INFO);
    struct
    {
        std::wstring appname, version, url;
    } unicode;
    unicode.appname = StringHelper::makeTstring(appname),
    unicode.version = StringHelper::makeTstring(version),
    unicode.url = StringHelper::makeTstring(url);

    if (unicode.appname.empty())
        unicode.appname = L"App";

    if (unicode.url.empty())
        unicode.url = L"https://voipobjects.com/crashrpt/crashrpt.php";

    if (unicode.version.empty())
        unicode.version = L"General version";

  info.pszAppName = unicode.appname.c_str();
  info.pszAppVersion = unicode.version.c_str();
  info.pszEmailSubject = TEXT("Crash report");
  //info.pszEmailTo = L"amegyeri@minerva-soft.com";
  //info.pszUrl = L"http://ftp.minerva-soft.com/crashlog/crashrpt.php";  
  //info.pszUrl = L"http://sip.crypttalk.com/crashlog/crashrpt.php";
  info.pszUrl = unicode.url.c_str();
  info.pfnCrashCallback = Callback;
  info.uPriorities[CR_HTTP] = 1;
  info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY;
  info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY;
  info.dwFlags = 0;
  info.pszCrashSenderPath = TEXT(".");

  int nResult = CrInstall(&info);
  if (nResult)
  {
    wchar_t errorMsg[512] = L"";        
    CrGetLastErrorMsg(errorMsg, 512); 
    OutputDebugStringW(errorMsg);
    //LogCritical("Core", << "Failed to install CrashReporter with code " << nResult << " and message " << errorMsg);
  }
#endif
}

void CrashReporter::free()
{
#if defined(TARGET_WIN)
  if (isLoaded())
  {
    CrUninstall();
    CrInstall = nullptr;
    CrUninstall = nullptr;
    CrInstallIntoCurrentThread = nullptr;
    CrUninstallFromCurrentThread = nullptr;
    CrGetLastErrorMsg = nullptr;
#if defined(CRASHRPT_DYNAMIC)
    ::FreeLibrary(CrLibraryHandle); CrLibraryHandle = NULL;
#endif
  }
#endif
}

bool CrashReporter::isLoaded()
{
#if defined(TARGET_WIN)
  return !(!CrInstall || !CrUninstall || !CrGetLastErrorMsg ||
    !CrInstallIntoCurrentThread || !CrUninstallFromCurrentThread);
#else
  return false;
#endif
}

void CrashReporter::initThread()
{
#if defined(TARGET_WIN)
  if (isLoaded())
    CrInstallIntoCurrentThread(0);
#endif
}

void CrashReporter::freeThread()
{
#if defined(TARGET_WIN)
  if (isLoaded())
    CrUninstallFromCurrentThread();
#endif
}

CrashReporterThreadPoint::CrashReporterThreadPoint()
{
  CrashReporter::initThread();
}

CrashReporterThreadPoint::~CrashReporterThreadPoint()
{
  CrashReporter::freeThread();
}

CrashReporterGuard::CrashReporterGuard()
{
  CrashReporter::init("generic");
}

CrashReporterGuard::~CrashReporterGuard()
{
  CrashReporter::free();
}
