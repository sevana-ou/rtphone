#ifndef __HL_PROCESS_H
#define __HL_PROCESS_H

#include <string>
#include <functional>

class OsProcess
{
public:
    static std::string execCommand(const std::string& cmdline);
    static void asyncExecCommand(const std::string& cmdline,
                                   std::function<void(const std::string& line)> callback,
                                   bool& finish_flag);

};

#endif
