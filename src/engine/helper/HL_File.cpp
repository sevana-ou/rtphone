#include "HL_File.h"
#include <fstream>

#if defined(TARGET_LINUX) || defined(TARGET_OSX)
# include <unistd.h>
#endif

bool FileHelper::exists(const std::string& s)
{
#if defined(TARGET_WIN)
    std::ifstream ifs(s);
    return !ifs.bad();
#else
    return (access(s.c_str(), R_OK) != -1);
#endif
}

bool FileHelper::exists(const char* s)
{
    return exists(std::string(s));
}

void FileHelper::remove(const std::string& s)
{
    ::remove(s.c_str());
}

void FileHelper::remove(const char* s)
{
    ::remove(s);
}

std::string FileHelper::gettempname()
{
#if defined(TARGET_LINUX) || defined(TARGET_WIN)
    char buffer[L_tmpnam];
    tmpnam(buffer);

    return buffer;
#elif defined(TARGET_OSX)
    char template_filename[L_tmpnam] = "rtphone_XXXXXXX.tmp";
    int handle = mkstemp(template_filename);
    if (handle != -1)
    {
        close(handle);
        return template_filename;
    }
    else
        return std::string();
#endif
}

bool FileHelper::isAbsolute(const std::string& s)
{
    if (s.empty())
        return false;

    return s.front() == '/' || s.front() == '\\';
}

std::string FileHelper::getCurrentDir()
{
#if defined(TARGET_WIN)
    return std::string();
#endif

#if defined(TARGET_LINUX) || defined(TARGET_OSX)
    char buffer[512];
    if (getcwd(buf, sizeof buf) != nullptr)
        return buf;
    else
        return std::string();
#endif
}

std::string FileHelper::addTrailingSlash(const std::string& s)
{
    if (s.empty())
        return "/";

    if (s.back() == '/' || s.back() == '\\')
        return s;

    return s + "/";
}

std::string FileHelper::mergePathes(const std::string& s1, const std::string& s2)
{
    std::string result(addTrailingSlash(s1));
    if (isAbsolute(s2))
        result += s2.substr(1, s2.size() - 1);
    else
        result += s2;

    return result;
}
