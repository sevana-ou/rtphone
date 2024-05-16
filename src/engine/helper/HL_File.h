#ifndef __HL_FILE_H
#define __HL_FILE_H

#include <string>

class FileHelper
{
public:
    static bool exists(const std::string& s);
    static bool exists(const char* s);

    static void remove(const std::string& s);
    static void remove(const char* s);

    static std::string gettempname();
    static bool isAbsolute(const std::string& s);

    static std::string getCurrentDir();

    static std::string addTrailingSlash(const std::string& s);
    static std::string mergePathes(const std::string& s1, const std::string& s2);

    // Returns free space on volume for path
    // Works for Linux only. For other systems (size_t)-1 is returned (for errors too)
    static size_t getFreespace(const std::string& path);
    static std::string expandUserHome(const std::string& path);
};

#endif
