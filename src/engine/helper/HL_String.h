/* Copyright(C) 2007-2018 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __HELPER_STRING_H
#define __HELPER_STRING_H

#include <vector>
#include <string>
#include <sstream>
#include <stdint.h>
#include "HL_Types.h"

#ifdef TARGET_OSX
#define stricmp strcasecmp
#endif

#include <cstdint>

class strx
{
public:
    static std::string  makeUtf8(const std::tstring& arg);
    static std::string  toUtf8(const std::tstring& arg);
    static std::tstring makeTstring(const std::string& arg);

    static int          toInt(const char* s, int defaultValue, bool* isOk = nullptr);
    static uint64_t     toUint64(const char* s, uint64_t def, bool *isOk = nullptr);
    static std::string  toHex(unsigned int value);
    static std::string  toHex(const void* ptr);
    static std::string  toHex(const uint8_t* input, size_t inputLength);
    static std::string  intToString(int value);
    static std::string  doubleToString(double value, int precision);
    static int          fromHex2Int(const std::string& s);
    static std::string  fromHex2String(const std::string& s);
    static float        toFloat(const std::string& s, float defaultValue = 0.0, bool* isOk = nullptr);

    static std::string  extractFilename(const std::string& path);
    static std::string  appendPath(const std::string& s1, const std::string& s2);

    static std::string  prefixLines(const std::string& source, const std::string& prefix);
    static const char* findSubstring(const char* buffer, const char* substring, size_t bufferLength);

    static void         split(const std::string& src, std::vector<std::string>& dst, const std::string& delims);
    static std::vector<std::string> split(const std::string& src, const std::string& delims = "\n");

    template <typename T>
    static std::string join(const std::vector<T>& v, const std::string& delimiter)
    {
        std::ostringstream s;
        for (const auto& i : v)
        {
            if (&i != &v[0])
                s << delimiter;
            s << i;
        }
        return s.str();
    }

    static std::pair<std::string, int> parseHost(const std::string& host, int defaultPort);
    static std::pair<std::string, std::string> parseAssignment(const std::string& s, bool trimQuotes = true);
    static std::string  trim(const std::string& s);
    static std::string  timeToString(time_t t);
    static std::string  millisecondsToString(uint64_t t);
    static std::string  replace(const std::string& s, char f, char r);
    static std::string  replace(const std::string& s, const std::string& tmpl, const std::string& n);
    static std::string  decodeUri(const std::string& s);
    static bool         startsWith(const std::string& s, const std::string& prefix);
    static bool         endsWith(const std::string& s, const std::string& suffix);
    static int          stringToDuration(const std::string& s);
    static std::string  uppercase(const std::string& s);
    static std::string  removeQuotes(const std::string& s);

};

class XcapHelper
{
public:
    static std::string buildBuddyList(std::string listName, std::vector<std::string> buddies);
    static std::string buildRules(std::vector<std::string> buddies);
    static std::string buildServices(std::string serviceUri, std::string listRef);
    static std::string normalizeSipUri(std::string uri);
};


#if defined(TARGET_WIN)

// MSVC++ lacks memmem support
extern const void *memmem(const void *haystack, size_t haystack_len,
                          const void * const needle, const size_t needle_len);
#endif

#endif
