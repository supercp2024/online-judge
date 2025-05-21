#pragma once
#include <condition_variable>
#include <functional>
#include <filesystem>
#include <utility>
#include <string>
#include <bitset>
#include <vector>
#include <mutex>

#include <sys/types.h>
#include <sys/stat.h>

#include "my_utility.hpp"
#include "setting.hpp"

namespace ns_examiner
{
    // 临时文件存储区单例管理模块    
    class file_manager
    {
    public:
        enum class file_type : unsigned int
        {
            original_file = 0,
            executable_file,
            error_file,
            input_file,
            output_file,
            pipe_file,
            directory
        };
    private:
        file_manager();
        ~file_manager();
    public:
        static void init() noexcept;
        // 临时文件外存路径
        static std::filesystem::path getDiskPath(unsigned int file_group, file_type f_t, const std::string& name);
        // 临时文件内存路径
        static std::filesystem::path getMemoryPath(unsigned int file_group, file_type f_t, const std::string& name);
    private:
        static std::filesystem::path _code_root_disk;
        static std::filesystem::path _code_root_memory;
        static const std::vector<std::string> _extension_conv;
    };
}