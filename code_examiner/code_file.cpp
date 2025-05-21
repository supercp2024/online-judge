#include "code_file.hpp"

namespace ns_examiner
{
    std::filesystem::path file_manager::_code_root_disk = "./code";
    std::filesystem::path file_manager::_code_root_memory = "/dev/shm/examiner/code";
    const std::vector<std::string> file_manager::_extension_conv = { ".cpp", ".out", "_err.txt", "_input.txt", "_output.txt", ".p", "" };
    
    file_manager::file_manager()
    {
        std::filesystem::path tem;
        std::filesystem::create_directories(_code_root_disk);
        for (unsigned int i = 0; i < MAX_THREAD_SIZE; ++i)
        {
            std::filesystem::create_directory(_code_root_disk / std::to_string(i + 1));
            tem = getDiskPath(i + 1, file_manager::file_type::pipe_file, "in");
            if (std::filesystem::exists(tem) == false && mkfifo(tem.c_str(), 0644) == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("connot create fifo: {0:s}", strerror(errno)));
                throw examiner_exception{};
            }
            tem = getDiskPath(i + 1, file_manager::file_type::pipe_file, "out");
            if (std::filesystem::exists(tem) == false && mkfifo(tem.c_str(), 0644) == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("connot create fifo: {0:s}", strerror(errno)));
                throw examiner_exception{};
            }
        }
        std::filesystem::create_directories(_code_root_memory);
        for (unsigned int i = 0; i < MAX_THREAD_SIZE; ++i)
        {
            std::filesystem::create_directory(_code_root_memory / std::to_string(i + 1));
            tem = getMemoryPath(i + 1, file_manager::file_type::pipe_file, "in");
            if (!std::filesystem::exists(tem) && mkfifo(tem.c_str(), 0644) == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("connot create fifo: {0:s}", strerror(errno)));
                throw examiner_exception{};
            }
            tem = getMemoryPath(i + 1, file_manager::file_type::pipe_file, "out");
            if (!std::filesystem::exists(tem) && mkfifo(tem.c_str(), 0644) == -1)
            {
                log::_log_ptr->fatal(INFO, std::format("connot create fifo: {0:s}", strerror(errno)));
                throw examiner_exception{};
            }
        }
        log::_log_ptr->debug(INFO, "created user code directories");
    }

    file_manager::~file_manager()
    {                 
        std::filesystem::remove_all(_code_root_disk);
        std::filesystem::remove_all(_code_root_memory);
        log::_log_ptr->debug(INFO, "deleted user code directories");
    }

    void file_manager::init() noexcept
    {
        static file_manager instance;
    }

    std::filesystem::path file_manager::getDiskPath(unsigned int file_group, file_type f_t, const std::string& name)
    {    
        return _code_root_disk / std::to_string(file_group) / (name + _extension_conv[static_cast<unsigned int>(f_t)]);
    }

    std::filesystem::path file_manager::getMemoryPath(unsigned int file_group, file_type f_t, const std::string& name)
    {    
        return _code_root_memory / std::to_string(file_group) / (name + _extension_conv[static_cast<unsigned int>(f_t)]);
    }
}