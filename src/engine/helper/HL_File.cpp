#include "HL_File.h"
#include <fstream>

bool FileHelper::exists(const std::string& s)
{
    std::ifstream ifs(s);
    return !ifs.bad();
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
    char buffer[L_tmpnam];
    tmpnam(buffer);

    return buffer;
}
