#pragma once
#include <condition_variable>
#include <unordered_map>
#include <string_view>
#include <functional>
#include <utility>
#include <string>
#include <thread>
#include <memory>
#include <mutex>

#include <boost/lockfree/queue.hpp>

#include "globle_variable.hpp"
#include "code_file.hpp"
#include "protocol.hpp"
#include "socket.hpp"
#include "task.hpp"

namespace ns_examiner
{
    class thread_pool
    {
        // 工作线程变量
        struct worker
        {
            worker(int fd = -1);
            ~worker() = default;
            worker(worker&& other) noexcept;
            worker& operator=(worker&& other) noexcept;

            std::thread _thr;
            std::mutex _ctrl;
            std::condition_variable _cond;
            std::string _net_buf;
            size_t _index;
            int _cur_fd;
            sockaddr_in _client_msg;
            socklen_t _client_msg_len;
        };

        using queue_t = boost::lockfree::queue<worker*, boost::lockfree::capacity<MAX_THREAD_SIZE>>;
    public:
        using data_t = worker;
    public:
        thread_pool();
        void thread_entry(worker* self_ptr);
    private:
        static void bodyProcessing(http_request& input, http_response& output, worker* self_ptr);
        static bool recv(http_request& input, worker* self_ptr);
        static bool send(http_response&& output, worker* self_ptr);
    public:
        queue_t _thr_queue;
    };
}