/* Copyright(C) 2007-2023 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HL_String.h"
#include <sstream>
#include <iomanip>
#include <memory.h>
#include <algorithm>
#include <inttypes.h>

#ifdef TARGET_WIN
# include <WinSock2.h>
# include <Windows.h>
# include <cctype>
#endif

std::string strx::extractFilename(const std::string& path)
{
    if (path.empty())
        return std::string();

    // Look for separator from end of string
    std::string::size_type p_s = path.find_last_of('/'), p_bs = path.find_last_of('\\');
    if (p_s == std::string::npos && p_bs == std::string::npos)
        return path;
    if (p_s != std::string::npos)
        return path.substr(p_s + 1);
    else
        return path.substr(p_bs + 1);
}

std::string strx::appendPath(const std::string& s1, const std::string& s2)
{
    std::string result = s1;
    if (!endsWith(result, "/") && !endsWith(result, "\\"))
    {
#if defined(TARGET_WIN)
        result += "\\";
#else
        result += "/";
#endif
    }
    return result + s2;
}

std::string strx::makeUtf8(const std::tstring &arg)
{
#if defined(TARGET_WIN)
    int required = WideCharToMultiByte(CP_UTF8, 0, arg.c_str(), -1, NULL, 0, NULL, NULL);
    if (required <= 0)
        return std::string();
    std::string result(static_cast<size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, arg.c_str(), -1, &result[0], required, NULL, NULL);
    result.resize(strlen(result.c_str())); // strip the trailing NUL written by the API
    return result;
#else
    return arg;
#endif
}

std::string strx::toUtf8(const std::tstring &arg)
{
    return makeUtf8(arg);
}

std::tstring strx::makeTstring(const std::string& arg)
{
#if defined(TARGET_WIN)
    int count = MultiByteToWideChar(CP_UTF8, 0, arg.c_str(), -1, NULL, 0);
    if (count <= 0)
        return std::tstring();
    std::wstring result(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, arg.c_str(), -1, &result[0], count);
    result.resize(wcslen(result.c_str())); // strip the trailing NUL written by the API
    return result;
#else
    return arg;
#endif
}

int strx::toInt(const char *s, int defaultValue, bool* isOk)
{
    int result;
    if (sscanf(s, "%d", &result) != 1)
    {
        if (isOk)
            *isOk = false;
        result = defaultValue;
    }
    else
        if (isOk)
            *isOk = true;

    return result;
}

uint64_t strx::toUint64(const char* s, uint64_t def, bool *isOk)
{
    uint64_t result = def;
    if (sscanf(s, "%" SCNu64, &result) != 1)
    {
        if (isOk)
            *isOk = false;
        result = def;
    }
    else
        if (isOk)
            *isOk = true;

    return result;
}

std::string strx::toHex(unsigned int value)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%x", value);
    return buffer;
}

std::string strx::toHex(const void *ptr)
{
    std::ostringstream oss;
    oss << std::fixed << std::setw(8) << std::setfill('0') << std::hex << ptr;
    return oss.str();
}

//must be lowercase for MD5
static const char hexmap[] = "0123456789abcdef";

std::string strx::toHex(const uint8_t* input, size_t inputLength)
{
    std::string result; result.resize(inputLength * 2);

    const char* p = (const char*)input;
    char* r = &result[0];
    for (size_t i=0; i < inputLength; ++i)
    {
        unsigned char temp = *p++;

        int hi = (temp & 0xf0)>>4;
        int low = (temp & 0xf);

        *r++ = hexmap[hi];
        *r++ = hexmap[low];
    }

    return result;
}

std::string strx::prefixLines(const std::string &source, const std::string &prefix)
{
    // Read source line by line
    std::istringstream iss(source);
    std::ostringstream oss;
    std::string line;
    while (std::getline(iss,line))
    {
        oss << prefix << line << std::endl;
    }
    return oss.str();
}

std::string strx::doubleToString(double value, int precision)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}


const char* strx::findSubstring(const char* buffer, const char* substring, size_t bufferLength)
{
    // The buffer is not necessarily NUL-terminated, so a bounded search is
    // required on every platform (a memmem replacement for MSVC is provided below).
    return (const char*)memmem(buffer, bufferLength, substring, strlen(substring));
}


void strx::split(const std::string& src, std::vector<std::string>& dst, const std::string& delims)
{
    dst.clear();
    std::string::size_type p = 0;
    while (p < src.size())
    {
        std::string::size_type f = src.find_first_of(delims, p);
        if (f == std::string::npos)
        {
            std::string t = src.substr(p);
            if (!t.empty())
                dst.push_back(t);
            p = src.size();
        }
        else
        {
            std::string t = src.substr(p, f-p);
            if (!t.empty())
                dst.push_back(t);
            p = f + 1;
        }
    }
}

std::vector<std::string> strx::split(const std::string& src, const std::string& delims)
{
    std::vector<std::string> r;
    split(src, r, delims);
    return r;
}

std::pair<std::string, int> strx::parseHost(const std::string& host, int defaultPort)
{
    std::pair<std::string, int> result;
    std::size_t p = host.find(':');
    if (p != std::string::npos)
    {
        result.first = host.substr(0, p);
        result.second = strx::toInt(host.c_str() + p + 1, defaultPort);
    }
    else
    {
        result.first = host;
        result.second = defaultPort;
    }
    return result;
}

std::pair<std::string, std::string> strx::parseAssignment(const std::string& s, bool trimQuotes)
{
    std::pair<std::string, std::string> result;

    std::string::size_type p = s.find('=');
    if (p != std::string::npos)
    {
        result.first = strx::trim(s.substr(0, p));
        result.second = strx::trim(s.substr(p+1));
        if (trimQuotes && result.second.size() >= 2)
        {
            if ((result.second[0] == '"' && result.second[result.second.size()-1] == '"') ||
                    (result.second[0] == '\'' && result.second[result.second.size()-1] == '\''))
                result.second = result.second.substr(1, result.second.size() - 2);
        }
    }
    else
        result.first = strx::trim(s);

    return result;
}

std::string strx::intToString(int value)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%d", value);
    return buffer;
}

float strx::toFloat(const std::string &s, float v, bool* isOk)
{
    float result = 0.0;
    int code = sscanf(s.c_str(), "%f", &result);
    if (code == 1)
    {
        if (isOk)
            *isOk = true;
    }
    else
    {
        result = v;
        if (isOk)
            *isOk = false;
    }

    return result;
}

std::string strx::trim(const std::string &s)
{
    auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c) { return std::isspace(c) || c == '\r' || c == '\n'; });
    auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c) { return std::isspace(c) || c == '\r' || c == '\n'; }).base();
    return (wsback <= wsfront ? std::string() : std::string(wsfront,wsback));
}

std::string strx::timeToString(time_t t)
{
    char buffer[128] = "";
    struct tm lt;
#if defined(TARGET_LINUX) || defined(TARGET_OSX) || defined(TARGET_ANDROID)
    if (localtime_r(&t, &lt) == nullptr)
        return std::string();
#else
    lt = *localtime(&t);
#endif
    strftime(buffer, sizeof(buffer)-1, "%Y-%m-%d %H:%M:%S", &lt);
    return buffer;
}

std::string strx::millisecondsToString(uint64_t t)
{
    return timeToString(t/1000);
}

int strx::fromHex2Int(const std::string &s)
{
    int result = 0;
    int retcode = sscanf(s.c_str(), "%x", &result);
    if (retcode == 0)
        return 0;

    return result;
}

static int hex2code(char s)
{
    if (s >= '0' && s <= '9')
        return s - '0';
    if (s >= 'a' && s <= 'f')
        return s - 'a' + 10;
    if (s >= 'A' && s <= 'F')
        return s - 'A' + 10;
    return 0;
}

/*static int hex2code(const char* s)
{
    return (hex2code(s[0]) << 4) + hex2code(s[1]);
}*/

std::string strx::fromHex2String(const std::string& s)
{
    std::string result; result.resize(s.size() / 2);
    const char* t = s.c_str();
    for (size_t i = 0; i < result.size(); i++)
        result[i] = static_cast<char>((hex2code(t[i*2]) << 4) | hex2code(t[i*2+1]));

    return result;
}

std::string strx::replace(const std::string& s, char f, char r)
{
    std::string result(s);
    for (std::string::size_type i = 0; i < result.size(); i++)
        if (result[i] == f)
            result[i] = r;

    return result;
}

std::string strx::replace(const std::string& s, const std::string& tmpl, const std::string& n)
{
    std::string result(s);
    std::string::size_type p = 0;
    while ( (p = result.find(tmpl, p)) != std::string::npos)
    {
        result.replace(p, tmpl.size(), n);
        p += n.size();
    }

    return result;
}

std::string strx::decodeUri(const std::string& s)
{
    std::string ret;
    ret.reserve(s.size());

    char ch;

    int i, ii = 0;
    for (i=0; i<(int)s.length(); i++)
    {
        if (s[i] == '%' && i + 2 < (int)s.length())
        {
            if (sscanf(s.substr(i+1,2).c_str(), "%x", &ii) == 1)
            {
                ch = static_cast<char>(ii);
                ret += ch;
                i += 2;
            }
            else
                ret += s[i];
        }
        else
            ret += s[i];
    }
    return ret;
}

bool strx::startsWith(const std::string& s, const std::string& prefix)
{
    if (prefix.size() > s.size())
        return false;
    return s.compare(0, prefix.size(), prefix) == 0;
}

bool strx::endsWith(const std::string& s, const std::string& suffix)
{
    if (suffix.size() > s.size())
        return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int strx::stringToDuration(const std::string& s)
{
    if (endsWith(s, "ms"))
        return std::stoi(s.substr(0, s.size()-2));
    if (endsWith(s, "s"))
        return std::stoi(s.substr(0, s.size()-1)) * 1000;
    if (endsWith(s, "m"))
        return std::stoi(s.substr(0, s.size()-1)) * 60000;
    if (endsWith(s, "h"))
        return std::stoi(s.substr(0, s.size()-1)) * 3600 * 1000;
    else
        return std::stoi(s) * 1000;
}

std::string strx::uppercase(const std::string& s)
{
    std::string r(s);
    std::transform(r.begin(), r.end(), r.begin(), ::toupper);
    return r;
}

std::string strx::lowercase(const std::string& s)
{
    std::string r(s);
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

std::string strx::removeQuotes(const std::string& s)
{
    std::string r(s);
    if (s.empty())
        return s;

    if (r.front() == '"')
        r = r.substr(1);

    if (r.back() == '"')
        r = r.substr(0, r.size()-1);

    return r;
}

#if defined(TARGET_WIN)

// MSVC++ lacks memmem support
const void *memmem(const void *haystack, size_t haystack_len,
                   const void * const needle, const size_t needle_len)
{
    if (!haystack || !haystack_len || !needle || !needle_len)
        return nullptr;

    for (const char *h = (const char*)haystack;
         haystack_len >= needle_len;
         ++h, --haystack_len) {
        if (!memcmp(h, needle, needle_len)) {
            return h;
        }
    }
    return nullptr;
}
#endif

