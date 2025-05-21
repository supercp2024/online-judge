#pragma once
#include <cstddef>
#include <string_view>
#include <array>

namespace my_log
{
    class log_level
    {
    public:
        enum class value : size_t
        {
            DEBUG = 0,
            INFO,
            WARN,
            ERR,
            FATAL,
            OFF
        };

        static inline const std::string_view& get_value(value v) noexcept
        {
            return _conv[static_cast<size_t>(v)];
        }
    private:
        constexpr static std::array<const std::string_view, 6> _conv = { "DEBUG", "INFO", "WARN", "ERR", "FATAL", "OFF" };
    };
}