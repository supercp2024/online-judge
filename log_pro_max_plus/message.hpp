#pragma once
#include <chrono>
#include <vector>
#include <string>
#include <thread>
#include <utility>
#include <source_location>
#include "level.hpp"
#include "compatible.hpp"

namespace my_log
{
    struct log_message
    {
        std::chrono::time_point<std::chrono::system_clock> _cur_time;
        std::string _logger;
        pid_t _pid;
        pid_t _tid;
        std::string _file;
        size_t _line;
        size_t _column;
        my_log::log_level::value _level;
        std::string _payload;

        log_message(const std::string& logger, const std::string& file,
            size_t line, size_t column, my_log::log_level::value level,
            const std::string& main_msg)
            : _cur_time(std::chrono::system_clock::now())
            , _logger(logger)
            , _pid(getpid())
            , _tid(gettid())
            , _file(file)
            , _line(line)
            , _column(column)
            , _level(level)
            , _payload(main_msg)
        {}
    };
}