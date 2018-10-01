#ifndef __HL_PROCESS_H
#define __HL_PROCESS_H

#include <string>
#include <functional>
#include <memory>
#include <thread>

class OsProcess
{
public:
    static std::string execCommand(const std::string& cmdline);
    static std::shared_ptr<std::thread> asyncExecCommand(const std::string& cmdline,
                                   std::function<void(const std::string& line)> callback,
                                   bool& finish_flag);

};

#endif
