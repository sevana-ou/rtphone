#include "HL_Process.h"
#include <thread>
#include <memory>

#ifdef TARGET_WIN
# define popen _popen
# define pclose _pclose
#endif

#if defined(TARGET_WIN)
#include <Windows.h>
#include <iostream>
#include "helper/HL_String.h"

int OsProcess::execSystem(const std::string& cmd)
{
    return system(cmd.c_str());
}

std::string OsProcess::execCommand(const std::string& cmd)
{
    std::string output;
    HANDLE hPipeRead, hPipeWrite;

    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
    saAttr.bInheritHandle = TRUE;   //Pipe handles are inherited by child process.
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe to get results from child's stdout.
    if ( !CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0) )
        return output;

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdOutput  = hPipeWrite;
    si.hStdError   = hPipeWrite;
    si.wShowWindow = SW_HIDE;       // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.

    PROCESS_INFORMATION pi  = { 0 };

    char* cmdline = (char*)_alloca(cmd.size()+1);
    strcpy(cmdline, strx::replace(cmd, "/", "\\").c_str());

    BOOL fSuccess = CreateProcessA( nullptr, cmdline, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
    if (! fSuccess)
    {
        CloseHandle( hPipeWrite );
        CloseHandle( hPipeRead );
        return output;
    }

    bool bProcessEnded = false;
    for (; !bProcessEnded ;)
    {
        // Give some timeslice (50ms), so we won't waste 100% cpu.
        bProcessEnded = WaitForSingleObject( pi.hProcess, 50) == WAIT_OBJECT_0;

        // Even if process exited - we continue reading, if there is some data available over pipe.
        for (;;)
        {
            char buf[1024];
            DWORD dwRead = 0;
            DWORD dwAvail = 0;

            if (!::PeekNamedPipe(hPipeRead, NULL, 0, NULL, &dwAvail, NULL))
                break;

            if (!dwAvail) // no data available, return
                break;

            if (!::ReadFile(hPipeRead, buf, min(sizeof(buf) - 1, dwAvail), &dwRead, NULL) || !dwRead)
                // error, the child process might ended
                break;

            buf[dwRead] = 0;
            output += buf;
        }
    } //for

    CloseHandle( hPipeWrite );
    CloseHandle( hPipeRead );
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    return output;
}

std::shared_ptr<std::thread> OsProcess::asyncExecCommand(const std::string& cmdline,
                                                    std::function<void(const std::string& line)> callback,
                                                    std::function<void(const std::string& reason)> finished_callback,
                                                    bool& finish_flag)
{
    // std::cout << cmdline << std::endl;

    std::string output;
    HANDLE hPipeRead, hPipeWrite;

    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), nullptr, FALSE };
    saAttr.bInheritHandle = TRUE;   //Pipe handles are inherited by child process.
    saAttr.lpSecurityDescriptor = nullptr;
    
    // Create a pipe to get results from child's stdout.
    if ( !CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0) )
        return std::shared_ptr<std::thread>();

    STARTUPINFOA si; memset(&si, 0, sizeof si);
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdOutput  = hPipeWrite;
    si.hStdError   = hPipeWrite;
    si.wShowWindow = SW_HIDE;       // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof pi);

    char* cmdbuffer = (char*)_alloca(cmdline.size()+1);
    strcpy(cmdbuffer, strx::replace(cmdline, "/", "\\").c_str());


    BOOL fSuccess = CreateProcessA( nullptr, cmdbuffer, nullptr, nullptr, TRUE,
                                    CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);
    if (! fSuccess)
    {
        CloseHandle( hPipeWrite );
        CloseHandle( hPipeRead );
        return std::shared_ptr<std::thread>();
    }

    std::shared_ptr<std::thread> r = std::make_shared<std::thread>(
                [&finish_flag, pi, callback, finished_callback, hPipeRead, hPipeWrite]()
    {
        char buf[4096]; memset(buf, 0, sizeof buf);
        for (; !finish_flag ;)
        {
            // Give some timeslice (50ms), so we won't waste 100% cpu.
            bool timeouted = WaitForSingleObject( pi.hProcess, 50) == WAIT_OBJECT_0;

            // Even if process exited - we continue reading, if there is some data available over pipe.
            for (;;)
            {
                DWORD dwRead = 0;
                DWORD dwAvail = 0;

                if (!::PeekNamedPipe(hPipeRead, nullptr, 0, nullptr, &dwAvail, nullptr))
                    break;

                if (!dwAvail) // no data available, return
                    break;

                int filled = strlen(buf);
                if (!::ReadFile(hPipeRead, buf + filled, min(sizeof(buf) - 1 - filled, dwAvail), &dwRead, nullptr) || !dwRead)
                    // error, the child process might ended
                    break;

                buf[dwRead] = 0;

                // Split to lines and send to callback
                const char* cr;
                while ((cr = strchr(buf, '\n')) != nullptr)
                {
                    std::string line(buf, cr - buf -1);
                    if (callback)
                        callback(strx::trim(line));
                    memmove(buf, cr + 1, strlen(cr+1) + 1);
                }
            }
        } //for

        if (buf[0])
            callback(strx::trim(std::string(buf)));

        char ctrlc = 3;
        //if (finish_flag)
        //  ::WriteFile(hPipeWrite, &ctrlc, 1, nullptr, nullptr);

        //    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pi.dwProcessId);
        
        CloseHandle( hPipeWrite );
        CloseHandle( hPipeRead );
        if (finish_flag)
        {
            //GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
            // Close underlying process
            //TerminateProcess(pi.hProcess, 3);
        }
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
        if (finished_callback)
            finished_callback(std::string());
    });

    return r;
}

#else

#include <memory>
#include <stdexcept>
#include <sys/select.h>
#include <fcntl.h>

std::string OsProcess::execCommand(const std::string& cmd)
{
    std::string cp = cmd;
    std::shared_ptr<FILE> pipe(popen(cp.c_str(), "r"), pclose);
    if (!pipe)
        throw std::runtime_error("Failed to run.");

    char buffer[1024];
    std::string result = "";
    while (!feof(pipe.get()))
    {
        if (fgets(buffer, 1024, pipe.get()) != nullptr)
            result += buffer;
    }
    return result;
}

int OsProcess::execSystem(const std::string& cmd)
{
    return system(cmd.c_str());
}

#include <poll.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>
#include "helper/HL_String.h"
#include "helper/HL_Sync.h"

std::shared_ptr<std::thread> OsProcess::asyncExecCommand(const std::string& cmdline,
                                   std::function<void(const std::string& line)> line_callback,
                                   std::function<void(const std::string& reason)> finished_callback,
                                   bool& finish_flag)
{
    std::shared_ptr<std::thread> t = std::make_shared<std::thread>([cmdline, line_callback, finished_callback, &finish_flag]()
    {
        ThreadHelper::setName("OsProcess::asyncExecCommand");
        std::string cp = cmdline;
        FILE* pipe = popen(cp.c_str(), "r");
        if (!pipe)
        {
            if (finished_callback)
                finished_callback("Failed to open pipe");
            return;
        }

        char buffer[1024];
        std::string lines;
        std::string result = "";
        int fno = fileno(pipe);

        // Make it non blocking
        fcntl(fno, F_SETFL, O_NONBLOCK);

        while (!feof(pipe) && !finish_flag)
        {
            // Wait for more data
            struct pollfd pfd{ .fd = fno, .events = POLLIN };

            while (poll(&pfd, 1, 0) == 0 && !finish_flag)
                ;

            // Read data
            if (finish_flag)
                continue;

            int r;
            do
            {
                r = static_cast<int>(read(fno, buffer, sizeof(buffer) - 1));
                if (r > 0)
                {
                    buffer[r] = 0;
                    lines += std::string(buffer);
                }
            }
            while (r == sizeof(buffer) - 1);

            if (lines.find("\n") != std::string::npos && line_callback)
            {
                std::string::size_type p = 0;
                while (p < lines.size())
                {
                    std::string::size_type d = lines.find("\n", p);
                    if (d != std::string::npos)
                    {
                        if (line_callback)
                            line_callback(strx::trim(lines.substr(p, d-p)));
                        p = d + 1;
                    }
                }

                lines.erase(0, p);
            }
        }

        if (finish_flag)
        {
            // Send SIGINT to process
        }
        if (pipe)
            pclose(pipe);

        finish_flag = true;
        if (finished_callback)
            finished_callback(std::string());
    });

    return t;
}

#if defined(TARGET_OSX) || defined(TARGET_LINUX)

pid_t OsProcess::findPid(const std::string& cmdline)
{
    try
    {
        std::ostringstream oss;
        oss << "pgrep -f " << "\"" << cmdline << "\"";
        std::string output = execCommand(oss.str());
        return std::atoi(output.c_str());
    }
    catch(...)
    {
        return 0;
    }
}

void OsProcess::killByPid(pid_t pid)
{
    if (pid <= 0)
        return;
    execSystem("kill -9 " + std::to_string(pid) + " &");
}
#endif

#endif
