#ifndef __HL_PROCESS_H
#define __HL_PROCESS_H

#include <string>

class OsProcess
{
public:
    static std::string execCommand(const std::string& cmdline);
};

#endif