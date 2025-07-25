#pragma once
#include <functional>
#include <unordered_map>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include "/home/jjim/cpp_server/nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;
class RequestHandler {
public:
    RequestHandler();
    void process(int client_fd, int python_fd, const std::string& jsonStr, std::string client_ip);

private:
    using HandlerFunc = std::function<void(int, int, const nlohmann::json&)>;
    std::unordered_map<int, HandlerFunc> handlers;

    void handleProtocol1(int client_fd, int python_fd, const nlohmann::json& j);
    void handleProtocol2(int client_fd, int python_fd, const nlohmann::json& j);
    void handleProtocol3(int client_fd, int python_fd, const nlohmann::json& j);
    void handleProtocol4(int client_fd, int python_fd, const nlohmann::json& j);
    void handleProtocol5(int client_fd, int python_fd, const nlohmann::json& j);
    void handleProtocol6(int client_fd, int python_fd, const nlohmann::json& j);
    void handleProtocol7(int client_fd, int python_fd, const nlohmann::json& j);
    void handleProtocol8(int client_fd, int python_fd, const nlohmann::json& j);
    void handleProtocol9(int client_fd, int python_fd, const nlohmann::json& j);
    void send_json(int client_fd, const json& jsonstr, const string& jsonstr_2);

    std::string NAME;
    std::string ip;
};