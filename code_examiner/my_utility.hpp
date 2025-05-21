#pragma once
#include <exception>

#include <cstring>
#include <cerrno>

#include <sys/sysinfo.h>
#include <unistd.h>
#include <fcntl.h>

#include "log.hpp"

namespace ns_examiner
{
    class examiner_exception : public std::exception
    {
    public:
        examiner_exception(const char* msg)
            : std::exception()
            , error_message(msg)
        {}

        examiner_exception(int err_num)
            : std::exception()
            , error_message(strerror(err_num))
        {}

        examiner_exception()
            : std::exception()
            , error_message(strerror(errno))
        {}

        inline virtual const char* what() const noexcept override
        {
            return error_message;
        }
    private:
        const char* error_message;
    };

    // 系统组件包装
    class utility
    {
    public:
        static int sys_open(const char *pathname, int flags, mode_t mode);
        static int sys_open(const char *pathname, int flags = 0);
        static int sys_close(int fd);
        static ssize_t sys_read(int fd, void* buf, size_t bytes);
        static ssize_t sys_write(int fd, const void* buf, size_t bytes);
        static bool is_memory_available();
    };
}