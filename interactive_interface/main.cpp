#include <signal.h>

#include <iostream>

#include "server.hpp"

int server_fd = -1;

void signal_handler(int signal) 
{
    std::cout << std::endl;
    if (server_fd != -1) 
    {
        close(server_fd);
        ns_interface::log::_log_ptr->fatal(INFO, "fd closed");
    }
    ns_interface::log::_log_ptr->fatal(INFO, std::format("signal {0:d} triggered", signal));
    ns_interface::need_exit = true;
}

void setting_conf()
{
    auto set = my_log::log_format_setting::getInstance();
    set.insert("default", "[{0:lg}]: level={0:lv} {0:%Y}-{0:%m}-{0:%d} {0:%H}:{0:%M}:{0:%S} pid={0:pi} tid={0:ti} {0:fi}: line={0:li} colunm={0:co} {0:ms}");
    set.flush();
}

void componentInit()
{
    // setting_conf();
    ns_interface::log::init();
}

int main()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    componentInit();
    ns_interface::interface_server server(6400);
    server.start(server_fd);
    return 0;
}