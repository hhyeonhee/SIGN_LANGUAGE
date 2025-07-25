#include "server.h"
#include "parsing_Json.h"
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fstream>

using namespace std;

TcpServer::TcpServer(int port)
    : port_(port), server_fd_(-1), running_(false) {}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::createSocket() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        perror("socket failed");  // 추가
        return false;
    }
    return true;
}

bool TcpServer::bindSocket() {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("bind failed");  // 추가
        return false;
    }
    return true;
}


bool TcpServer::listenSocket() {
    return listen(server_fd_, 5) != -1;
}

bool TcpServer::connectToPythonServer(const string& ip, int port) {
    python_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (python_fd_ < 0) {
        cerr << "[C++] Python socket() failed\n";
        return false;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        cerr << "[C++] Invalid Python server address\n";
        return false;
    }

    if (connect(python_fd_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "[C++] Connection to Python server failed\n";
        return false;
    }

    cout << "[C++] Connected to Python server on " << ip << ":" << port << endl;
    cout << "[Python] 소켓: " << python_fd_ << endl;
    return true;
}

bool TcpServer::start() {
    if (!createSocket()) {
        cerr << "createSocket 실패" << endl;
        return false;
    }
    if (!bindSocket()) {
        cerr << "bindSocket 실패" << endl;
        return false;
    }
    if (!listenSocket()) {
        cerr << "listenSocket 실패" << endl;
        return false;
    }
    
    // 🔹 Python 서버 연결
    if (!connectToPythonServer("10.10.20.117", 12345)) {
        cerr << "Could not connect to Python server.\n";
        return false;
    }
    
    running_ = true;
    pythonReceiverThread_ = thread(&TcpServer::pythonReceiveThread, this);

    thread(&TcpServer::acceptClients, this).detach();
    cout << "Server listening on port " << port_ << endl;
    return true;
}

void TcpServer::pythonReceiveThread() {
    
    char buffer[1024];
    while (running_) {
        ssize_t len = read(python_fd_, buffer, sizeof(buffer));
        if (len > 0) {
            string response(buffer, len);
            cout << "[Python] 응답 수신: " << response << endl;
            
            auto j = nlohmann::json::parse(response);
            
            int protocol = stoi(j["PROTOCOL"].get<string>());

            if (protocol == 9) {
                handlePythonProtocol7(response);
            }
        }
        else if (len == 0) {
            cerr << "[Python] 연결이 종료되었습니다.\n";
            break;
        }
        else {
            perror("[Python] read 오류");
            break;
        }
    }

    close(python_fd_);
    python_fd_ = -1;
}

// void TcpServer::pythonReceiveThread() {
//     char buffer[1024];
//     while (running_) {
//         ssize_t len = read(python_fd_, buffer, sizeof(buffer));
//         if (len > 0) {
//             string response(buffer, len);
//             cout << "[Python] 응답 수신: " << response << endl;

//             try {
//                 auto j = nlohmann::json::parse(response);

//                 int protocol = -1;
//                 if (j.contains("PROTOCOL")) {
//                     if (j["PROTOCOL"].is_string()) {
//                         protocol = stoi(j["PROTOCOL"].get<string>());
//                     } else {
//                         cerr << "[Python] PROTOCOL 값 타입 오류" << endl;
//                         continue;
//                     }
//                 } else {
//                     cerr << "[Python] PROTOCOL 키 없음" << endl;
//                     continue;
//                 }

//                 if (protocol == 10) {
//                     handlePythonProtocol7(response);
//                 }

//                 // 다른 프로토콜 처리 추가 가능

//             } catch (const nlohmann::json::exception& e) {
//                 cerr << "[Python] JSON 파싱 오류: " << e.what() << endl;
//                 cerr << "[Python] 원본: " << response << endl;
//             } catch (const std::exception& e) {
//                 cerr << "[Python] 일반 예외 발생: " << e.what() << endl;
//             }
//         }
//         else if (len == 0) {
//             cerr << "[Python] 연결이 종료되었습니다.\n";
//             break;
//         }
//         else {
//             perror("[Python] read 오류");
//             break;
//         }
//     }

//     close(python_fd_);
//     python_fd_ = -1;
// }


// C++ 서버 수정 코드 (현희수정전 원본)
// void TcpServer::handlePythonProtocol7(const string& jsonStr) {
//     try {
//         auto j = nlohmann::json::parse(jsonStr);
        
//         // 안전한 문자열 추출
//         string client_fd_str = "";
//         string video_path = "";
        
//         // CLIENT_FD 안전 추출
//         if (j.contains("CLIENT_FD") && !j["CLIENT_FD"].is_null()) {
//             if (j["CLIENT_FD"].is_string()) {
//                 client_fd_str = j["CLIENT_FD"].get<string>();
//             } else {
//                 client_fd_str = to_string(j["CLIENT_FD"].get<int>());
//             }
//         }
        
//         // VIDEO_PATH 안전 추출
//         if (j.contains("VIDEO_PATH") && !j["VIDEO_PATH"].is_null()) {
//             if (j["VIDEO_PATH"].is_string()) {
//                 video_path = j["VIDEO_PATH"].get<string>();
//             }
//         }
        
//         cout << "[PROTOCOL 7] CLIENT_FD: " << client_fd_str << ", VIDEO_PATH: " << video_path << endl;
        
//         // 빈 비디오 경로 처리
//         if (video_path.empty()) {
//             cout << "[PROTOCOL 7] 비디오 경로가 비어있음 - 클라이언트에게 오류 응답 전송" << endl;
            
//             // 클라이언트에게 오류 응답 전송
//             if (!client_fd_str.empty()) {
//                 int client_fd = stoi(client_fd_str);
//                 nlohmann::json error_response = {
//                     {"PROTOCOL", "ERROR"},
//                     {"MESSAGE", "비디오를 찾을 수 없습니다"}
//                 };
//                 string error_msg = error_response.dump();
//                 write(client_fd, error_msg.c_str(), error_msg.size());
//             }
//             return;
//         }
        
//         // 기존 파일 처리 로직
//         int client_fd = stoi(client_fd_str);

//         // JSON 헤더 전송
//         nlohmann::json header = {
//             {"PROTOCOL", "2"},
//             {"VIDEO_PATH", video_path}
//         };
//         string header_str = header.dump();
//         write(client_fd, header_str.c_str(), header_str.size());

//         cout << "[PROTOCOL 7] 클라이언트 FD " << client_fd << " 에게 영상 전송 완료" << endl;
        
//     } catch (const nlohmann::json::exception& e) {
//         cerr << "[PROTOCOL 7] JSON 처리 오류: " << e.what() << endl;
//         cerr << "[PROTOCOL 7] 원본 JSON: " << jsonStr << endl;
//     } catch (const exception& e) {
//         cerr << "[PROTOCOL 7] 일반 오류: " << e.what() << endl;
//     }
// }

void TcpServer::handlePythonProtocol7(const std::string& jsonStr) {
    std::cout << "DEBUG JSON STRING: " << jsonStr << std::endl;
    std::cout << "JSON 파싱 전입니다" << std::endl;

    try {
        auto j = nlohmann::json::parse(jsonStr);

        // 안전하게 값 추출
        std::string video_path = j.value("VIDEO_PATH", "");
        std::string client_fd_str;

        if (j.contains("CLIENT_FD")) {
            if (j["CLIENT_FD"].is_string()) {
                client_fd_str = j.value("CLIENT_FD", "");
            } else if (j["CLIENT_FD"].is_number_integer()) {
                client_fd_str = std::to_string(j.value("CLIENT_FD", 0));
            }
        }

        // 유효성 검사
        if (client_fd_str.empty() || video_path.empty()) {
            std::cerr << "[ERROR] 비어 있는 값 발견 (CLIENT_FD 또는 VIDEO_PATH)\n";
            std::cerr << "[원본 JSON] " << jsonStr << std::endl;
            return;
        }

        // 문자열을 정수로 변환
        int client_fd = std::stoi(client_fd_str);

        // 응답 생성 및 전송
        nlohmann::json response = {
            {"PROTOCOL", "2"},
            {"VIDEO_PATH", video_path}
        };

        std::string response_str = response.dump();
        write(client_fd, response_str.c_str(), response_str.size());

        std::cout << "[PROTOCOL 7] FD: " << client_fd << " → 전송 완료\n";

    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[JSON 파싱 오류] " << e.what() << std::endl;
        std::cerr << "[원본 JSON] " << jsonStr << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[예외 발생] " << e.what() << std::endl;
    }
}



void TcpServer::stop() {
    running_ = false;
    cout << 1;
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }

    if (python_fd_ != -1) {
        close(python_fd_);
        python_fd_ = -1;
    }

    if (pythonReceiverThread_.joinable()) {
        pythonReceiverThread_.join();  // 안전하게 종료 대기
    }

    lock_guard<mutex> lock(threadMutex_);
    for (auto& t : clientThreads_) {
        if (t.joinable())
            t.join();
    }
    clientThreads_.clear();
}

void TcpServer::acceptClients() {
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t addrlen = sizeof(clientAddr);
        int client_fd = accept(server_fd_, (struct sockaddr*)&clientAddr, &addrlen);
        if (client_fd < 0) continue;
        if (client_fd >= 0) {
            // 클라이언트 IP 문자열로 변환
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), ip_str, INET_ADDRSTRLEN);

            cout << "New client connected: " << inet_ntoa(clientAddr.sin_addr) << endl;
            cout << "클라이언트 IP: " << ip_str << endl;

            std::string client_ip(ip_str);  // 변환 완료
            client_ip_ = client_ip;
        }

        lock_guard<mutex> lock(threadMutex_);
        clientThreads_.emplace_back(&TcpServer::handleClient, this, client_fd);
    }
}

void TcpServer::handleClient(int client_fd) {
    char buffer[1024];
    ssize_t bytesRead;

    RequestHandler handler;  // 요청 처리 클래스 생성

    while ((bytesRead = read(client_fd, buffer, sizeof(buffer))) > 0) {
        string message(buffer, bytesRead);
        cout << "Received: " << message << endl;
        cout << "f_d: " << client_fd << endl; 
        // JSON 파싱 및 처리
        handler.process(client_fd, python_fd_, message, client_ip_);
    }

    close(client_fd);
    cout << "Client disconnected." << endl;
}
