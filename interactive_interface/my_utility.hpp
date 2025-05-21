#pragma once
#include <exception>
#include <atomic>

#include <cstring>
#include <cerrno>

#include <sys/sysinfo.h>
#include <unistd.h>
#include <fcntl.h>

#include "log.hpp"

namespace ns_interface
{
    class interface_exception : public std::exception
    {
    public:
        interface_exception(const char* msg)
            : std::exception()
            , error_message(msg)
        {}

        interface_exception(int err_num)
            : std::exception()
            , error_message(strerror(err_num))
        {}

        interface_exception()
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

    class utility
    {
    public:
        // 系统io包装函数
        static int sys_open(const char *pathname, int flags, mode_t mode);
        static int sys_open(const char *pathname, int flags = 0);
        static int sys_close(int fd);
        static ssize_t sys_read(int fd, void* buf, size_t bytes);
        static ssize_t sys_write(int fd, const void* buf, size_t bytes);
        static bool is_memory_available();
    };

    class spin_lock 
    {
    public:
        spin_lock()
            : _lock(ATOMIC_FLAG_INIT)
        {} 

        spin_lock(const spin_lock& other) = delete;
        spin_lock(spin_lock&& other) = delete;

        void lock() 
        {
            while (_lock.test_and_set(std::memory_order_acquire));
        }

        bool try_lock() 
        {
            return !_lock.test_and_set(std::memory_order_acquire);
        }
    
        void unlock() 
        {
            _lock.clear(std::memory_order_release);
        }
    
    private:
        std::atomic_flag _lock;
    };
}