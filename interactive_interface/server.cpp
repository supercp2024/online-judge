#include "server.hpp"

namespace ns_interface
{
    const std::unordered_set<std::string_view> interface_server::_ip_any = { "0", "0.0", "0.0.0", "0.0.0.0" };

    interface_server::interface_server(short port, const std::string& ip)
        : _sched()
        , _server_msg()
        , _server_msg_len(0)
    {
        _server_msg = ns_socket::internetStructInit(AF_INET, port,
            _ip_any.find(ip) != _ip_any.end() ? ip.data() : INADDR_ANY);
        _server_msg_len = sizeof(_server_msg);
        int pipes[2];
        for (unsigned int i = 0; i < MAX_THREAD_SIZE; ++i)
        {
            if (pipe(pipes) == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("connot create pipe: {0:s}", strerror(errno)));
                throw interface_exception { strerror(errno) };
            }
            triggers[i] = pipes[1];
            _sched.setTrigger(i, pipes[0]);
        }
    }

    interface_server::~interface_server()
    {
        char ch = '$';
        for (unsigned int i = 0; i < MAX_THREAD_SIZE; ++i)
        {
            utility::sys_write(triggers[i], &ch, 1);
            utility::sys_close(triggers[i]);
        }
    }

    void interface_server::start(int& recycle_fd)
    {
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        if (listen_fd == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("socket failed: {0:s}", strerror(errno)));
            throw interface_exception { strerror(errno) };
        }
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("setsockopt failed: {0:s}", strerror(errno)));
            throw interface_exception { strerror(errno) };
        }
        recycle_fd = listen_fd;
        if (bind(listen_fd, reinterpret_cast<sockaddr*>(&_server_msg), _server_msg_len) == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("bind failed: {0:s}", strerror(errno)));
            throw interface_exception { strerror(errno) };
        }
        int connection_fd = 0;
        int flag = 0;
        if (listen(listen_fd, MAX_THREAD_SIZE) == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("listen failed: {0:s}", strerror(errno)));
            throw interface_exception{ strerror(errno) };
        }
        log::_log_ptr->info(INFO, "server prepared");
        while (!need_exit)
        {
            connection_fd = accept(listen_fd, nullptr, nullptr);
            if (need_exit)
            {
                break;
            }
            if (connection_fd == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("accept failed: {0:s}", strerror(errno)));
                throw interface_exception{ strerror(errno) };
            }
            log::_log_ptr->info(INFO, "new connection established");
            flag = fcntl(connection_fd, F_GETFL, 0);
            if (flag == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("fcntl failed: {0:s}", strerror(errno)));
                throw interface_exception{ strerror(errno) };
            }
            flag |= O_NONBLOCK;
            flag = fcntl(connection_fd, F_SETFL, flag);
            if (flag == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("fcntl failed: {0:s}", strerror(errno)));
                throw interface_exception{ strerror(errno) };
            }
            _sched.dispatch(connection_fd);
        }
    }
}