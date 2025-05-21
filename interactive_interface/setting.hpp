#pragma once
#include <chrono>
#include <sys/stat.h>

namespace ns_interface
{
    constexpr timeval NET_TIME_OUT = {.tv_sec = 2, .tv_usec = 0};
    constexpr std::chrono::duration START_DURATION = std::chrono::milliseconds{ 1 };
    constexpr unsigned long long NET_BUFFER_SIZE = 1024 * 1024 * 10;
    constexpr unsigned int MAX_THREAD_SIZE = 2;
}