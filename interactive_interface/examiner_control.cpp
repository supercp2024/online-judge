#include "examiner_control.hpp"

namespace ns_interface
{
    examiner_server_controller::examiner_server_info::examiner_server_info(short port, const std::string& ip) noexcept
        : _port(port)
        , _ip(ip)
        , _server_msg(ns_socket::internetStructInit(AF_INET, port, ip.data()))
        , _server_msg_len(sizeof(_server_msg))
        , _load(0)
    {}

    examiner_server_controller::examiner_server_info::examiner_server_info(examiner_server_info&& other)
        : _port(other._port)
        , _ip(other._ip)
        , _server_msg(other._server_msg)
        , _server_msg_len(other._server_msg_len)
        , _load(other._load.load(std::memory_order_relaxed))
    {}

    int examiner_server_controller::examiner_server_info::connect()
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        {
            throw interface_exception{ strerror(errno) };
        }
        if (::connect(fd, reinterpret_cast<const sockaddr*>(&_server_msg), _server_msg_len) == -1)
        {
            utility::sys_close(fd);
            return -1;
        }
        ++_load;
        return fd;
    }

    void examiner_server_controller::examiner_server_info::disconnect(int fd)
    {
        utility::sys_close(fd);
        --_load;
    }

    unsigned int examiner_server_controller::examiner_server_info::getValue() const noexcept
    {
        return _load.load(std::memory_order_relaxed);
    }

    examiner_server_controller::examiner_server_controller()
        : _server_arr()
        , _ctrl()
    {
        loadSetting();
    }

    examiner_server_controller::~examiner_server_controller()
    {
        delete _instance_ptr;
    }

    void examiner_server_controller::loadSetting()
    {
        std::ifstream fin("./examiner_server_setting.conf", std::ios::binary | std::ios::in);
        short port = 0;
        std::string ip;
        while (fin >> ip >> port)
        {
            _server_arr.push_back(std::make_shared<examiner_server_info>(port, ip));
        }
    }

    std::pair<int, size_t> examiner_server_controller::connect()
    {
        if (_server_arr.size() == 0)
        {
            return { -1, -1 };
        }
        unsigned int min_count = -1;
        unsigned int index = 0;
        int fd = -1;
        std::shared_lock lock(_ctrl);
        for (unsigned int i = 0; i < _server_arr.size(); ++i)
        {
            if (_server_arr[i]->getValue() < min_count)
            {
                min_count = _server_arr[i]->getValue();
                index = i;
            }
        }
        fd = _server_arr[index]->connect();
        if (fd == -1)
        {
            _server_arr.erase(_server_arr.begin() + index);
            return { -1, -1 };
        }
        return { fd, index };
    }

    void examiner_server_controller::disconnect(std::pair<int, size_t> fd_and_index)
    {
        _server_arr[fd_and_index.second]->disconnect(fd_and_index.first);
    }

    bool examiner_server_controller::empty() const noexcept
    {
        return !_server_arr.size();
    }

    examiner_server_controller* examiner_server_controller::_instance_ptr = new examiner_server_controller{};
}