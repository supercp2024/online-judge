#include "task.hpp"

namespace ns_examiner
{
    task::task()
        : _ctrl()
        , _cond()
        , _group(0)
    {}

    void task::start(unsigned int group, const Json::Value& input, Json::Value& output)
    {
        std::fstream fout;
        std::string name = input["name"].asString();
        std::string original_file = input["code"].asString();
        setGroup(group);
        if (utility::is_memory_available())
        {
            log::_log_ptr->debug(INFO, "more memory use");
            fout.open(file_manager::getMemoryPath(group, file_manager::file_type::original_file, name), std::ios::out);
            fout.write(original_file.data(), original_file.size());
            fout.close();
            examine(name, &file_manager::getMemoryPath, input, output);
        }
        else
        {
            log::_log_ptr->debug(INFO, "more disk use");
            fout.open(file_manager::getDiskPath(group, file_manager::file_type::original_file, name), std::ios::out);
            fout.write(original_file.data(), original_file.size());
            fout.close();
            examine(name, &file_manager::getDiskPath, input, output);
        }
    }

    void task::setGroup(unsigned int group)
    {
        _group = group;
    }

    void task::examine(const std::string& name, call_back getPath, const Json::Value& input, Json::Value& output)
    {
        std::filesystem::path exe_file_path = getPath(_group, file_manager::file_type::executable_file, name);
        std::filesystem::path cpp_file_path = getPath(_group, file_manager::file_type::original_file, name);
        std::filesystem::path err_file_path = getPath(_group, file_manager::file_type::error_file, name);
        std::filesystem::path path_pull = getPath(_group, file_manager::file_type::pipe_file, "in");
        std::filesystem::path path_push = getPath(_group, file_manager::file_type::pipe_file, "out");
        std::string answer;
        bool is_match = false;
        std::unique_lock lock(_ctrl, std::try_to_lock_t{});
        log::_log_ptr->debug(INFO, "complie start");
        if (!complie(cpp_file_path, exe_file_path, err_file_path, output))
        {
            return;
        }
        log::_log_ptr->debug(INFO, "execute start");
        for (unsigned int i = 0; i < input["inputs"].size(); ++i)
        {
            if (!execute(exe_file_path, err_file_path, path_pull, path_push, input["inputs"][i], answer, output))
            {
                return;
            }
            else
            {
                std::string tem;
                for (auto it = input["outputs"][i].begin(); it != input["outputs"][i].end(); ++it)
                {
                    tem = it->asString();
                    if (answer == it->asString())
                    {
                        is_match = true;
                        break;
                    }
                }
                if (!is_match)
                {
                    output["status"] = "wrong";
                    output["which"] = input["inputs"][i]["example"];
                    log::_log_ptr->debug(INFO, "example wrong");
                    remove(exe_file_path);
                    remove(err_file_path);
                    return;
                }
            }
            is_match = false;
        }
        remove(exe_file_path);
        remove(err_file_path);
        output["status"] = "correct";
        log::_log_ptr->debug(INFO, "all examples correct");
    }
    
    bool task::complie(const std::filesystem::path& cpp_file_path, const std::filesystem::path& exe_file_path, 
        const std::filesystem::path& err_file_path, Json::Value& output)
    {
        std::unique_lock lock(_ctrl, std::try_to_lock_t{});
        pid_t proc = fork();
        if (proc == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("fork failed: {0:s}", strerror(errno)));
            throw examiner_exception{ strerror(errno) };
        }
        else if (proc == 0)
        {
            int err_file_fd = utility::sys_open(err_file_path.c_str(), O_TRUNC | O_WRONLY | O_CREAT, 0644);
            dup2(err_file_fd, stderr->_fileno);
            execlp("g++", "g++", "-std=c++20", "-o", exe_file_path.c_str(), cpp_file_path.c_str(), nullptr);
            log::_log_ptr->fatal(INFO, std::format("process replacement failed: {0:s}", strerror(errno)));
            throw examiner_exception{ strerror(errno) };
        }
        else
        {
            if (waitpid(0, nullptr, 0) == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("waitpid failed: {0:s}", strerror(errno)));
                throw examiner_exception{ strerror(errno) };
            }
            remove(cpp_file_path);
        }
        if (std::filesystem::file_size(err_file_path) != 0)
        {
            std::ifstream fin(err_file_path);
            std::string err_msg;
            err_msg.resize(std::filesystem::file_size(err_file_path));
            fin.read(err_msg.data(), err_msg.size());
            fin.close();
            remove(err_file_path);
            output["status"] = "comerr";
            output["msg"] = err_msg;
            log::_log_ptr->debug(INFO, "complie error");
            return false;
        }
        log::_log_ptr->debug(INFO, "complie success");
        return true;
    }

    bool task::execute(const std::filesystem::path& exe_file_path, const std::filesystem::path& err_file_path, 
        const std::filesystem::path& path_pull, const std::filesystem::path& path_push, const Json::Value& example, 
        std::string& answer, Json::Value& output)
    {
        std::vector<std::string> args_storge;
        std::vector<const char*> args;
        std::string raw_example;
        int proc = 0;
        args_storge.push_back(exe_file_path.string());
        for (auto it = example["args"].begin(); it != example["args"].end(); ++it)
        {
            args_storge.push_back(it->asString());
        }
        for (auto it = args_storge.begin(); it != args_storge.end(); ++it)
        {
            args.push_back(it->c_str());
        }
        args.push_back(nullptr);
        proc = fork();
        if (proc == -1)
        {
            log::_log_ptr->fatal(INFO, std::format("fork failed: {0:s}", strerror(errno)));
            throw examiner_exception{};
        }
        else if (proc == 0)
        {
            int err_file_fd = utility::sys_open(err_file_path.c_str(), O_TRUNC | O_WRONLY);
            int user_in_fd = utility::sys_open(path_push.c_str(), O_RDONLY);
            int user_out_fd = utility::sys_open(path_pull.c_str(), O_WRONLY);
            Json::Value& rlimits = output["rlimits"];
            rlimit rl;
            for (unsigned int i = 0; i < rlimits.size(); ++i)
            {
                rl.rlim_cur = rlimits[i][1].asUInt();
                rl.rlim_max = rlimits[i][2].asUInt();
                if (setrlimit(rlimits[i][0].asUInt(), &rl) == -1)
                {
                    throw examiner_exception{};
                }
            }
            dup2(err_file_fd, stderr->_fileno);
            dup2(user_in_fd, stdin->_fileno);
            dup2(user_out_fd, stdout->_fileno);
            execv(exe_file_path.c_str(), const_cast<char**>(args.data()));
            log::_log_ptr->fatal(INFO, std::format("process replacement failed: {0:s}", strerror(errno)));
            throw examiner_exception{};
        }
        else
        {
            raw_example = example["example"].asString();
            int server_out_fd = utility::sys_open(path_push.c_str(), O_WRONLY);
            int server_in_fd = utility::sys_open(path_pull.c_str(), O_RDONLY);
            int arg = fcntl(server_in_fd, F_GETFD, 0);
            if (arg == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("fcntl failed: {0:s}", strerror(errno)));
                throw examiner_exception{};
            }
            arg |= O_NONBLOCK;
            if (fcntl(server_in_fd, F_SETFD, server_in_fd))
            {
                log::_log_ptr->fatal(INFO, std::format("fcntl failed: {0:s}", strerror(errno)));
                throw examiner_exception{};
            }
            arg = fcntl(server_out_fd, F_GETFD, 0);
            if (arg == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("fcntl failed: {0:s}", strerror(errno)));
                throw examiner_exception{};
            }
            arg |= O_NONBLOCK;
            if (fcntl(server_out_fd, F_SETFD, server_out_fd))
            {
                log::_log_ptr->fatal(INFO, std::format("fcntl failed: {0:s}", strerror(errno)));
                throw examiner_exception{};
            }
            transmit(server_in_fd, server_out_fd, raw_example, answer);
            int status = 0;
            if (waitpid(0, &status, 0) == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("waitpid failed: {0:s}", strerror(errno)));
                throw examiner_exception{};
            }
            if (WTERMSIG(status) != 0)
            {
                std::string err_msg;
                char* sig_des = strsignal(WTERMSIG(status));
                err_msg.append(sig_des);
                err_msg += '\n';
                std::ifstream fin(err_file_path);
                size_t sig_size = err_msg.size();
                err_msg.resize(sig_size + std::filesystem::file_size(err_file_path));
                fin.read(err_msg.data() + sig_size, err_msg.size() - sig_size);
                fin.close();
                remove(exe_file_path);
                remove(err_file_path);
                output["status"] = "runerr";
                output["which"] = raw_example;
                output["msg"] = err_msg;
                log::_log_ptr->debug(INFO, "runerr");
                return false;
            }
        }
        log::_log_ptr->debug(INFO, "one example success");
        return true;
    }

    void task::transmit(int read_fd, int write_fd, std::string& input, std::string& output)
    {
        int max_fd = (read_fd > write_fd ? read_fd : write_fd) + 1;
        unsigned int ctrl_symbol = 0;
        ssize_t ret = 0;
        fd_set read_set;
        fd_set write_set;
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_SET(read_fd, &read_set);
        FD_SET(write_fd, &write_set);
        fd_set read_set_buf = read_set;
        fd_set write_set_buf = write_set;
        timeval timeout = {.tv_sec = 0, .tv_usec = 0};
        ssize_t read_bytes = 0;
        ssize_t written_bytes = 0;
        do
        {
            if (ctrl_symbol != 2)
            {
                if (select(max_fd, &read_set_buf, &write_set_buf, nullptr, &timeout) == -1)
                {
                    log::_log_ptr->fatal(INFO, std::format("select failed: {0:s}", strerror(errno)));
                    throw examiner_exception{};
                }
            }
            if (FD_ISSET(write_fd, &write_set_buf))
            {
                ret = pipeWrite(write_fd, input, written_bytes);
                if (ret == -1)
                {
                    switch (errno)
                    {
                    case EPIPE:
                        FD_ZERO(&write_set);
                        utility::sys_close(write_fd);
                        ++ctrl_symbol;
                        break;
                    }
                }
                else
                {
                    FD_ZERO(&write_set);
                    utility::sys_close(write_fd);
                    ++ctrl_symbol;
                }
            }
            if (FD_ISSET(read_fd, &read_set_buf))
            {
                ret = pipeRead(read_fd, output, read_bytes);
                if (ret == 0)
                {
                    FD_ZERO(&read_set);
                    utility::sys_close(read_fd);
                    output.resize(read_bytes);
                    ++ctrl_symbol;
                }
            }
            read_set_buf = read_set;
            write_set_buf = write_set;
        }
        while (ctrl_symbol != 2);
    }

    ssize_t task::pipeRead(int read_fd, std::string& read_buf, ssize_t& read_bytes)
    {
        ssize_t cur_read_bytes = 0;
        if (read_buf.size() == 0)
        {
            read_buf.resize(256);
        }
        do
        {
            if (read_buf.size() == static_cast<size_t>(read_bytes))
            {
                read_buf.resize(read_buf.size() * 2);
            }
            cur_read_bytes = utility::sys_read(read_fd, read_buf.data() + read_bytes, read_buf.size() - read_bytes);
            if (cur_read_bytes > 0)
            {
                read_bytes += cur_read_bytes;
            }      
        }
        while (cur_read_bytes > 0);
        return cur_read_bytes;
    }

    ssize_t task::pipeWrite(int write_fd, std::string& write_buf, ssize_t& written_bytes)
    {
        ssize_t cur_written_bytes = 0;
        do
        {
            cur_written_bytes = utility::sys_write(write_fd, write_buf.data() + written_bytes, write_buf.size() - written_bytes);
            if (cur_written_bytes > 0)
            {
                written_bytes += cur_written_bytes;
            }
        }
        while (cur_written_bytes != -1 && write_buf.size() != static_cast<size_t>(written_bytes));
        return cur_written_bytes;
    }
}