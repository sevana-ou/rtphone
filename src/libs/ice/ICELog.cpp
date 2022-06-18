/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICELog.h"
#include "ICETime.h"
#include "ICESync.h"
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <chrono>
#if defined(__ANDROID_API__)
# include <android/log.h>
#endif

using namespace ice;


LogLevel LogLevelHelper::parse(const std::string& t)
{
    if (t == "none")
        return LL_NONE;
    else
    if (t == "debug")
        return LL_DEBUG;
    else
    if (t == "critical")
        return  LL_CRITICAL;
    else
    if (t == "info")
        return LL_INFO;
    else
    if (t == "media")
        return LL_MEDIA;
    else
    if (t == "error")
        return LL_ERROR;
    else
    if (t == "special")
        return LL_SPECIAL;
    else
    if (t == "media")
        return LL_MEDIA;

    throw std::runtime_error("Bad log param string value.");
}

std::string LogLevelHelper::toString(LogLevel level)
{
    switch (level)
    {
    case LL_NONE:       return "none";
    case LL_CRITICAL:   return "critical";
    case LL_DEBUG:      return "debug";
    case LL_ERROR:      return "error";
    case LL_INFO:       return "info";
    case LL_MEDIA:      return "media";
    case LL_SPECIAL:    return "special";
    }

    throw std::runtime_error("Bad log param value.");
}

ice::Logger ice::GLogger;
const char* ice::Logger::TabPrefix = "  ";

Logger::Logger()
    :mStream(nullptr)
{
    mFile = nullptr;
    mUseDebugWindow = false;
    mDelegate = nullptr;
    mLevel = LL_DEBUG;
}

Logger::~Logger()
{
    //LogGuard l(mGuard);
    if (mFile)
    {
        fclose(mFile);
        mFile = NULL;
    }
}

void 
Logger::useDebugWindow(bool enable)
{
    LogGuard l(mGuard);

    mUseDebugWindow = enable;
}

void
Logger::useFile(const char* filepath)
{
    LogGuard l(mGuard);

    mLogPath = filepath ? filepath : "";
    if (mLogPath.empty())
        return;

    FILE* f = fopen(filepath, "at");
    if (f)
    {
        if (mFile)
            fclose(mFile);
        mFile = f;
    }

    if (f)
    {
        fprintf(f, "New log chunk starts here.\n");
        fflush(f);
    }
}

void 
Logger::useNull()
{
    LogGuard l(mGuard);

    mUseDebugWindow = false;
    if (mFile)
    {
        fflush(mFile);
        fclose(mFile);
        mFile = NULL;
    }

    mDelegate = NULL;
}

void Logger::closeFile()
{
    LogGuard l(mGuard);
    if (mFile)
    {
        fflush(mFile);
        fclose(mFile);
        mFile = nullptr;
    }
}

void Logger::openFile()
{
    LogGuard l(mGuard);
    if (mLogPath.empty())
        return;

    remove(mLogPath.c_str());
    useFile(mLogPath.c_str());
}


void Logger::useDelegate(LogHandler* delegate_)
{
    mDelegate = delegate_;
}

LogGuard& Logger::mutex()
{
    return mGuard;
}

LogLevel Logger::level()
{
    return mLevel;
}

void Logger::setLevel(LogLevel level)
{
    mLevel = level;
}

void Logger::beginLine(LogLevel level, const char* filename, int linenumber, const char* subsystem)
{
    mMsgLevel = level;
    mStream = new std::ostringstream();

#ifdef WIN32
    const char* filenamestart = strrchr(filename, '\\');
#else
    const char* filenamestart = strrchr(filename, '/');
#endif	
    if (!filenamestart)
        filenamestart = filename;
    else
        filenamestart++;

    mFilename = filenamestart;
    mLine = linenumber;
    mSubsystem = subsystem;

    // mStream << std::setw(8) << ICETimeHelper::timestamp() << " | " << std::setw(8) << ThreadInfo::currentThread() << " | " << std::setw(30) << filenamestart << " | " << std::setw(4) << linenumber << " | " << std::setw(12) << subsystem << " | ";
}

void
Logger::endLine()
{
    *mStream << std::flush;
    mStream->flush();

    std::ostringstream result;
    std::chrono::milliseconds unix_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    time_t unix_timestamp = unix_timestamp_ms.count() / 1000;
    struct tm current_time = *gmtime(&unix_timestamp);
    char time_buffer[128]; strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &current_time);

    result << time_buffer << ":"  << (unix_timestamp_ms.count() % 1000 ) << "\t" << " | " << std::setw(8) << ThreadInfo::currentThread() << " | " << std::setw(30)
           << mFilename.c_str() << " | " << std::setw(4) << mLine << " | " << std::setw(12) << mSubsystem.c_str() << " | "
           << mStream->str().c_str() << std::endl;

    std::string t = result.str();
    if (mUseDebugWindow) {
#ifdef TARGET_WIN
        OutputDebugStringA(t.c_str());
#elif defined(TARGET_ANDROID)
        if (t.size() > 512) {
            std::string cut = t;
            cut.erase(480); // Erase tail of string
            cut += "\r\n... [cut]";
            __android_log_print(ANDROID_LOG_INFO, "VoipAgent", "%s", cut.c_str());
        } else {
            __android_log_print(ANDROID_LOG_INFO, "VoipAgent", "%s", t.c_str());
        }
#else
        std::cerr << result.str() << std::endl << std::flush;
#endif
    }
    if (mFile)
    {
        fprintf(mFile, "%s", result.str().c_str());
        fflush(mFile);
    }
    if (mDelegate)
        mDelegate->onIceLog(mMsgLevel, mFilename, mLine, mSubsystem, mStream->str());

    delete mStream; mStream = NULL;
}

Logger& 
Logger::operator << (const char* data)
{
    *mStream << data;

    return *this;
}

Logger& 
Logger::operator << (const wchar_t* data)
{
    *mStream << data;

    return *this;
}

Logger& 
Logger::operator << (const int data)
{
    *mStream << data;

    return *this;
}

Logger& 
Logger::operator << (const float data)
{
    *mStream << data;

    return *this;
}

Logger&
Logger::operator<<(const int64_t data)
{
    *mStream << data;
    return *this;
}

Logger&
Logger::operator<<(const unsigned int data)
{
    *mStream << data;
    return *this;
}


Logger&
Logger::operator<<(const uint64_t data)
{
    *mStream << data;
    return *this;
}


Logger& 
Logger::operator << (const std::string& data)
{
    *mStream << data.c_str();
    return *this;
}
