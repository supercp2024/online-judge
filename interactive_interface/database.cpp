#include "database.hpp"

namespace ns_interface
{
    sql_client::sql_client()
        : _driver_ptr(nullptr)
        , _con_ptr(nullptr)
    {
        _driver_ptr = get_driver_instance();
        _con_ptr = _driver_ptr->connect("tcp://localhost:3306", "sp", "20040715cp");
        _con_ptr->setSchema("questions");
    }

    sql_client::~sql_client()
    {
        delete _con_ptr;
    }

    void sql_client::search(Json::Value& output, std::string&& user_code)
    {
        std::string name = output["name"].asString();
        sql::PreparedStatement* pstmt = nullptr;
        sql::ResultSet* res = nullptr;
        Json::Value tem;
        Json::Reader reader;
        pstmt = _con_ptr->prepareStatement(getCode()); 
        pstmt->setString(1, name);
        res = pstmt->executeQuery();
        res->next();
        user_code.append("\n");
        user_code.append(res->getString("code"));
        output["code"] = user_code;
        delete res;
        delete pstmt;
        pstmt = _con_ptr->prepareStatement(getLimits());
        pstmt->setString(1, name);
        res = pstmt->executeQuery();
        res->next();
        parseLimit(res, output["rlimits"]);
        delete res;
        delete pstmt;
        pstmt = _con_ptr->prepareStatement(getInputs());
        pstmt->setString(1, name);
        res = pstmt->executeQuery();
        res->next();
        reader.parse(res->getString("inputs"), tem);
        output["inputs"] = tem["inputs"];
        delete res;
        delete pstmt;
        pstmt = _con_ptr->prepareStatement(getOutputs());
        pstmt->setString(1, name);
        res = pstmt->executeQuery();
        res->next();
        reader.parse(res->getString("outputs"), tem);
        output["outputs"] = tem["outputs"];
        delete res;
        delete pstmt;
    }

    void sql_client::searchAll(Json::Value& output)
    {
        sql::Statement* stmt = _con_ptr->createStatement();
        sql::ResultSet* res = stmt->executeQuery("SELECT name, difficulty, tags FROM info");
        Json::Value tem;
        Json::Reader reader;
        for (unsigned int i = 0; res->next(); ++i)
        {
            output[i]["name"] = static_cast<std::string>(res->getString("name"));
            output[i]["difficulty"] = static_cast<std::string>(res->getString("difficulty"));
            reader.parse(res->getString("tags"), tem);
            output[i]["tags"] = tem["tags"];
        }
        delete res;
        delete stmt;
    }

    bool sql_client::searchInfo(Json::Value& output, const std::string& name)
    {   
        sql::PreparedStatement* pstmt = _con_ptr->prepareStatement(getInfo());
        pstmt->setString(1, name);
        sql::ResultSet* res = pstmt->executeQuery();
        Json::Value tem;
        Json::Reader reader;
        if (!res->next())
        {
            return false;
        }
        reader.parse(res->getString("info"), output);
        output["difficulty"] = static_cast<std::string>(res->getString("difficulty"));
        reader.parse(res->getString("tags"), tem);
        output["tags"] = tem["tags"];
        output["name"] = name;
        delete res;
        delete pstmt;
        return true;
    }

    void sql_client::parseLimit(sql::ResultSet* rlimit, Json::Value& output)
    {
        int64_t soft_limit = 0;
        for (unsigned int i = 0, j = 0; i < 17; ++i)
        {
            soft_limit = rlimit->getInt64(std::to_string(i));
            if (soft_limit != -1)
            {
                output[j][0] = i;
                output[j][1] = soft_limit;
                output[j][2] = RLIM_INFINITY;
            }
        }
    }

    const std::string& sql_client::getInputs()
    {
        const static std::string pattern = "SELECT * FROM input WHERE name = ?";
        return pattern;
    }

    const std::string& sql_client::getOutputs()
    {
        const static std::string pattern = "SELECT * FROM output WHERE name = ?";
        return pattern;
    }

    const std::string& sql_client::getCode()
    {
        const static std::string pattern = "SELECT * FROM main_code WHERE name = ?";
        return pattern;
    }

    const std::string& sql_client::getLimits()
    {
        const static std::string pattern = "SELECT * FROM rlimit WHERE name = ?";
        return pattern;
    }

    const std::string& sql_client::getInfo()
    {
        const static std::string pattern = "SELECT * FROM info WHERE name = ?";
        return pattern;
    }
}