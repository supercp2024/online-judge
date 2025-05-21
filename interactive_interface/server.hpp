#include <unordered_set>

#include "interface.hpp"

namespace ns_interface
{
    class interface_server
    {
        using trigger = std::array<int, ns_interface::MAX_THREAD_SIZE>;
    public:
        interface_server(short port, const std::string& ip = "0.0.0.0");
        ~interface_server();
        void start(int& recycle_fd);
    private:
        scheduler _sched;
        sockaddr_in _server_msg;
        socklen_t _server_msg_len;
        std::array<int, MAX_THREAD_SIZE> triggers;
    private:
        static const std::unordered_set<std::string_view> _ip_any;
    };
}