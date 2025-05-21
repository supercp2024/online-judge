#include <format>

#include <sys/resource.h>

#include "database.hpp"

void description(Json::Value& root)
{
    std::ifstream fin;
    std::string buffer;
    std::string tem;
    unsigned int count = 0;
    fin.open("./info/description.txt", std::ios::binary);
    while (std::getline(fin, tem))
    {
        if (tem != "")
        {
            buffer += tem;
        }
        else
        {
            root["description"][count] = buffer;
            buffer.clear();
            ++count;
        }
    }
    fin.close();
}

void hints(Json::Value& root)
{
    std::ifstream fin;
    std::string buffer;
    buffer.resize(std::filesystem::file_size("./info/hints.txt"));
    fin.open("./info/hints.txt", std::ios::binary);
    for (unsigned int i = 0; std::getline(fin, buffer); ++i)
    {
        root["hints"][i] = buffer;
    }
    fin.close();
}

void example(Json::Value& root, unsigned int index)
{
    std::ifstream fin;
    std::string buffer;
    buffer.resize(std::filesystem::file_size(std::format("./info/examples/{0:d}.txt", index + 1)));
    fin.open(std::format("./info/examples/{0:d}.txt", index + 1), std::ios::binary);
    fin.read(buffer.data(), buffer.size());
    fin.close();
    root["examples"][index] = buffer;
}

void solution(Json::Value& root, unsigned int index)
{
    std::ifstream fin;
    std::string buffer;
    bool trigger = false;
    buffer.resize(std::filesystem::file_size(std::format("./info/solutions/{0:d}/title.txt", index + 1)));
    fin.open(std::format("./info/solutions/{0:d}/title.txt", index + 1), std::ios::binary);
    fin.read(buffer.data(), buffer.size());
    fin.close();
    root["solutions"][index]["title"] = buffer;
    buffer.resize(std::filesystem::file_size(std::format("./info/solutions/{0:d}/description.txt", index + 1)));
    fin.open(std::format("./info/solutions/{0:d}/description.txt", index + 1), std::ios::binary);
    fin.read(buffer.data(), buffer.size());
    fin.close();
    root["solutions"][index]["description"] = buffer;
    buffer.resize(std::filesystem::file_size(std::format("./info/solutions/{0:d}/code.txt", index + 1)));
    fin.open(std::format("./info/solutions/{0:d}/code.txt", index + 1), std::ios::binary);
    fin.read(buffer.data(), buffer.size());
    fin.close();
    root["solutions"][index]["code"] = buffer;
    buffer.clear();
    fin.open(std::format("./info/solutions/{0:d}/complexity.txt", index + 1), std::ios::binary);
    while (std::getline(fin, buffer))
    {
        if (!trigger)
        {
            root["solutions"][index]["complexity"]["time"] = buffer;
            trigger = true;
        }
        else
        {
            root["solutions"][index]["complexity"]["space"] = buffer;
        }
    }
    fin.close();
}

int main()
{
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder.settings_["emitUTF8"] = true;
    writerBuilder.settings_["indentation"] = "";
    sql_client client;
    Json::Value root;
    std::string str1 = "简单";
    root["tags"][0] = "array";
    root["tags"][1] = "hash table";
    std::string str2 = Json::writeString(writerBuilder, root);
    root.clear();
    description(root);
    hints(root);
    for (unsigned int i = 0; i < 3; ++i)
    {
        example(root, i);
    }
    for (unsigned int i = 0; i < 2; ++i)
    {
        solution(root, i);
    }
    std::cout << Json::writeString(writerBuilder, root) << std::endl;
    client.setInfo("两数之和", str1, str2, Json::writeString(writerBuilder, root));
    client.setCode("两数之和", "./entry.cpp");
    root.clear();
    root["inputs"][0]["args"][0] = 6;
    root["inputs"][0]["example"] = "1 3 5 7 9 13 20";
    root["inputs"][1]["args"][0] = 10;
    root["inputs"][1]["example"] = "-1 20 34 98 34 65 -9 10 12 13 -10";
    root["inputs"][2]["args"][0] = 2;
    root["inputs"][2]["example"] = "1 -1 0";
    client.setInput("两数之和", Json::writeString(writerBuilder, root));
    root.clear();
    root["outputs"][0][0] = "3 5";
    root["outputs"][0][1] = "5 3";
    root["outputs"][1][0] = "0 6";
    root["outputs"][1][1] = "6 0";
    root["outputs"][2][0] = "0 1";
    root["outputs"][2][1] = "1 0";
    client.setOutput("两数之和", Json::writeString(writerBuilder, root));
    return 0;
}