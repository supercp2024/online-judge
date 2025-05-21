#pragma once
#include "../log_pro_max_plus/logger.hpp"
#define INFO std::source_location::current()

namespace ns_interface
{
    struct log
    {
    private:
        log();
        ~log();
    public:
        static void init();
    public:
        static my_log::sync_logger* _log_ptr;
    };
}