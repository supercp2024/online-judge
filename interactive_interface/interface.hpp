#pragma once
#include <coroutine>
#include <concepts>
#include <utility>
#include <list>

#include <sys/epoll.h>

#include "examiner_control.hpp"
#include "globle_variable.hpp"
#include "my_utility.hpp"
#include "database.hpp"
#include "protocol.hpp"
#include "socket.hpp"

namespace ns_interface
{
    struct worker
    {
        struct coroutine_info
        {
            enum class status_info : unsigned int
            {
                FAILURE = 0,
                UNFINISH,
                SUCCESS,
                EXCEPTION
            };

            enum class func_info : unsigned int
            {
                TASK = 0,
                READ,
                WRITE,
                OPEN,
                CLOSE
            };

            struct status
            {
                status(status_info ret = status_info::SUCCESS, func_info which = func_info::TASK);

                status_info _ret;
                func_info _which;
            };

            struct promise_type
            {
                promise_type();
                coroutine_info get_return_object();
                void unhandled_exception();  
                std::suspend_never initial_suspend();
                std::suspend_always final_suspend() noexcept;
                void return_value(status ret_val);
                std::suspend_always yield_value(status ret_val);

                std::exception_ptr _e_ptr;
                status _status;
            };

            using cor_handle = std::coroutine_handle<promise_type>;

            coroutine_info(const cor_handle& h = nullptr);
            ~coroutine_info();
            coroutine_info(const coroutine_info& other) = delete;
            coroutine_info(coroutine_info&& other);
            coroutine_info& operator=(const coroutine_info& other) = delete;
            coroutine_info& operator=(coroutine_info&& other);
            void rethrow();
            const char* error();

            cor_handle _h;
            constexpr static std::array<std::string_view, 4> _status_to_string = { "failure", "unfinish", "success", "exception" };
            constexpr static std::array<std::string_view, 5> _func_to_string = { "task", "read", "write", "open", "close" };
        };

        worker(int fd = -1);
        worker(const worker& other) = delete;
        worker& operator=(const worker& other) = delete;
        worker(worker&& other);
        worker& operator=(worker&& other);
        // 收发报文
        coroutine_info parse();
        // 处理网页请求
        coroutine_info bodyProcessing();
        // 拉取404html
        void notFound();
        // 拉取网页骨架资源
        void normalResourse(const std::filesystem::path& target, const std::string& extension);
        // 拉取题库资源
        void totalResourse();
        // 拉取具体题目资源
        void singleResourse(const std::string& name);

        template<class HTTP_TYPE>
        requires std::is_base_of_v<http_base, HTTP_TYPE>
        coroutine_info recv(int fd, HTTP_TYPE& input);

        template<class HTTP_TYPE>
        requires std::is_base_of_v<http_base, HTTP_TYPE>
        coroutine_info send(int fd, const HTTP_TYPE& output);

        http_request _input;
        http_response _output;
        coroutine_info _io;
        coroutine_info _self;
        std::string _buf;
        int _fd;
    };

    class scheduler
    {
    private:
        struct thread_variable
        {
            thread_variable();
            ~thread_variable();
            std::thread _th;
            std::mutex _m;
            std::condition_variable _cond;
            spin_lock _sp;
            int _epoll_fd;
            int _trigger_fd;
        };
    public:
        scheduler();
        ~scheduler();
        scheduler(const scheduler& other) = delete;
        scheduler(scheduler&& other) = delete;
        const scheduler& operator=(const scheduler& other) = delete;
        const scheduler& operator=(scheduler&& other) = delete;
        // 负载均衡式任务分配函数
        void dispatch(int connection_fd);
        // 任务线程入口
        void thread_entry(size_t group);
        // 唤醒在epoll上阻塞的线程退出
        void setTrigger(size_t group, int fd);
    private:
        // 线程相关变量存放处
        std::vector<thread_variable> _execute_stream_arr;
        // 每个线程正在处理的任务数量
        std::vector<std::list<worker>> _process_queue_arr;
        // 每个线程等待处理的任务数量，使用该结构以尽可能的减少阻塞临界区的代价
        std::vector<std::list<worker>> _waiting_queue_arr;
        // 每个线程正在处理的任务数量
        std::vector<std::atomic<size_t>> _total_node_arr;
        // 空闲节点池
        // 这里少一个垃圾回收器，不过只有高负载才会有影响
        std::list<worker> _spare_node_queue;
        spin_lock _sp;
    };
}