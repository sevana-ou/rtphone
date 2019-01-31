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
                                   std::function<void(const std::string& line)> line_callback,
                                   std::function<void(const std::string& reason)> finished_callback,
                                   bool& finish_flag);
#if defined(TARGET_OSX) || defined(TARGET_LINUX)
    static pid_t findPid(const std::string& cmdline);
    static void killByPid(pid_t pid);
#endif

};

#endif
