#include "log.hpp"

namespace ns_interface
{
    my_log::sync_logger* log::_log_ptr = nullptr;

    log::log()
    {
        _log_ptr = new my_log::sync_logger("test", my_log::log_level::value::DEBUG);
        std::vector<my_log::output_t> tem;
        tem.emplace_back(my_log::output_t{ my_log::screen_output{} });
        _log_ptr->addFmtStrAndOutput("default", std::move(tem));
        _log_ptr->start();
        _log_ptr->debug(INFO, "logger start");
    }

    log::~log()
    {}

    void log::init()
    {
        static log instance;
    }
}