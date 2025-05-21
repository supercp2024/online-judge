#pragma once
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <array>

#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include "my_utility.hpp"
#include "code_file.hpp"
#include "setting.hpp"

#include <jsoncpp/json/json.h>

namespace ns_examiner
{
    // 代码校验服务
    class task
    {
    private:
        using call_back = std::function<std::filesystem::path(unsigned int, file_manager::file_type, const std::string&)>;
        // using call_back = std::filesystem::path (*)(unsigned int, file_manager::file_type, const std::string&);
    public:
        task();
        // return 0 : success
        // return -1 : complie error
        // return -2 : runtime error
        void start(unsigned int group, const Json::Value& input, Json::Value& output);
    private:
        void examine(const std::string& name, call_back getPath, const Json::Value& input, Json::Value& output);
        // 编译模块
        bool complie(const std::filesystem::path& cpp_file_path, const std::filesystem::path& exe_file_path, 
            const std::filesystem::path& err_file_path, Json::Value& output);
        // 执行模块
        bool execute(const std::filesystem::path& exe_file_path, const std::filesystem::path& err_file_path, 
            const std::filesystem::path& path_pull, const std::filesystem::path& path_push, const Json::Value& example, 
            std::string& answer, Json::Value& output);
        // 通信模块
        void transmit(int read_fd, int write_fd, std::string& input, std::string& output);
        // 管道读取结果
        ssize_t pipeRead(int read_fd, std::string& read_buf, ssize_t& read_bytes);
        // 管道输入测试用例
        ssize_t pipeWrite(int write_fd, std::string& write_buf, ssize_t& written_bytes);
        // 设置临时文件存储路径
        void setGroup(unsigned int group);
    private:
        std::mutex _ctrl;
        std::condition_variable _cond;
        unsigned int _group;
    };
}