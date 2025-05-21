#include "worker.hpp"

namespace ns_examiner
{
    thread_pool::worker::worker(int fd)
        : _thr()
        , _ctrl()
        , _cond()
        , _net_buf()
        , _index(0)
        , _cur_fd(fd)
        , _client_msg()
        , _client_msg_len(0)
    {
        _net_buf.resize(NET_BUFFER_SIZE);
        static size_t i = 0;
        _index = ++i;
    }

    thread_pool::worker::worker(worker&& other) noexcept
        : _thr(std::move(other._thr))
        , _ctrl()
        , _cond()
        , _net_buf(std::move(other._net_buf))
        , _index(other._index)
        , _cur_fd(other._cur_fd)
        , _client_msg(other._client_msg)
        , _client_msg_len(other._client_msg_len)
    {}

    thread_pool::worker& thread_pool::worker::operator=(worker&& other) noexcept 
    {
        if (this != &other) 
        {
            _thr = std::move(other._thr);
            _net_buf = std::move(other._net_buf);
            _cur_fd = other._cur_fd;
            _index = other._index;
            _client_msg = other._client_msg;
            _client_msg_len = other._client_msg_len;
        }
        return *this;
    }

    thread_pool::thread_pool()
        : _thr_queue()
    {
        auto call_back = std::bind(&thread_pool::thread_entry, this, std::placeholders::_1);
        for (size_t i = 0; i < MAX_THREAD_SIZE; ++i)
        {
            worker* ptr = new worker;
            ptr->_thr = std::thread{ call_back, ptr };
            log::_log_ptr->debug(INFO, "new worker created");
        }
    }

    void thread_pool::thread_entry(worker* self_ptr)
    {
        http_request input;
        http_response output;
        std::unique_lock lock(self_ptr->_ctrl, std::try_to_lock_t{});
        while (!need_exit)
        {
            _thr_queue.push(self_ptr);
            log::_log_ptr->debug(INFO, std::format("worker {0:d} sleep", self_ptr->_index));
            input.clear();
            output.clear();
            self_ptr->_cond.wait(lock);
            log::_log_ptr->debug(INFO, std::format("worker {0:d} awake", self_ptr->_index));
            if (need_exit)
            {
                break;
            }
            if (!this->recv(input, self_ptr))
            {
                input.clear();
                utility::sys_close(self_ptr->_cur_fd);
                log::_log_ptr->debug(INFO, std::format("worker {0:d} request out of time", self_ptr->_index));
                continue;
            }
            bodyProcessing(input, output, self_ptr);
            output._version = "1.0";
            output._code = "200";
            output._status = "OK";
            output.headerStorge("Content-Length", true, std::to_string(output._content.size()), true);
            if (!this->send(std::move(output), self_ptr))
            {
                input.clear();
                log::_log_ptr->debug(INFO, std::format("worker {0:d} response out of time", self_ptr->_index));
            }
            utility::sys_close(self_ptr->_cur_fd);
        }
        self_ptr->_thr.detach();
        delete self_ptr;
        log::_log_ptr->debug(INFO, std::format("worker {0:d} quit", self_ptr->_index));
    }

    void thread_pool::bodyProcessing(http_request& input, http_response& output, worker* self_ptr)
    {
        task job;
        Json::Reader reader;
        Json::FastWriter writer;
        Json::Value parsed_content;
        Json::Value unserialized_content;
        std::fstream f;
        reader.parse(input._content, parsed_content);
        long long group = self_ptr->_index;
        log::_log_ptr->debug(INFO, std::format("worker {0:d} ready for job", self_ptr->_index));
        job.start(group, parsed_content, unserialized_content);
        log::_log_ptr->debug(INFO, std::format("worker {0:d} job done", self_ptr->_index));
        output._content = writer.write(unserialized_content);
    }

    bool thread_pool::recv(http_request& input, worker* self_ptr)
    {
        int fd = self_ptr->_cur_fd;
        fd_set read_set;
        fd_set cur_read_set;
        FD_ZERO(&read_set);
        FD_SET(fd, &read_set);
        cur_read_set = read_set;
        timeval tem_time_out = NET_TIME_OUT;
        unsigned int out_of_time = 0;
        ssize_t read_bytes = 0;
        auto begin = self_ptr->_net_buf.begin();
        do
        {
            if (select(fd + 1, &cur_read_set, nullptr, nullptr, &tem_time_out) == -1)       
            {
                log::_log_ptr->fatal(INFO, std::format("worker {0:d} select failed: {1:s}", self_ptr->_index, strerror(errno)));
                throw examiner_exception{};
            }
            if (FD_ISSET(fd, &cur_read_set))
            {
                read_bytes = utility::sys_read(fd, self_ptr->_net_buf.data(), NET_BUFFER_SIZE);
                if (read_bytes == 0)
                {
                    return false;
                }
            }
            else
            {
                if (out_of_time == 3)
                {
                    log::_log_ptr->debug(INFO, std::format("client out of time, worker {0:d} connot receive message", self_ptr->_index));
                    return false;
                }
                ++out_of_time;
            }
            tem_time_out = NET_TIME_OUT;
            cur_read_set = read_set;
        }
        while (!input.deserialize(begin, begin + read_bytes));
        log::_log_ptr->debug(INFO, std::format("worker {0:d} has received a message", self_ptr->_index));
        return true;
    }

    bool thread_pool::send(http_response&& output, worker* self_ptr)
    {
        int fd = self_ptr->_cur_fd;
        fd_set write_set;
        fd_set cur_write_set;
        FD_ZERO(&write_set);
        FD_SET(fd, &write_set);
        cur_write_set = write_set;
        timeval tem_time_out = NET_TIME_OUT;
        unsigned int out_of_time = 0;
        auto data = output.serialize();
        ssize_t written_bytes = 0;
        ssize_t cur_written_bytes = 0;  
        for (auto& str : data)
        {
            while (static_cast<size_t>(written_bytes) < str.size())
            {
                if (select(fd + 1, nullptr, &cur_write_set, nullptr, &tem_time_out) == -1)       
                {
                    log::_log_ptr->fatal(INFO, std::format("worker {0:d} select failed: {1:s}", self_ptr->_index, strerror(errno)));
                    throw examiner_exception{};
                } 
                if (FD_ISSET(fd, &cur_write_set))
                {
                    cur_written_bytes = utility::sys_write(fd, str.data() + written_bytes, str.size() - written_bytes);                   
                    if (cur_written_bytes == -1)
                    {
                        switch (errno)
                        {
                        case EPIPE:
                            return false;
                            break;
                        }
                    }
                    else
                    {
                        written_bytes += cur_written_bytes;
                    }
                }
                else
                {
                    if (out_of_time == 3)
                    {
                        log::_log_ptr->debug(INFO, std::format("client out of time, worker {0:d} connot send message", self_ptr->_index));
                        return false;
                    }
                    ++out_of_time;
                }
                tem_time_out = NET_TIME_OUT;
                cur_write_set = write_set;
            }
            written_bytes = 0;
        }
        log::_log_ptr->debug(INFO, std::format("worker {0:d} has sent a message", self_ptr->_index));
        return true;
    }
}