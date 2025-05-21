#include "my_utility.hpp"

namespace ns_interface
{
    int utility::sys_open(const char *pathname, int flags, mode_t mode)
    {
        int ret = 0;
        do
        {
            ret = ::open(pathname, flags, mode);
            if (ret >= 0)
            {
                return ret;
            }
            else if (ret == -1)
            {
                switch (errno)
                {
                case EINTR:
                    continue;
                    break;
                default:
                    throw interface_exception{};
                    break;
                }
            }
        }
        while (ret != 0);
        return 0;
    }

    int utility::sys_open(const char *pathname, int flags)
    {
        int ret = 0;
        do
        {
            ret = ::open(pathname, flags);
            if (ret >= 0)
            {
                return ret;
            }
            else if (ret == -1)
            {
                switch (errno)
                {
                case EINTR:
                    continue;
                    break;
                default:
                    log::_log_ptr->fatal(INFO, std::format("open err: {0:s}", strerror(errno)));
                    throw interface_exception{};
                    break;
                }
            }
        }
        while (1);
        return 0;
    }

    int utility::sys_close(int fd)
    {
        int ret = 0;
        do
        {
            ret = ::close(fd);
            if (ret == 0)
            {
                return ret;
            }
            else if (ret == -1)
            {
                switch (errno)
                {
                case EINTR:
                    continue;
                    break;
                default:
                    log::_log_ptr->fatal(INFO, std::format("close err: {0:s}", strerror(errno)));
                    throw interface_exception{};
                    break;
                }
            }
        }
        while (1);
        return 0;
    }

    ssize_t utility::sys_read(int fd, void* buf, size_t bytes)
    {
        int ret = 0;
        do
        {
            ret = ::read(fd, buf, bytes);
            if (ret >= 0)
            {
                return ret;
            }
            else if (ret == -1)
            {
                switch (errno)
                {
                case EINTR:
                    continue;
                    break;
                case EAGAIN:
                    return ret;
                    break;
                case ECONNRESET:
                    return 0;
                    break;
                default:
                    log::_log_ptr->fatal(INFO, std::format("read err: {0:s}", strerror(errno)));
                    throw interface_exception{};
                    break;
                }
            }
        }
        while (1);
        return 0;
    }

    ssize_t utility::sys_write(int fd, const void* buf, size_t bytes)
    {
        int ret = 0;
        do
        {
            ret = ::write(fd, buf, bytes);
            if (ret >= 0)
            {
                return ret;
            }
            else if (ret == -1)
            {
                switch (errno)
                {
                case EINTR:
                    continue;
                    break;
                case EAGAIN:
                    return ret;
                    break;
                case EPIPE:
                    return ret;
                    break;
                default:
                    log::_log_ptr->fatal(INFO, std::format("write err: {0:s}", strerror(errno)));
                    throw interface_exception{};
                    break;
                }
            }
        }
        while (1);
        return 0;
    }

    bool utility::is_memory_available()
    {
        struct sysinfo info;
        if (sysinfo(&info) != 0) 
        {
            log::_log_ptr->fatal(INFO, std::format("sysinfo err: {0:s}", strerror(errno)));
            throw interface_exception{};
        }
        const double free_ratio = static_cast<double>(info.freeram) / info.totalram;
        return free_ratio > 0.2;
    }
}