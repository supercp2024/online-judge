#include "protocol.hpp"

namespace ns_interface
{
    const std::unordered_map<std::string_view, std::string_view> http_base::_msg_type = {
        { ".html", "text/html; charset=utf-8" }, { ".css", "text/css; charset=utf-8" }, { ".js", "text/javascript; charset=utf-8" }, 
        { ".json", "application/json; charset=utf-8" }, { ".ico", "image/x-icon" }
    };

    http_request::http_request()
        : http_base()
        , _method()
        , _url()
        , _version()
    {}   

    bool http_request::deserialize(std::string::iterator begin, std::string::iterator end)
    {
        if (begin == end)
        {
            return false;
        }
        switch (_ctrl._progress)
        {
        case 0:
            if (!parseTitle(begin, end, _method))
            {
                return false;
            }
            log::_log_ptr->debug(INFO, "req phase 1 complete");
            [[fallthrough]];
        case 1:
            if (!parseTitle(begin, end, _url))
            {
                return false;
            }
            log::_log_ptr->debug(INFO, "req phase 2 complete");
            [[fallthrough]];
        case 2:
            if (!parseTitle(begin, end, _version))
            {
                return false;
            }
            log::_log_ptr->debug(INFO, "req phase 3 complete");
            [[fallthrough]];
        case 3:
            if (!parseHeader(begin, end))
            {
                return false;
            }
            log::_log_ptr->debug(INFO, "req phase 4 complete");
            [[fallthrough]];
        case 4:
            if (_ctrl._final_content_size != 0)
            {
                if (!parseContent(begin, end))
                {
                    return false;
                }
            }
            else if (_header_table.find("Content-Length") != _header_table.end())
            {
                _ctrl._final_content_size = toInteger(_header_table["Content-Length"]);
                _content.resize(_ctrl._final_content_size);
                _ctrl._cur_content_size = 0;
                if (!parseContent(begin, end))
                {
                    return false;
                }
                ++_ctrl._progress;
            }
            else
            {
                ++_ctrl._progress;
            }
            log::_log_ptr->debug(INFO, "req phase 5 complete");
            [[fallthrough]];
        case 5:
            _ctrl._progress = 0;
            log::_log_ptr->debug(INFO, "req deserialize complete");
            return true;
            break;
        default:
            return false;
            break;
        }
    }

    std::vector<std::string_view> http_request::serialize() const
    {
        std::vector<std::string_view> ret;
        http_base::serialize(ret, { _method, _url, _version });
        log::_log_ptr->debug(INFO, "req serialize complete");
        return ret;
    }

    std::string http_request::serialize_sp() const
    {
        std::string ret;
        http_base::serialize_sp(ret, { _method, _url, _version });
        log::_log_ptr->debug(INFO, "req serialize complete");
        return ret;
    }

    void http_request::clear()
    {
        _method.clear();
        _url.clear();
        _version.clear();
        http_base::clear();
    }

    http_response::http_response()
        : http_base()
        , _version()
        , _code()
        , _status()
    {}

    bool http_response::deserialize(std::string::iterator begin, std::string::iterator end)
    {
        if (begin == end)
        {
            return false;
        }
        switch (_ctrl._progress)
        {
        case 0:
            if (!parseTitle(begin, end, _version))
            {
                return false;
            }
            log::_log_ptr->debug(INFO, "res phase 1 complete");
            [[fallthrough]];
        case 1:
            if (!parseTitle(begin, end, _code))
            {
                return false;
            }
            log::_log_ptr->debug(INFO, "res phase 2 complete");
            [[fallthrough]];
        case 2:
            if (!parseTitle(begin, end, _status))
            {
                return false;
            }
            log::_log_ptr->debug(INFO, "res phase 3 complete");
            [[fallthrough]];
        case 3:
            if (!parseHeader(begin, end))
            {
                return false;
            }
            log::_log_ptr->debug(INFO, "res phase 4 complete");
            [[fallthrough]];
        case 4:
            if (_ctrl._final_content_size != 0)
            {
                if (!parseContent(begin, end))
                {
                    return false;
                }
            }
            else if (_header_table.find("Content-Length") != _header_table.end())
            {
                _ctrl._final_content_size = toInteger(_header_table["Content-Length"]);
                _content.resize(_ctrl._final_content_size);
                _ctrl._cur_content_size = 0;
                if (!parseContent(begin, end))
                {
                    return false;
                }
                ++_ctrl._progress;
            }
            else
            {
                ++_ctrl._progress;
            }
            log::_log_ptr->debug(INFO, "res phase 5 complete");
            [[fallthrough]];
        case 5:
            _ctrl._progress = 0;
            log::_log_ptr->debug(INFO, "res deserialize complete");
            return true;
            break;
        default:
            return false;
            break;
        }
    }

    std::vector<std::string_view> http_response::serialize() const
    {
        std::vector<std::string_view> ret;
        http_base::serialize(ret, { _version, _code, _status });
        log::_log_ptr->debug(INFO, "res serialize complete");
        return ret;
    }

    std::string http_response::serialize_sp() const
    {
        std::string ret;
        http_base::serialize_sp(ret, { _version, _code, _status });
        log::_log_ptr->debug(INFO, "res serialize complete");
        return ret;
    }

    void http_response::clear()
    {
        _status.clear();
        _code.clear();
        _version.clear();
        http_base::clear();
    }

    http_base::completeness_control::completeness_control()
        : _buf()
        , _is_single_header_completed(true)
        , _key()
        , _final_content_size(0)
        , _cur_content_size(0)
        , _progress(0)
    {}

    void http_base::completeness_control::reset()
    {
        _buf.clear();
        _is_single_header_completed = true;
        _key = "";
        _final_content_size = 0;
        _cur_content_size = 0;
    }

    http_base::http_base()
        : _header_table()
        , _content()
        , _header_array()
    {}

    http_base::~http_base()
    {}

    bool http_base::parseTitle(std::string::iterator& begin, const std::string::iterator& end, std::string& parsed_str)
    {
        while (begin != end)
        {
            if (*begin == '\r');
            else if (*begin == ' ' || *begin == '\n')
            {
                ++begin;
                ++_ctrl._progress;
                return true;
            }
            else
            {
                parsed_str.push_back(*begin);
            }
            ++begin;
        }
        return false;
    }

    bool http_base::parseHeader(std::string::iterator& begin, const std::string::iterator& end)
    {
        while (begin != end)
        {
            if (*begin != '\r' && *begin != '\n')
            {
                _ctrl._is_single_header_completed = false;
            }
            else if (*begin == '\r')
            {
                ++begin;
                continue;
            }
            else
            {
                ++begin;
                ++_ctrl._progress;
                return true;
            }
            if (!_ctrl._is_single_header_completed)
            {
                while (begin != end)
                {
                    if (*begin == '\r')
                    {
                        _header_array.emplace_back(new std::string{ std::move(_ctrl._buf) });
                    }
                    else if (*begin == '\n')
                    {                  
                        _ctrl._is_single_header_completed = true;
                        _header_table.emplace(_ctrl._key, *_header_array.back());
                        ++begin;
                        break;
                    }
                    else if (*begin == ':')
                    {
                        _header_array.emplace_back(new std::string{ std::move(_ctrl._buf) });
                        _ctrl._key = *_header_array.back();
                    }
                    else if (*begin == ' ');
                    else
                    {
                        _ctrl._buf.push_back(*begin);
                    }
                    ++begin;
                }
            }
        }
        return false;
    }

    bool http_base::parseContent(std::string::iterator& begin, const std::string::iterator& end)
    {
        for (; begin != end && _ctrl._cur_content_size != _ctrl._final_content_size; ++_ctrl._cur_content_size, ++begin)
        {
            _content[_ctrl._cur_content_size] = *begin;
        }
        if (_ctrl._cur_content_size != _ctrl._final_content_size)
        {
            return false;
        }
        ++_ctrl._progress;
        _ctrl._final_content_size = 0;
        return true;
    }

    void http_base::serialize(std::vector<std::string_view>& ret, std::array<std::string_view, 3> input) const
    {
        ret.push_back(input[0]);
        ret.push_back(_space);
        ret.push_back(input[1]);
        ret.push_back(_space);
        ret.push_back(input[2]);
        ret.push_back(_blank);
        for (const auto& kv : _header_table)
        {
            ret.push_back(kv.first);
            ret.push_back(_header_blank);
            ret.push_back(kv.second);
            ret.push_back(_blank);
        }
        ret.push_back(_blank);
        if (_content.size() != 0)
        {
            ret.push_back(_content);
        }
    }

    void http_base::serialize_sp(std::string& ret, std::array<std::string_view, 3> input) const
    {
        ret += input[0];
        ret += _space;
        ret += input[1];
        ret += _space;
        ret += input[2];
        ret += _blank;
        for (const auto& kv : _header_table)
        {
            ret += kv.first;
            ret += _header_blank;
            ret += kv.second;
            ret += _blank;
        }
        ret += _blank;
        if (_content.size() != 0)
        {
            ret += _content;
        }
    }

    void http_base::headerStorge(std::string_view key, bool storge_key, std::string_view value, bool storge_value)
    {
        if (storge_key)
        {
            _header_array.emplace_back(new std::string{ key });
            key = *_header_array.back();
        }
        if (storge_value)
        {
            _header_array.emplace_back(new std::string{ value });
            value = *_header_array.back();
        }
        _header_table.emplace(key, value);
    }

    void http_base::clear()
    {
        _content.clear();
        _header_table.clear();
        _header_array.clear();
        _ctrl.reset();
    }

    unsigned long long http_base::toInteger(const std::string_view& number_str)
    {
        unsigned long long ret = 0;
        for (const auto& ch : number_str)
        {
            ret *= 10;
            ret += (ch - '0');
        }
        return ret;
    }
 
    url::url(const std::string& input)
        : _resourse_path()
        , _extension()
        , _parameters()
    {
        init(input);
    }

    url& url::operator=(const std::string& input)
    {
        _parameters.clear();
        _extension.clear();
        _resourse_path.clear();
        init(input);
        return *this;
    }

    void url::init(const std::string& input)
    {

        int pos = input.find_first_of('?');
        int next = 0; 
        if (pos == -1)
        {
            _resourse_path = url_decode(input);
        }
        else
        {
            _resourse_path = url_decode(input.substr(0, pos++));
            std::string_view para_str(input.data() + pos, input.size() - pos);
            std::string_view key;
            pos = 0;
            bool is_complete = false;
            bool is_and = false;
            while (!is_complete)
            {
                if (is_and)
                {
                    next = para_str.find_first_of('&', pos);
                    if (next == -1)
                    {
                        is_complete = true;
                        _parameters.emplace(url_decode(key), url_decode(para_str.substr(pos, para_str.size())));
                    }
                    else
                    {
                        _parameters.emplace(url_decode(key), url_decode(para_str.substr(pos, next++)));
                    }
                }
                else
                {
                    next = para_str.find_first_of('=', pos);
                    key = para_str.substr(pos, next++);
                }
                pos = next;
                is_and = !is_and;
            }
        }
        if (_resourse_path.has_extension())
        {
            _extension = _resourse_path.extension();
        }
        else
        {
            _extension = "";
        }
    }

    std::string url::url_decode(const std::string& encoded) 
    {
        std::ostringstream decoded;
        for (size_t i = 0; i < encoded.size(); ++i) 
        {
            if (encoded[i] == '%') 
            {
                if (i + 2 < encoded.size()) 
                {
                    int hex1 = std::tolower(encoded[i + 1]);
                    int hex2 = std::tolower(encoded[i + 2]);
                    int value = (hex1 >= 'a' ? hex1 - 'a' + 10 : hex1 - '0') * 16 +
                                (hex2 >= 'a' ? hex2 - 'a' + 10 : hex2 - '0');
                    decoded << static_cast<char>(value);
                    i += 2;
                }
            } 
            else if (encoded[i] == '+') 
            {
                decoded << ' ';
            } 
            else 
            {
                decoded << encoded[i];
            }
        }
        return decoded.str();
    }

    std::string url::url_decode(const std::string_view& encoded) 
    {
        std::ostringstream decoded;
        for (size_t i = 0; i < encoded.size(); ++i) 
        {
            if (encoded[i] == '%') 
            {
                if (i + 2 < encoded.size()) 
                {
                    int hex1 = std::tolower(encoded[i + 1]);
                    int hex2 = std::tolower(encoded[i + 2]);
                    int value = (hex1 >= 'a' ? hex1 - 'a' + 10 : hex1 - '0') * 16 +
                                (hex2 >= 'a' ? hex2 - 'a' + 10 : hex2 - '0');
                    decoded << static_cast<char>(value);
                    i += 2;
                }
            } 
            else if (encoded[i] == '+') 
            {
                decoded << ' ';
            } 
            else 
            {
                decoded << encoded[i];
            }
        }
        return decoded.str();
    }
}