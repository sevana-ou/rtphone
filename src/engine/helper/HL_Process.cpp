#include "HL_Process.h"
#include <thread>
#include <memory>

#ifdef TARGET_WIN
# define popen _popen
# define pclose _pclose
#endif

#if defined(TARGET_WIN)
#include <Windows.h>
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
    strcpy(cmdline, StringHelper::replace(cmd, "/", "\\").c_str());

    BOOL fSuccess = CreateProcessA( NULL, cmdline, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
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

#include <poll.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>
#include "helper/HL_String.h"

std::shared_ptr<std::thread> OsProcess::asyncExecCommand(const std::string& cmdline,
                                   std::function<void(const std::string& line)> callback,
                                   bool& finish_flag)
{
    std::shared_ptr<std::thread> t = std::make_shared<std::thread>([cmdline, callback, &finish_flag]()
    {
        std::string cp = cmdline;
        FILE* pipe = popen(cp.c_str(), "r");
        if (!pipe)
            throw std::runtime_error("Failed to run.");

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
                r = read(fno, buffer, sizeof(buffer)-1);
                if (r > 0)
                {
                    buffer[r] = 0;
                    lines += std::string(buffer);
                }
            }
            while (r == sizeof(buffer) - 1);

            if (lines.find("\n") != std::string::npos && callback)
            {
                std::string::size_type p = 0;
                while (p < lines.size())
                {
                    std::string::size_type d = lines.find("\n", p);
                    if (d != std::string::npos)
                    {
                        callback(StringHelper::trim(lines.substr(p, d-p)));
                        p = d + 1;
                    }
                }

                lines.erase(0, p);
            }
        }

        if (pipe)
            pclose(pipe);

        finish_flag = true;
    });

    return t;
}

#endif
