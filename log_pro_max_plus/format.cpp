#include "format.hpp"

const std::unordered_map<std::string_view, std::string_view> std::formatter<my_log::log_message>::_get_log_msg_type =
{ { "ti", "d" }, { "pi", "d" }, { "li", "d" }, { "co", "d" }, { "fi", "s" }, { "lv", "s" }, { "lg", "s" }, { "ms", "s" } };

std::string std::formatter<my_log::log_message>::_str{};

namespace my_log
{
    void log_format_setting::insert(const std::string& name, const std::string& format_str)
    {
        if (_pat_form.find(name) != _pat_form.end())
        {
            std::cerr << "log_format_setting::insert: registered name" << std::endl;
            std::terminate();
        }
        format_check(name, format_str);
        _pat_form[name] = format_str;
    }

    void log_format_setting::remove(const std::string& name)
    {
        if (_pat_form.find(name) == _pat_form.end())
        {
            std::cerr << "log_format_setting::remove: unregistered name" << std::endl;
            std::terminate();
        }
        _pat_form.erase(name);
    }

    void log_format_setting::modify(const std::string& name, const std::string& format_str)
    {
        if (_pat_form.find(name) == _pat_form.end())
        {
            std::cerr << "log_format_setting::modify: unregistered name" << std::endl;
            std::terminate();
        }
        format_check(name, format_str);
        _pat_form.at(name) = format_str;
    }

    const std::string& log_format_setting::at(const std::string& name) const
    {
        if (_pat_form.find(name) == _pat_form.end())
        {
            std::cerr << "log_format_setting::at: unregistered name" << std::endl;
            std::terminate();
        }
        return _pat_form.at(name);
    }

    void log_format_setting::format_check(const std::string& name, const std::string& format_str)
    {
        static log_message example("unexist logger", "unexist file", 0, 0, log_level::value::OFF, "empty");
        if (name == "")
        {
            std::cerr << "log_format_setting::format_check: name is empty" << std::endl;
            std::terminate();
        }
        if (name.size() + format_str.size() > MAX_PATTERN_LEN)
        {
            std::cerr << "log_format_setting::format_check: name is out of space" << std::endl;
            std::terminate();
        }
        try
        {
            std::string formatted_str = std::vformat(format_str, std::make_format_args(example));
            if (formatted_str.size() > MAX_FORMATTED_STR_LEN)
            {
                std::cerr << "log_format_setting::format_check: formatted_str is out of space" << std::endl;
                std::terminate();
            }
            std::cout << "样式效果: " << std::endl;
            std::cout << formatted_str << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "log_format_setting::format_check: " << e.what() << std::endl;
            std::terminate();
        }
    }

    log_format_setting& log_format_setting::getInstance() noexcept
    {
        static log_format_setting instance;
        return instance;
    }

    log_format_setting::log_format_setting()
        : _pat_init_path("")
        , _pat_form()
    {
        load_init();
    }

    void log_format_setting::flush()
    {
        std::fstream pattern_init;
        try
        {
            pattern_init.open(getPatPath(), std::ios::out | std::ios::binary | std::ios::trunc);
            for (auto& it : _pat_form)
            {
                pattern_init << it.first << '\n' << it.second << '\n';
            }
        }
        catch (const std::ios_base::failure& e)
        {
            std::cerr << "log_format_setting::flush: " << e.what() << std::endl;
            std::terminate();
        }
    }

    void log_format_setting::load_init()
    {
        std::fstream pattern_init;
        std::string name;
        std::string formatted_str;
        try
        {
            if (std::filesystem::exists(getPatPath()))
            {
                pattern_init.open(getPatPath(), std::ios::in | std::ios::binary);
                for (size_t i = 0; !pattern_init.eof(); ++i)
                {
                    std::getline(pattern_init, name, '\n');
                    std::getline(pattern_init, formatted_str, '\n');
                    _pat_form.insert({ std::move(name), std::move(formatted_str) });
                    pattern_init.peek();
                }
            }
        }
        catch (const std::ios_base::failure& e)
        {
            std::cerr << "log_format_setting::load_init" << e.what() << std::endl;
            std::terminate();
        }
    }
}