#include "database.hpp"

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

void sql_client::setInput(const std::string& name, std::string str)
{
    const static std::string pattern = "INSERT INTO input(name, inputs) VALUES(?, ?)";
    sql::PreparedStatement* pstmt = _con_ptr->prepareStatement(pattern);
    pstmt->setString(1, name);
    pstmt->setString(2, str);
    pstmt->executeUpdate();
    delete pstmt;
}

void sql_client::setOutput(const std::string& name, std::string str)
{
    const static std::string pattern = "INSERT INTO output(name, outputs) VALUES(?, ?)";
    sql::PreparedStatement* pstmt = _con_ptr->prepareStatement(pattern);
    pstmt->setString(1, name);
    pstmt->setString(2, str);
    pstmt->executeUpdate();
    delete pstmt;
}

void sql_client::setCode(const std::string& name, std::filesystem::path path)
{
    std::ifstream fin(path, std::ios::in);
    std::string content;
    content.resize(std::filesystem::file_size(path));
    fin.read(content.data(), content.size());
    fin.close();
    const static std::string pattern = "INSERT INTO main_code(name, code) VALUES(?, ?)";
    sql::PreparedStatement* pstmt = _con_ptr->prepareStatement(pattern);
    pstmt->setString(1, name);
    pstmt->setString(2, content);
    pstmt->executeUpdate();
    delete pstmt;
}

void sql_client::setLimit(const std::string& name, std::unordered_map<unsigned int, unsigned int>&& limits)
{
    const static std::string pattern = "INSERT INTO rlimit(name, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)"
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sql::PreparedStatement* pstmt = _con_ptr->prepareStatement(pattern);
    pstmt->setString(1, name);
    for (unsigned int i = 0; i < 17; ++i)
    {
        if (limits.find(i) == limits.end())
        {
            pstmt->setBigInt(i, "-1");
        }
        else
        {
            pstmt->setBigInt(i, std::to_string(limits[i]));
        }
    }
    pstmt->executeUpdate();
    delete pstmt;
}

void sql_client::setInfo(const std::string& name, const std::string& difficulty, const std::string& tags, std::string str)
{
    const static std::string pattern = "INSERT INTO info(name, difficulty, tags, info) VALUES(?, ?, ?, ?)";
    sql::PreparedStatement* pstmt = _con_ptr->prepareStatement(pattern);
    Json::FastWriter writer;
    pstmt->setString(1, name);
    pstmt->setString(2, difficulty);
    pstmt->setString(3, tags);
    pstmt->setString(4, str);
    pstmt->executeUpdate();
    delete pstmt;
}