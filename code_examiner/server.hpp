#pragma once
#include <unordered_set>

#include "worker.hpp"

namespace ns_examiner
{
    // 服务器配置+校验请求分发
    class examiner_server
    {
    public:
        examiner_server(short port, const std::string& ip = "0.0.0.0");
        ~examiner_server();
        void start(int& recycle_fd);
    private:
        sockaddr_in _server_msg;
        socklen_t _server_msg_len;
        int _listen_fd;
        thread_pool _pool;
    private:
        static const std::unordered_set<std::string_view> _ip_any;
    };
}