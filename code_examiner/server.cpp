#include "server.hpp"

namespace ns_examiner
{
    const std::unordered_set<std::string_view> examiner_server::_ip_any = { "0", "0.0", "0.0.0", "0.0.0.0" };

    examiner_server::examiner_server(short port, const std::string& ip)
        : _server_msg()
        , _server_msg_len(0)
        , _listen_fd(-1)
    {
        _server_msg = ns_socket::internetStructInit(AF_INET, port,
            _ip_any.find(ip) != _ip_any.end() ? ip.data() : INADDR_ANY);
        _server_msg_len = sizeof(_server_msg);
    }

    examiner_server::~examiner_server()
    {
        thread_pool::data_t* ptr = nullptr;
        for (unsigned int i = 0; i < MAX_THREAD_SIZE; ++i)
        {
            _pool._thr_queue.pop(ptr);
            ptr->_cond.notify_one();
        }
    }
    
    void examiner_server::start(int& recycle_fd)
    {
        thread_pool::data_t* ptr = nullptr;
        int opt = 1;
        _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_listen_fd == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("{0:s}: {1:s}", "socket failed", strerror(errno)));
            throw examiner_exception { strerror(errno) };
        }
        if (setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("{0:s}: {1:s}", "setsockopt failed", strerror(errno)));
            throw examiner_exception { strerror(errno) };
        }
        recycle_fd = _listen_fd;
        if (bind(_listen_fd, reinterpret_cast<sockaddr*>(&_server_msg), _server_msg_len) == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("{0:s}: {1:s}", "bind failed", strerror(errno)));
            throw examiner_exception { strerror(errno) };
        }
        int connection_fd = 0;
        if (listen(_listen_fd, MAX_THREAD_SIZE) == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("{0:s}: {1:s}", "listen failed", strerror(errno)));
            throw examiner_exception{ strerror(errno) };
        }
        log::_log_ptr->info(INFO, "server prepared");
        while (!need_exit)
        {
            connection_fd = accept(_listen_fd, nullptr, nullptr);
            if (need_exit)
            {
                break;
            }
            if (connection_fd == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("{0:s}: {1:s}", "accept failed", strerror(errno)));
                throw examiner_exception{ strerror(errno) };
            }
            log::_log_ptr->info(INFO, "new connection established");
            _pool._thr_queue.pop(ptr);
            ptr->_cur_fd = connection_fd;
            ptr->_cond.notify_one();
        }
    }
}