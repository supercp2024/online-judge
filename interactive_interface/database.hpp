#include <sys/resource.h>

#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <mysql_connection.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <jsoncpp/json/json.h>

#include "log.hpp"

/*************
 * 1. 主文件内容
 * 2. 测试用例
 * 3. 正确答案
 * 4. 资源限制
 ************/

namespace ns_interface
{
    class sql_client
    {
    public:
        sql_client();
        ~sql_client();
        // 拉取检验用信息
        void search(Json::Value& output, std::string&& user_code);
        // 拉取题库信息
        void searchAll(Json::Value& output);
        // 拉取题目信息
        bool searchInfo(Json::Value& output, const std::string& name);
    private:
        const std::string& getInputs();
        const std::string& getOutputs();
        const std::string& getCode();
        const std::string& getLimits();
        const std::string& getInfo();
        void parseLimit(sql::ResultSet* rlimit, Json::Value& output);
    private:
        sql::Driver* _driver_ptr;
        sql::Connection* _con_ptr;
    };
}