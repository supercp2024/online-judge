#pragma once
#include <unordered_map>
#include <string_view>
#include <utility>
#include <vector>
#include <string>
#include <memory>

#include "setting.hpp"
#include "log.hpp"

namespace ns_examiner
{
    class http_base
    {
    protected:
        struct completeness_control
        {
            completeness_control();
            void reset();
            std::string _buf;
            bool _is_single_header_completed;
            std::string_view _key;
            unsigned long long _final_content_size = 0;
            unsigned long long _cur_content_size = 0;
            unsigned int _progress;
        };
    public:
        // 将生存期太短的字符串放入内部存储
        void headerStorge(std::string_view key, bool storge_key, std::string_view value, bool storge_value);
    protected:
        http_base();
        virtual ~http_base();
        bool parseTitle(std::string::iterator& begin, const std::string::iterator& end, std::string& parsed_str);
        bool parseHeader(std::string::iterator& begin, const std::string::iterator& end);
        bool parseContent(std::string::iterator& begin, const std::string::iterator& end);
        void serialize(std::vector<std::string_view>& ret, std::array<std::string_view, 3> input) const;
        void serialize_sp(std::string& ret, std::array<std::string_view, 3> input) const;
        void clear();
        unsigned long long toInteger(const std::string_view& number_str);
    public:
        std::unordered_map<std::string_view, std::string_view> _header_table;
        std::string _content;
    public:
        const static std::unordered_map<std::string_view, std::string_view> _msg_type;
    protected:
        std::vector<std::shared_ptr<std::string>> _header_array;
        completeness_control _ctrl;
    protected:
        constexpr static const char* _blank = "\r\n";
        constexpr static const char* _header_blank = ": ";
        constexpr static const char* _space = " ";
    };

    class http_request : public http_base
    {
    public:
        http_request();
        bool deserialize(std::string::iterator begin, std::string::iterator end);
        // 空间友好型序列化
        std::vector<std::string_view> serialize() const;
        // 普通序列化
        std::string serialize_sp() const;
        void clear();
    public:
        std::string _method;
        std::string _url;
        std::string _version;
    };

    class http_response : public http_base
    {
    public:
        http_response();
        bool deserialize(std::string::iterator begin, std::string::iterator end);
        // 空间友好型序列化
        std::vector<std::string_view> serialize() const;
        // 普通序列化
        std::string serialize_sp() const;
        void clear();
    public:
        std::string _version;
        std::string _code;
        std::string _status;
    };

    class url
    {
    public:
        url(const std::string& input = "/");
        url& operator=(const std::string& input);
    private:
        void init(const std::string& input);
        static std::string url_decode(const std::string& encoded);
        static std::string url_decode(const std::string_view& encoded);
    public:
        std::filesystem::path _resourse_path;
        std::string _extension;
        std::unordered_map<std::string, std::string> _parameters;
    };
}