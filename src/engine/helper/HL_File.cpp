#include "HL_File.h"
#include <fstream>

#if defined(TARGET_LINUX) || defined(TARGET_OSX)
# include <unistd.h>
# include <sys/statvfs.h>
# include <memory.h>
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
#if defined(TARGET_LINUX)
    char template_filename[L_tmpnam] = "rtphone_XXXXXXX.tmp";
    mkstemp(template_filename);
    return template_filename;
#elif defined(TARGET_WIN)
    char buffer[L_tmpnam];
    tmpnam(buffer);

    return buffer;
#elif defined(TARGET_OSX)
    char template_filename[L_tmpnam] = "rtphone_XXXXXXX.tmp";
    mktemp(template_filename);
    return template_filename;
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
    char buf[512];
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

// Returns free space on volume for path
size_t FileHelper::getFreespace(const std::string& path)
{
    size_t r = static_cast<size_t>(-1);

#if defined(TARGET_LINUX)
    struct statvfs stats; memset(&stats, 0, sizeof stats);

    int retcode = statvfs(path.c_str(), &stats);
    if (retcode == 0)
        r = stats.f_bfree * stats.f_bsize;
#endif
    return r;
}
