#pragma once
#include <shared_mutex>
#include <memory>
#include <queue>

#include "my_utility.hpp"
#include "socket.hpp"

namespace ns_interface
{
    // 编译服务器单例控制器
    class examiner_server_controller
    {
    private:
        // 编译服务器信息
        class examiner_server_info
        {
        public:
            examiner_server_info(short port, const std::string& ip = "0.0.0.0") noexcept;
            examiner_server_info(examiner_server_info&& other);
            int connect();
            void disconnect(int fd);
            unsigned int getValue() const noexcept;
        private:
            short _port;
            std::string _ip;
            sockaddr_in _server_msg;
            socklen_t _server_msg_len;
            std::atomic<unsigned int> _load;
        };
    private:
        using info_ptr = std::shared_ptr<examiner_server_info>;
    private:
        examiner_server_controller();
        ~examiner_server_controller();
        void loadSetting();
    public:
        // 申请编译服务使用权
        std::pair<int, size_t> connect();
        // 归还编译服务使用权
        void disconnect(std::pair<int, size_t> fd_and_index);
        bool empty() const noexcept;
    private:
        std::vector<info_ptr> _server_arr;
        std::shared_mutex _ctrl;
    public:
        static examiner_server_controller* _instance_ptr;
    };
}