#pragma once
#include <filesystem>
#include <ranges>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <variant>
#include <functional>
#include <unordered_map>

#include "setting.hpp"
#include "message.hpp"

namespace my_log
{
    // 样式设计
    // ti 打印线程id    
    // pi 打印进程id    
    // fi 打印文件      
    // li 打印行号  
    // co 打印列号   
    // lv 打印等级      
    // lg 打印日志器    
    // ms 打印正文   
    // 支持chrono样式

    class log_format_setting
    {
    private:
        log_format_setting();
    public:
        inline std::filesystem::path& getPatPath() noexcept
        {
            if (_pat_init_path == "")
            {
                _pat_init_path = std::filesystem::current_path();
                _pat_init_path.append("pattern.conf");
            }
            return _pat_init_path;
        }

        inline size_t size() const noexcept
        {
            return _pat_form.size();
        }
        static log_format_setting& getInstance() noexcept;
        void insert(const std::string& name, const std::string& format_str);
        void remove(const std::string& name);
        void modify(const std::string& name, const std::string& format_str);
        void flush();
        const std::string& at(const std::string& name) const;
    private:
        void load_init();
        void format_check(const std::string& name, const std::string& format_str);
    private:
        std::filesystem::path _pat_init_path;
        std::unordered_map<std::string, std::string> _pat_form;
    };
}

template<>
class std::formatter<my_log::log_message>
{
public:
    formatter()
    {}

    // error: this function isn't constexpr, please use std::vformat for instead
    template<typename ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& context)
    {
        auto it = context.begin();
        auto end = context.end();
        if (it == end)
        {
            if (*it == '}')
            {
                _type_status = _format_type::EMPTY;
                return it;
            }
            else
            {
                throw std::format_error("Missing '}' in format string.");
            }
        }
        size_t total_count = 0;
        size_t type_count = 0;
        bool is_target = false;
        bool has_type = false;
        while (it != end)
        {
            if ((*it - 'a' >= 0 && *it - 'z' <= 0) || (*it - 'A' >= 0 && *it - 'Z' <= 0))
            {
                if (type_count == 0)
                {
                    _type_status = _format_type::LOG_MSG;
                    has_type = true;
                }
                if (type_count == 2 && _type_status == _format_type::LOG_MSG)
                {
                    throw std::format_error("too many log message in signle \"{}\"");
                }
                if (total_count == 128)
                {
                    throw std::format_error("too much type identifier in single \"{}\"");
                }
                _str += *it;
                ++type_count;
                ++total_count;
                is_target = true;
            }
            else if (*it == '%' && (_type_status != _format_type::LOG_MSG &&
                (type_count == 0 || (type_count % 2 == 0 || _str[total_count - 1] == '%'))))
            {
                if (type_count == 0)
                {
                    _type_status = _format_type::CHRONO;
                    has_type = true;
                }
                if (total_count == 128)
                {
                    throw std::format_error("too much type identifier in single \"{}\"");
                }
                _str += *it;
                ++type_count;
                is_target = true;
            }
            else if (*it == '}')
            {
                return it;
            }
            else
            {
                _str += *it;
                ++total_count;
            }
            if (type_count > 0 && is_target == false)
            {
                throw std::format_error("error type identifier in format string.");
            }
            ++it;
            is_target = false;
        }
        if (!has_type)
        {
            throw std::format_error("no type infomation.");
        }
        return it;
    }

    template<typename FmtContext>
    FmtContext::iterator format(const my_log::log_message& log_msg, FmtContext& context) const
    {
        if (_type_status != _format_type::EMPTY)
        {
            std::string tem = _str;
            _str.clear();
            if (_type_status == _format_type::CHRONO)
            {
                _str += "{0:";
                _str += tem;
                _str += '}';
                std::vformat_to(context.out(), _str, std::make_format_args(log_msg._cur_time));
            }
            else if (_type_status == _format_type::LOG_MSG)
            {
                _str += "{0:";
                _str += _get_log_msg_type.at(tem);
                _str += '}';
                if (tem == "ti")
                {
                    std::vformat_to(context.out(), _str, std::make_format_args(log_msg._tid));
                }
                else if (tem == "pi")
                {
                    std::vformat_to(context.out(), _str, std::make_format_args(log_msg._pid));
                }
                else if (tem == "li")
                {
                    std::vformat_to(context.out(), _str, std::make_format_args(log_msg._line));
                }
                else if (tem == "co")
                {
                    std::vformat_to(context.out(), _str, std::make_format_args(log_msg._column));
                }
                else if (tem == "fi")
                {
                    std::ranges::copy(log_msg._file, context.out());
                }
                else if (tem == "lv")
                {
                    std::ranges::copy(my_log::log_level::get_value(log_msg._level), context.out());
                }
                else if (tem == "lg")
                {
                    std::ranges::copy(log_msg._logger, context.out());
                }
                else if (tem == "ms")
                {
                    std::ranges::copy(log_msg._payload, context.out());
                }
                else
                {
                    throw std::format_error("unregistered type");
                }
            }
        }
        _str.clear();
        return context.out();
    }
private:
    enum class _format_type : size_t
    {
        EMPTY = 0,
        CHRONO,
        LOG_MSG
    };
private:
    _format_type _type_status = _format_type::EMPTY;
    static const std::unordered_map<std::string_view, std::string_view> _get_log_msg_type;
    static std::string _str;
};