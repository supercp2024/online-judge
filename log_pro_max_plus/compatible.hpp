#pragma once
#ifdef _WIN32
#include <windows.h>
#elif __linux
#include <unistd.h>
#include <sys/types.h>
#endif

namespace my_log
{
    #ifdef _WIN32
    using pid_t = DWORD;
    #elif __linux
    using pid_t = ::pid_t;
    #endif

    #ifdef _WIN32
    inline pid_t getpid()
    {
        return GetCurrentProcessId();
    }

    inline pid_t gettid()
    {
        return GetCurrentThreadId();
    }
    #elif __linux
    inline pid_t getpid()
    {
        return ::getpid();
    }

    inline pid_t gettid()
    {
        return ::gettid();
    }
    #endif
}