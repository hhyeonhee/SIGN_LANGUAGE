#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>

class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();

    bool start();
    void stop();

private:
    int server_fd_;
    int python_fd_;
    int port_;
    bool running_;
    std::string client_ip_;

    std::thread pythonReceiverThread_;
    std::vector<std::thread> clientThreads_;
    std::mutex threadMutex_;

    bool createSocket();
    bool bindSocket();
    bool listenSocket();

    void acceptClients();
    void handleClient(int client_fd);
    void cleanup();

    bool connectToPythonServer(const std::string& ip, int port);
    void pythonReceiveThread();
    void handlePythonProtocol7(const std::string& jsonStr);

};
