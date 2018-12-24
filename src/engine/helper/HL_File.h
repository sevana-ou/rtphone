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
};

#endif
