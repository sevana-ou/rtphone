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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define KEEP_SIZE (128 * 1024)  // 128 KB = 131,072 bytes

/**
 * Truncates a file to its last 128 KB of content.
 *
 * @param filename Path to the file to truncate
 * @return 0 on success, -1 on error (check errno for details)
 *
 * Behavior:
 * - Files ≤128 KB remain unchanged
 * - Files >128 KB are replaced with their last 128 KB
 * - Uses binary mode to avoid newline translation issues
 * - Preserves file existence but NOT original inode/permissions (reopens in write mode)
 */
int truncate_file_to_last_128k(const char *filename) {
    if (!filename || *filename == '\0') {
        errno = EINVAL;
        return -1;
    }

    // Step 1: Open file for reading (binary mode for exact byte handling)
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return -1;  // errno set by fopen
    }

    // Step 2: Get file size
    if (fseek(fp, 0, SEEK_END) != 0) {
        int err = errno;
        fclose(fp);
        errno = err;
        return -1;
    }

    long size = ftell(fp);
    if (size < 0) {
        int err = errno;
        fclose(fp);
        errno = err;
        return -1;
    }

    // Step 3: Nothing to do if file is small enough
    if (size <= KEEP_SIZE) {
        fclose(fp);
        return 0;
    }

    // Step 4: Position at start of region to keep (last 128 KB)
    long start_pos = size - KEEP_SIZE;
    if (fseek(fp, start_pos, SEEK_SET) != 0) {
        int err = errno;
        fclose(fp);
        errno = err;
        return -1;
    }

    // Step 5: Read the last 128 KB into buffer
    char *buffer = (char*)malloc(KEEP_SIZE);
    if (!buffer) {
        fclose(fp);
        errno = ENOMEM;
        return -1;
    }

    size_t read_bytes = fread(buffer, 1, KEEP_SIZE, fp);
    int read_err = ferror(fp);
    fclose(fp);

    if (read_bytes != KEEP_SIZE || read_err) {
        free(buffer);
        errno = read_err ? ferror(fp) : EIO;
        return -1;
    }

    // Step 6: Reopen file in write mode (truncates existing content)
    fp = fopen(filename, "wb");
    if (!fp) {
        free(buffer);
        return -1;  // errno set by fopen
    }

    // Step 7: Write preserved content back to file
    size_t written = fwrite(buffer, 1, KEEP_SIZE, fp);
    int write_err = ferror(fp);
    fclose(fp);
    free(buffer);

    if (written != KEEP_SIZE || write_err) {
        errno = write_err ? ferror(fp) : EIO;
        return -1;
    }

    return 0;
}

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
        mFile = nullptr;
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

    // Keep only last 128Kb
    truncate_file_to_last_128k(filepath);

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

const std::string& Logger::getLogPath() const {
    return mLogPath;
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
        mFile = nullptr;
    }

    mDelegate = nullptr;
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

    // remove(mLogPath.c_str());
    useFile(mLogPath.c_str());
}

std::string Logger::getLastContent(size_t bytes) {
    LogGuard l(mGuard);
    std::string r;

    bool fileOpenNeeded = false;
    if (mFile)
    {
        fclose(mFile); mFile = nullptr;
        fileOpenNeeded = true;
    }

    mFile = fopen(mLogPath.c_str(), "rt");

    if (mFile) {
        fseek(mFile, 0, SEEK_END);
        auto size = ftell(mFile);

        if (size < bytes)
            fseek(mFile, 0, SEEK_SET);
        else
            fseek(mFile, size - (long)bytes, SEEK_SET);

        auto contentPos = ftell(mFile);
        size_t contentLength = size - contentPos;
        if (bytes < contentLength)
            contentLength = bytes;

        r.resize(contentLength, ' ');
        auto readCount = fread(r.data(), 1, contentLength, mFile);
        if (readCount >= 0)
            r.resize(readCount);
        fclose(mFile); mFile = nullptr;
    }

    if (fileOpenNeeded)
    {
        mFile = fopen(mLogPath.c_str(), "at");
    }

    return r;
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
    *mStream << std::endl;
    *mStream << std::flush;
    mStream->flush();

    std::ostringstream result;
    std::chrono::milliseconds unix_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    time_t unix_timestamp = unix_timestamp_ms.count() / 1000;
    struct tm current_time = *gmtime(&unix_timestamp);
    char time_buffer[128]; strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &current_time);

    result << time_buffer << ":"  << (unix_timestamp_ms.count() % 1000 ) << "\t" << " | " << std::setw(8) << ThreadInfo::currentThread() << " | "
           << std::setw(40) << mFilename.c_str() << " | " << std::setw(4) << mLine << " | " << std::setw(12) << mSubsystem.c_str() << " | "
           << mStream->str().c_str();

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

    delete mStream; mStream = nullptr;
}

Logger& 
Logger::operator << (const char* data)
{
    *mStream << data;

    return *this;
}

/*Logger&
Logger::operator << (const wchar_t* data)
{
    *mStream << data;

    return *this;
}*/

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

Logger&
Logger::operator << (const std::string_view& data)
{
    *mStream << data;
    return *this;
}

Logger&
Logger::operator << (const std::filesystem::path& p)
{
    *mStream << p;
    return *this;
}

Logger&
Logger::operator << (const std::chrono::milliseconds t)
{
    *mStream << t.count() << "ms";
    return *this;
}
