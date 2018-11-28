// Copyright (c) 2018, ZIH, Technische Universitaet Dresden, Federal Republic of Germany
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of metricq nor the names of its contributors
//       may be used to endorse or promote products derived from this software
//       without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "run_command.hpp"

#include <metricq/logger/nitro.hpp>

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

using Log = metricq::logger::nitro::Log;

[[noreturn]] static void spawn(const std::vector<std::string>& cmdline)
{
    auto len = cmdline.size();
    auto args = std::make_unique<const char*[]>(len + 1);
    {
        const char** argp = args.get();
        for (auto& arg : cmdline)
        {
            *argp++ = arg.c_str();
        }
        *argp = nullptr;
    }

    const char* progname = args[0];
    ::execvp(progname, const_cast<char* const*>(args.get()));

    int errsv = errno;
    Log::error() << "Failed to execute " << progname << ": " << std::strerror(errsv);

    std::exit(EXIT_FAILURE);
}

int run_command(const std::vector<std::string>& cmdline)
{
    assert(cmdline.size() > 0);

    const std::string& progname = cmdline[0];

    int child_pid = ::fork();

    switch (child_pid)
    {
    case -1:
    {
        int errsv = errno;
        Log::error() << "Failed to execute " << progname << ": " << std::strerror(errsv);
        std::exit(EXIT_FAILURE);
    }
    case 0:
    {
        spawn(cmdline);
    }
    default:
    {
        Log::info() << "Waiting for " << progname << " (pid: " << child_pid << ") to exit...";
        int stat_val;
        while (true)
        {
            int wpid = ::waitpid(child_pid, &stat_val, WUNTRACED);

            if (wpid == -1)
            {
                int errsv = errno;
                Log::error() << "Error waiting for " << progname << " (pid: " << child_pid
                             << "): " << std::strerror(errsv);
                std::exit(EXIT_FAILURE);
            }

            if (WIFEXITED(stat_val))
            {
                int status = WEXITSTATUS(stat_val);
                Log::info() << progname << " exited (status " << status << ")";
                return status;
            }
            else if (WIFSIGNALED(stat_val))
            {
                int sig = WTERMSIG(stat_val);
                Log::info() << progname << " was killed (signal " << sig << ")";
                return 128 + sig;
            }
        }
    }
    }
}

