#include "interface.hpp"

namespace ns_interface
{
    worker::coroutine_info::status::status(status_info ret, func_info which)
        : _ret(ret)
        , _which(which)
    {}

    worker::coroutine_info::promise_type::promise_type()
        : _e_ptr(nullptr)
        , _status()
    {}

    worker::coroutine_info worker::coroutine_info::promise_type::get_return_object()
    {
        return cor_handle::from_promise(*this);
    }

    void worker::coroutine_info::promise_type::unhandled_exception() 
    { 
        _e_ptr = std::current_exception(); 
        _status._ret = status_info::EXCEPTION;
    }

    std::suspend_never worker::coroutine_info::promise_type::initial_suspend() 
    {
        return {}; 
    }

    std::suspend_always worker::coroutine_info::promise_type::final_suspend() noexcept 
    { 
        return {};
    }

    void worker::coroutine_info::promise_type::return_value(status ret_val)
    {
        _status = ret_val;
    }

    std::suspend_always worker::coroutine_info::promise_type::yield_value(status ret_val)
    {
        _status = ret_val;
        return {};
    }

    worker::coroutine_info::coroutine_info(const cor_handle& h)
        : _h(h)
    {}

    worker::coroutine_info::~coroutine_info()
    {
        if (_h && _h.done())
        {
            _h.destroy();
            _h = nullptr;
        }
    }

    worker::coroutine_info::coroutine_info(coroutine_info&& other)
        : _h(other._h)
    {
        other._h = nullptr;
    }

    worker::coroutine_info& worker::coroutine_info::operator=(coroutine_info&& other)
    {
        if (this != &other) 
        {
            if (_h && _h.done())
            {
                _h.destroy();
            }
            _h = other._h;
            other._h = nullptr;
        }
        return *this;
    }

    void worker::coroutine_info::rethrow()
    {  
        std::rethrow_exception(_h.promise()._e_ptr);
    }

    const char* worker::coroutine_info::error() 
    {  
        try
        {
            rethrow();
        }
        catch (const std::exception& e)
        {
            return e.what();
        }
        return "";
    }

    worker::worker(int fd)
        : _input()
        , _output()
        , _io()
        , _self()
        , _buf(NET_BUFFER_SIZE, '\0')
        , _fd(fd)
    {}

    worker::worker(worker&& other)
        : _input(std::move(other._input))
        , _output(std::move(other._output))
        , _io(std::move(other._io))
        , _self(std::move(other._self))
        , _buf(std::move(other._buf))
        , _fd(other._fd)
    {}

    worker& worker::operator=(worker&& other)
    {
        if (this != &other)
        {
            _input = std::move(other._input);
            _output = std::move(other._output);        
            _io = std::move(other._io);
            _self = std::move(other._self);
            _buf = std::move(other._buf);
            _fd = other._fd;
        }
        return *this;
    }

    // 接收 -> 处理 -> 发送
    worker::coroutine_info worker::parse()
    {
        coroutine_info body_ret;
        coroutine_info::status status;
        bool need_jump = false;
        // 循环只有在某个步骤失败或异常才会退出
        while (1)
        {
            _input.clear();
            _output.clear();
            log::_log_ptr->debug(INFO, std::format("{0:d}: recv start", _fd));
            _io = recv(_fd, _input);
            need_jump = false;
            while (!need_jump)
            {
                switch (_io._h.promise()._status._ret)
                {
                case coroutine_info::status_info::EXCEPTION:
                    log::_log_ptr->error(INFO, std::format("{0:d}: recv error, {1:s}", _fd, _io.error()));
                    _io._h.destroy();
                    _io._h = nullptr;
                    status._ret = coroutine_info::status_info::SUCCESS;
                    status._which = coroutine_info::func_info::TASK;
                    co_return status;
                    break;
                case coroutine_info::status_info::FAILURE:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: recv fail", _fd));
                    _io._h.destroy();
                    _io._h = nullptr;
                    status._ret = coroutine_info::status_info::SUCCESS;
                    status._which = coroutine_info::func_info::TASK;
                    co_return status;
                    break;
                case coroutine_info::status_info::UNFINISH:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: recv suspend", _fd));
                    status._ret = coroutine_info::status_info::UNFINISH;
                    status._which = coroutine_info::func_info::TASK;
                    co_yield status;
                    log::_log_ptr->debug(INFO, std::format("{0:d}: recv resume", _fd));
                    _io._h.resume();
                    break;
                case coroutine_info::status_info::SUCCESS:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: recv success", _fd));
                    _io._h.destroy();
                    _io._h = nullptr;
                    need_jump = true;
                    break;
                }
            }
            log::_log_ptr->debug(INFO, std::format("{0:d}: body start", _fd));
            body_ret = bodyProcessing();
            need_jump = false;
            while (!need_jump)
            {
                switch (body_ret._h.promise()._status._ret)
                {
                case coroutine_info::status_info::EXCEPTION:
                    log::_log_ptr->error(INFO, std::format("{0:d}: body error, {1:s}", _fd, body_ret.error()));
                    body_ret._h.destroy();
                    body_ret._h = nullptr;
                    status._ret = coroutine_info::status_info::SUCCESS;
                    status._which = coroutine_info::func_info::TASK;
                    co_return status;
                    break;
                case coroutine_info::status_info::FAILURE:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: body fail", _fd));
                    body_ret._h.destroy();
                    body_ret._h = nullptr;
                    status._ret = coroutine_info::status_info::FAILURE;
                    status._which = coroutine_info::func_info::TASK;
                    co_return status;
                    break;
                case coroutine_info::status_info::UNFINISH:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: body suspend", _fd));
                    status._ret = coroutine_info::status_info::UNFINISH;
                    status._which = coroutine_info::func_info::TASK;
                    co_yield status;
                    log::_log_ptr->debug(INFO, std::format("{0:d}: body resume", _fd));
                    body_ret._h.resume();
                    break;
                case coroutine_info::status_info::SUCCESS:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: body success", _fd));
                    body_ret._h.destroy();
                    body_ret._h = nullptr;
                    need_jump = true;
                    break;
                }
            }
            log::_log_ptr->debug(INFO, std::format("{0:d}: send start", _fd));
            _io = send(_fd, _output);
            need_jump = false;
            while (!need_jump)
            {
                switch (_io._h.promise()._status._ret)
                {
                case coroutine_info::status_info::EXCEPTION:
                    log::_log_ptr->error(INFO, std::format("{0:d}: send error, {1:s}", _fd, _io.error()));
                    _io._h.destroy();
                    _io._h = nullptr;
                    status._ret = coroutine_info::status_info::SUCCESS;
                    status._which = coroutine_info::func_info::TASK;
                    co_return status;
                    break;
                case coroutine_info::status_info::FAILURE:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: send fail", _fd));
                    _io._h.destroy();
                    _io._h = nullptr;
                    status._ret = coroutine_info::status_info::FAILURE;
                    status._which = coroutine_info::func_info::TASK;
                    co_return status;
                    break;
                case coroutine_info::status_info::UNFINISH:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: send suspend", _fd));
                    status._ret = coroutine_info::status_info::UNFINISH;
                    status._which = coroutine_info::func_info::TASK;
                    co_yield status;
                    log::_log_ptr->debug(INFO, std::format("{0:d}: send resume", _fd));
                    _io._h.resume();
                    break;
                case coroutine_info::status_info::SUCCESS:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: send success", _fd));
                    _io._h.destroy();
                    _io._h = nullptr;
                    need_jump = true;
                    break;
                }
            }
        }
    }

    worker::coroutine_info worker::bodyProcessing()
    {
        /********************************
         * 1. 拉取题库（主页）GET
         * 2. 跳转至某一道题（题目页）GET
         * 3. 提交题目 POST
         ********************************/
        const static std::filesystem::path root = "./wwwroot";
        coroutine_info ret;
        coroutine_info::status status;
        if (_input._method == "POST")
        {
            examiner_server_controller& es_ctrl = *examiner_server_controller::_instance_ptr;
            if (es_ctrl.empty())
            {
                status._ret = coroutine_info::status_info::FAILURE;
                status._which = coroutine_info::func_info::TASK;
                co_return status;
            }
            sql_client sql;
            Json::Reader reader;
            Json::StreamWriterBuilder writerBuilder;
            Json::Value input_json;
            Json::Value output_json;
            Json::Value examiner_input_json;
            Json::Value examiner_output_json;
            bool need_jump = false;
            http_request examiner_input;
            http_response examiner_output;
            writerBuilder.settings_["emitUTF8"] = true;
            writerBuilder.settings_["indentation"] = "";
            std::pair<int, size_t> server_msg;
            int fd = -1;
            do
            {
                server_msg = es_ctrl.connect();
                fd = server_msg.first;
            }
            while (fd == -1 && !es_ctrl.empty());
            if (es_ctrl.empty())
            {
                status._ret = coroutine_info::status_info::FAILURE;
                status._which = coroutine_info::func_info::TASK;
                co_return status;
            }
            reader.parse(_input._content, input_json);
            std::string user_code = input_json["code"].asString();
            examiner_input_json["name"] = input_json["name"];
            sql.search(examiner_input_json, std::move(user_code));
            examiner_input._url = '/';
            examiner_input._method = "post";
            examiner_input._version = "HTTP/1.0";
            examiner_input._content = Json::writeString(writerBuilder, examiner_input_json);
            examiner_input.headerStorge("Content-Length", true, std::to_string(examiner_input._content.size()), true);
            _io = send(fd, examiner_input);
            status = _io._h.promise()._status;
            need_jump = false;
            while (!need_jump)
            {
                switch (_io._h.promise()._status._ret)
                {
                case coroutine_info::status_info::EXCEPTION:
                    _io._h.destroy();
                    _io._h = nullptr;
                    status._ret = coroutine_info::status_info::FAILURE;
                    status._which = coroutine_info::func_info::TASK;
                    co_return status;
                    break;
                case coroutine_info::status_info::FAILURE:
                    _io._h.destroy();
                    _io._h = nullptr;
                    status._ret = coroutine_info::status_info::FAILURE;
                    status._which = coroutine_info::func_info::TASK;
                    co_return status;
                    break;
                case coroutine_info::status_info::UNFINISH:
                    status._ret = coroutine_info::status_info::UNFINISH;
                    status._which = coroutine_info::func_info::TASK;
                    co_yield status;
                    _io._h.resume();
                    break;
                case coroutine_info::status_info::SUCCESS:
                    _io._h.destroy();
                    _io._h = nullptr;
                    need_jump = true;
                    break;
                }
            }
            _io = recv(fd, examiner_output);
            status = _io._h.promise()._status;
            need_jump = false;
            while (!need_jump)
            {
                switch (_io._h.promise()._status._ret)
                {
                case coroutine_info::status_info::EXCEPTION:
                    _io._h.destroy();
                    _io._h = nullptr;
                    status._ret = coroutine_info::status_info::FAILURE;
                    status._which = coroutine_info::func_info::TASK;
                    co_return status;
                    break;
                case coroutine_info::status_info::FAILURE:
                    _io._h.destroy();
                    _io._h = nullptr;
                    status._ret = coroutine_info::status_info::FAILURE;
                    status._which = coroutine_info::func_info::TASK;
                    co_return status;
                    break;
                case coroutine_info::status_info::UNFINISH:
                    status._ret = coroutine_info::status_info::UNFINISH;
                    status._which = coroutine_info::func_info::TASK;
                    co_yield status;
                    _io._h.resume();
                    break;
                case coroutine_info::status_info::SUCCESS:
                    _io._h.destroy();
                    _io._h = nullptr;
                    need_jump = true;
                    break;
                }
            }
            _output._content = std::move(examiner_output._content);
            _output.headerStorge("Content-Length", true, std::to_string(_output._content.size()), true);
            _output._header_table.emplace("Content-Type", _output._msg_type.at(".json"));
            _output._header_table.emplace("X-Content-Type-Options", "nosniff");
            _output._code = "200";
            _output._status = "OK";
            _output._version = "HTTP/1.1";
            es_ctrl.disconnect(server_msg);
        }
        else if (_input._method == "GET")
        {
            url input_url(_input._url);
            std::filesystem::path target = root;
            if (input_url._resourse_path == "/")
            {
                input_url._resourse_path = "/problems/problems.html";
                input_url._extension = ".html";
            }
            auto cur_path = input_url._resourse_path.begin();
            ++cur_path;
            if (*cur_path != "problems")
            {
                notFound();
                status._which = coroutine_info::func_info::TASK;
                co_return status;
            }
            ++cur_path;
            if (cur_path->has_extension())
            {
                if (cur_path != input_url._resourse_path.end())
                {
                    if (_output._msg_type.find(input_url._extension) == _output._msg_type.end())
                    {
                        notFound();
                    }
                    else if (input_url._extension == ".json")
                    {
                        totalResourse();
                    }
                    else
                    {
                        target /= *cur_path;
                        normalResourse(target, input_url._extension);
                    }
                }
                else
                {
                    notFound();
                }
                status._which = coroutine_info::func_info::TASK;
                co_return status;
            }
            target /= *cur_path;
            ++cur_path;
            if (cur_path->has_extension())
            {
                if (cur_path != input_url._resourse_path.end())
                {
                    if (_output._msg_type.find(input_url._extension) == _output._msg_type.end())
                    {
                        notFound();
                    }
                    else if (input_url._extension == ".json")
                    {
                        singleResourse(input_url._parameters.at("name"));
                    }
                    else
                    {
                        target /= *cur_path;
                        normalResourse(target, input_url._extension);
                    }
                }
                else
                {
                    notFound();
                }
                status._which = coroutine_info::func_info::TASK;
                co_return status;
            }
        }
        status._which = coroutine_info::func_info::TASK;
        co_return status;
    }

    void worker::notFound()
    {
        std::ifstream fin;
        _output._code = "404";
        _output._version = "HTTP/1.1";
        _output._status = "Not Found";
        uintmax_t size = std::filesystem::file_size("./wwwroot/404.html");
        _output.headerStorge("Content-Length", true, std::to_string(size), true);
        _output._header_table.emplace("Content-Type", _output._msg_type.at(".html"));
        _output._header_table.emplace("Connection", "keep-alive");
        _output._header_table.emplace("X-Content-Type-Options", "nosniff");
        fin.open("./wwwroot/404.html", std::ios::binary);
        _output._content.resize(size);
        fin.read(_output._content.data(), size);
        fin.close();
    }

    void worker::normalResourse(const std::filesystem::path& target, const std::string& extension)
    {
        std::ifstream fin;
        _output._code = "200";
        _output._version = "HTTP/1.1";
        _output._status = "OK";
        uintmax_t size = std::filesystem::file_size(target);
        _output.headerStorge("Content-Length", true, std::to_string(size), true);
        _output._header_table.emplace("Content-Type", _output._msg_type.at(extension));
        _output._header_table.emplace("Connection", "keep-alive");
        _output._header_table.emplace("X-Content-Type-Options", "nosniff");
        fin.open(target, std::ios::binary);
        _output._content.resize(size);
        fin.read(_output._content.data(), size);
        fin.close();
    }

    void worker::totalResourse()
    {
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder.settings_["emitUTF8"] = true;
        writerBuilder.settings_["indentation"] = "";
        Json::Value output_json;
        sql_client sql;
        _output._code = "200";
        _output._version = "HTTP/1.1";
        _output._status = "OK";
        _output._header_table.emplace("Content-Type", _output._msg_type.at(".json"));
        _output._header_table.emplace("Connection", "keep-alive");
        _output._header_table.emplace("X-Content-Type-Options", "nosniff");
        sql.searchAll(output_json);
        _output._content = Json::writeString(writerBuilder, output_json);
        _output.headerStorge("Content-Length", true, std::to_string(_output._content.size()), true);
    }

    void worker::singleResourse(const std::string& name)
    {
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder.settings_["emitUTF8"] = true;
        writerBuilder.settings_["indentation"] = "";
        Json::Value output_json;
        sql_client sql;
        if (!sql.searchInfo(output_json, name))
        {
            notFound();
            return;
        };
        _output._code = "200";
        _output._version = "HTTP/1.1";
        _output._status = "OK";
        _output._header_table.emplace("Content-Type", _output._msg_type.at(".json"));
        _output._header_table.emplace("Connection", "keep-alive");
        _output._header_table.emplace("X-Content-Type-Options", "nosniff");
        _output._content = Json::writeString(writerBuilder, output_json);
        _output.headerStorge("Content-Length", true, std::to_string(_output._content.size()), true);
    }

    template<class HTTP_TYPE>
    requires std::is_base_of_v<http_base, HTTP_TYPE>
    worker::coroutine_info worker::recv(int fd, HTTP_TYPE& input)
    {
        coroutine_info::status status;
        status._which = coroutine_info::func_info::READ;
        ssize_t read_bytes = 0;
        do
        {
            read_bytes = utility::sys_read(fd, _buf.data(), NET_BUFFER_SIZE);
            if (read_bytes == 0)
            {
                status._ret = coroutine_info::status_info::FAILURE;
                co_return status;
            }
            else if (read_bytes == -1)
            {
                status._ret = coroutine_info::status_info::UNFINISH;
                co_yield status;
                read_bytes = 0;
                continue;
            }
        }
        while (!input.deserialize(_buf.begin(), _buf.begin() + read_bytes));
        status._ret = coroutine_info::status_info::SUCCESS;
        log::_log_ptr->info(INFO, "message received");
        co_return status;
    }

    template worker::coroutine_info worker::recv(int fd, http_request& input);
    template worker::coroutine_info worker::recv(int fd, http_response& input);

    template<class HTTP_TYPE>
    requires std::is_base_of_v<http_base, HTTP_TYPE>
    worker::coroutine_info worker::send(int fd, const HTTP_TYPE& output)
    {
        coroutine_info::status status;
        auto str_arr = output.serialize();
        ssize_t written_bytes = 0;
        ssize_t cur_bytes = 0;
        status._which = coroutine_info::func_info::WRITE;
        for (const auto& str : str_arr)
        {
            do
            {
                cur_bytes = utility::sys_write(fd, str.data() + written_bytes, str.size() - written_bytes);
                if (cur_bytes == 0)
                {
                    status._ret = coroutine_info::status_info::FAILURE;
                    co_return status;
                }
                else if (cur_bytes == -1)
                {
                    if (errno == EPIPE)
                    {
                        status._ret = coroutine_info::status_info::FAILURE;
                        co_return status;
                    }
                    else
                    {
                        status._ret = coroutine_info::status_info::UNFINISH;
                        co_yield status;
                    }
                }
                else
                {
                    written_bytes += cur_bytes;
                }
            }
            while (static_cast<size_t>(written_bytes) < str.size());
            written_bytes = 0;
        }
        status._ret = coroutine_info::status_info::SUCCESS;
        log::_log_ptr->info(INFO, "message sent");
        co_return status;
    }

    template worker::coroutine_info worker::send(int fd, const http_request& output);
    template worker::coroutine_info worker::send(int fd, const http_response& output);

    scheduler::scheduler()
        : _execute_stream_arr(MAX_THREAD_SIZE)
        , _process_queue_arr(MAX_THREAD_SIZE)
        , _waiting_queue_arr(MAX_THREAD_SIZE)
        , _total_node_arr(MAX_THREAD_SIZE)
        , _spare_node_queue()
        , _sp()
    {
        std::function<void(int)> func = std::bind(&scheduler::thread_entry, this, std::placeholders::_1);
        for (unsigned int i = 0; i < MAX_THREAD_SIZE; ++i)
        {
            _total_node_arr[i].store(0, std::memory_order_relaxed);
            _execute_stream_arr[i]._th = std::thread{ func, i };
        }
    }

    scheduler::~scheduler()
    {
        for (auto& th_c : _execute_stream_arr)
        {
            th_c._cond.notify_all();
            th_c._th.join();
        }
        for (auto& pro_q : _process_queue_arr)
        {
            for (auto& node : pro_q)
            {
                utility::sys_close(node._fd);
            }
        }
        for (auto& wait_q : _waiting_queue_arr)
        {
            for (auto& node : wait_q)
            {
                utility::sys_close(node._fd);
            }
        }
    }

    scheduler::thread_variable::thread_variable()
        : _th()
        , _m()
        , _cond()
        , _sp()
        , _epoll_fd(-1)
        , _trigger_fd(-1)
    {
        _epoll_fd = epoll_create(1);
        if (_epoll_fd == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("epoll_create failed: {0:s}", strerror(errno)));
            throw interface_exception{ strerror(errno) };
        }
    }

    scheduler::thread_variable::~thread_variable()
    {
        utility::sys_close(_epoll_fd);
        utility::sys_close(_trigger_fd);
    }

    void scheduler::setTrigger(size_t group, int fd)
    {
        static epoll_event event{};
        event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLET;
        if (epoll_ctl(_execute_stream_arr[group]._epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("epoll_ctl failed: {0:s}", strerror(errno)));
            throw interface_exception{ strerror(errno) };
        }
        _execute_stream_arr[group]._trigger_fd = fd;
    }

    void scheduler::dispatch(int connection_fd)
    {
        static epoll_event event{};
        event.events = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLET;
        size_t min_task = -1;
        size_t group = 0;
        for (unsigned int index = 0; index < MAX_THREAD_SIZE; ++index)
        {
            if (min_task > _total_node_arr[index].load(std::memory_order_relaxed))
            {
                min_task = _total_node_arr[index].load(std::memory_order_relaxed);
                group = index;
            }
        }
        std::list<worker>& waiting_queue = _waiting_queue_arr[group];
        std::atomic<size_t>& total_node = _total_node_arr[group];
        thread_variable& ctrl = _execute_stream_arr[group];
        ++total_node;
        if (_spare_node_queue.size() == 0)
        {
            std::lock_guard lock(ctrl._sp);
            waiting_queue.emplace_back(connection_fd);
        }
        else
        {
            std::scoped_lock lock(ctrl._sp, _sp);
            _spare_node_queue.begin()->_fd = connection_fd;
            waiting_queue.splice(waiting_queue.end(), _spare_node_queue, _spare_node_queue.begin());
        }
        if (epoll_ctl(ctrl._epoll_fd, EPOLL_CTL_ADD, connection_fd, &event) == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("epoll_ctl failed: {0:s}", strerror(errno)));
            throw interface_exception{ strerror(errno) };
        }
        log::_log_ptr->debug(INFO, "fd dispatched");
        ctrl._cond.notify_one();
    }

    void scheduler::thread_entry(size_t group)
    {
        // 因协程调度器取代了epoll处理具体网络请求的功能，所以epoll在项目中的实际功能仅为定时启动机制的上位替代
        // 这导致监控epoll事件失去实际意义，下列定义只为满足epoll_wait函数调用的最低要求
        static epoll_event event;
        std::list<worker>& process_queue = _process_queue_arr[group];
        std::list<worker>& waiting_queue = _waiting_queue_arr[group];
        std::atomic<size_t>& total_node = _total_node_arr[group];
        thread_variable& ctrl = _execute_stream_arr[group];
        unsigned short all_request_unfinished = 1;
        int ret = 0;
        std::unique_lock lock(ctrl._m);
        while (!need_exit)
        {
            all_request_unfinished = 1;
            for (auto it = process_queue.begin(), failed_it = process_queue.begin(); it != process_queue.end();)
            {
                log::_log_ptr->debug(INFO, std::format("{0:d}: scheduled", it->_fd));
                if (!it->_self._h || it->_self._h.promise()._status._ret != worker::coroutine_info::status_info::UNFINISH)
                {
                    it->_self = it->parse();
                }
                else
                {
                    it->_self._h.resume();
                }
                switch (it->_self._h.promise()._status._ret)
                {
                case worker::coroutine_info::status_info::EXCEPTION:
                case worker::coroutine_info::status_info::FAILURE:
                case worker::coroutine_info::status_info::SUCCESS:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: completed", it->_fd));
                    failed_it = it;
                    ++it;
                    failed_it->_input.clear();
                    failed_it->_output.clear();
                    failed_it->_io = worker::coroutine_info{};
                    failed_it->_self = worker::coroutine_info{};
                    if (epoll_ctl(ctrl._epoll_fd, EPOLL_CTL_DEL, failed_it->_fd, nullptr) == -1)
                    {
                        log::_log_ptr->fatal(INFO, std::format("epoll_ctl failed: {0:s}", strerror(errno)));
                        throw interface_exception{ strerror(errno) };
                    }
                    utility::sys_close(failed_it->_fd);
                    failed_it->_fd = -1;
                    _sp.lock();
                    _spare_node_queue.splice(_spare_node_queue.end(), process_queue, failed_it);
                    _sp.unlock();
                    --total_node;
                    all_request_unfinished *= 0;
                    log::_log_ptr->info(INFO, "one request done");
                    break;
                case worker::coroutine_info::status_info::UNFINISH:
                    log::_log_ptr->debug(INFO, std::format("{0:d}: unfinished", it->_fd));
                    ++it;
                    all_request_unfinished *= 1;
                    break;   
                }
            }
            if (need_exit)
            {
                break;
            }
            else if (waiting_queue.size() != 0)
            {        
                ctrl._sp.lock();
                process_queue.splice(process_queue.end(), waiting_queue);
                ctrl._sp.unlock();
            }
            else if (process_queue.size() == 0)
            {
                log::_log_ptr->debug(INFO, "empty queue");
                ctrl._cond.wait(lock, [&](){ return waiting_queue.size() > 0 || need_exit == true; });
            }
            else if (all_request_unfinished == 1)
            {
                log::_log_ptr->debug(INFO, "epoll check");
                ret = epoll_wait(ctrl._epoll_fd, &event, 1, -1);
                if (need_exit)
                {
                    break;
                }
                if (ret == -1)
                {
                    log::_log_ptr->fatal(INFO, std::format("epoll_wait failed: {0:s}", strerror(errno)));
                    throw interface_exception{ strerror(errno) };
                }
            }
        }
    }
}