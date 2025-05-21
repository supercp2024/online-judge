#pragma once
#include <chrono>

namespace my_log
{
	constexpr auto MAX_BUFFER_SIZE = 1024 * 1024 * 10;
	constexpr auto DEFAULT_STARTUP_DURATION = std::chrono::milliseconds{ 1000 };
    constexpr auto MAX_PATTERN_LEN = 512;
    constexpr auto MAX_FORMATTED_STR_LEN = 512;
}