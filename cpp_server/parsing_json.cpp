#include "parsing_Json.h"
#include "db_class.h"

using json = nlohmann::json;
using namespace std;

RequestHandler::RequestHandler() {
    handlers[1] = [this](int fd, int py_fd, const json& j) { handleProtocol1(fd, py_fd, j); };
    handlers[2] = [this](int fd, int py_fd, const json& j) { handleProtocol2(fd, py_fd, j); };
    handlers[3] = [this](int fd, int py_fd, const json& j) { handleProtocol3(fd, py_fd, j); };
    handlers[4] = [this](int fd, int py_fd, const json& j) { handleProtocol4(fd, py_fd, j); };
    handlers[5] = [this](int fd, int py_fd, const json& j) { handleProtocol5(fd, py_fd, j); };
    handlers[6] = [this](int fd, int py_fd, const json& j) { handleProtocol6(fd, py_fd, j); };
    handlers[7] = [this](int fd, int py_fd, const json& j) { handleProtocol7(fd, py_fd, j); };
    handlers[8] = [this](int fd, int py_fd, const json& j) { handleProtocol8(fd, py_fd, j); };
    handlers[10] = [this](int fd, int py_fd, const json& j) { handleProtocol9(fd, py_fd, j); };
}

void RequestHandler::handleProtocol1(int fd, int py_fd, const json& j) {
    NAME = j["NAME"].get<string>();
}

void RequestHandler::handleProtocol2(int fd, int py_fd, const json& j) {
    string text = j["TEXT"].get<string>();

    json msg_j = {
        {"PROTOCOL", "4"},
        {"CLIENT_FD", to_string(fd)},
        {"CLIENT_IP", NAME},
        {"CLIENT_NAME", NAME},
        {"TEXT", text}
    };

    string msg = msg_j.dump();

    ssize_t sent = write(py_fd, msg.c_str(), msg.size());
    if (sent != (ssize_t)msg.size()) {
        cerr << "[Python] 텍스트 전송 실패 (write 오류)" << endl;
    } else {
        cout << "[Python] 텍스트 전송 완료: " << msg << endl;
    }
}

void RequestHandler::handleProtocol3(int fd, int py_fd, const json& j) {
    int his_num = stoi(j["HISTORY_NUM"].get<string>());
    SuhwaDB db;
    string text;
    if (db.connect()) {
        text = db.getHistoryContentByNum(his_num);
        if (!text.empty()) {
            std::cout << "해당 문장: " << text << std::endl;
        }
    }
    json msg_j = {
        {"PROTOCOL", "4"},
        {"CLIENT_FD", to_string(fd)},
        {"CLIENT_IP", NAME},
        {"CLIENT_NAME", NAME},
        {"TEXT", text}
    };

    string msg = msg_j.dump();

    ssize_t sent = write(py_fd, msg.c_str(), msg.size());
    if (sent != (ssize_t)msg.size()) {
        cerr << "[Python] 텍스트 전송 실패 (write 오류)" << endl;
    } else {
        cout << "[Python] 텍스트 전송 완료: " << msg << endl;
    }
}

void RequestHandler::handleProtocol4(int fd, int py_fd, const json& j) {
    int cat_num = stoi(j["CATEGORY_NUM"].get<string>());
    SuhwaDB db;

    vector<string> words;
    if (!db.connect()) return;
    words = db.fetchWordNamesByCategory(cat_num);

    json contentJson;
    contentJson["CONTENT"] = words;

    string contentStr = contentJson.dump();      
    int contentLength = contentStr.size();
    json headerJson;
    headerJson["PROTOCOL"] = "3";
    headerJson["LENGTH"] = contentLength;
    
    
    // 3. 전송
    send_json(fd, headerJson, contentStr);
}

void RequestHandler::handleProtocol5(int fd, int py_fd, const json& j) {
    //카테고리 번호에 해당하는 테이블에서 content에 해당하는 영상경로를 찾아서 영상을 보내주는 프로토콜이다
    int cat_num = stoi(j["CATEGORY_NUM"].get<string>());
    string content = j["CONTENT"].get<string>();

    json msg = {
        {"PROTOCOL", "5"},
        {"CLIENT_FD", to_string(fd)},
        {"CONTENT", content},
        {"CATEGORY_NUM", to_string(cat_num)}
    };

    string msgStr = msg.dump();

    ssize_t sent = write(py_fd, msgStr.c_str(), msgStr.size());

    if (sent != (ssize_t)msgStr.size()) {
        cerr << "[Python] Protocol 5 전송 실패 (write 오류)" << endl;
    } else {
        cout << "[Python] Protocol 5 전송 완료: " << msgStr << endl;
    }
}

void RequestHandler::handleProtocol6(int fd, int py_fd, const json& j) {
    json dataJson;
    dataJson["DATA"] = json::array();

    SuhwaDB db;

    if (!db.connect()) return;
    
    vector<HistoryEntry> historyList;
    
    historyList = db.fetchHistoryEntriesByIP(NAME);

    for (const auto& entry : historyList) {
        dataJson["DATA"].push_back({
            {"HISTORY_NUM", entry.history_num},
            {"TIME", entry.history_time},
            {"TEXT", entry.text}
        });
    }

    string dataStr = dataJson.dump();
    int dataLength = dataStr.size();

    // 2. 헤더 JSON 구성
    json headerJson = {
        {"PROTOCOL", "1"},
        {"LENGTH", dataLength}
    };

    send_json(fd, headerJson, dataStr);
}

void RequestHandler::handleProtocol7(int fd, int py_fd, const json& j) {
    SuhwaDB db;
    if (!db.connect()) return;
    int history_num = stoi(j["HISTORY_NUM"].get<string>());
    
    db.deletehistory(history_num);
    
}

void RequestHandler::handleProtocol8(int fd, int py_fd, const json& j) {
    SuhwaDB db;
    if (!db.connect()) return;
    db.deletehistoryall(NAME);
}
void RequestHandler::handleProtocol9(int fd, int py_fd, const json& j) {

    try {
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
    } catch (const std::exception& e) {
        std::cerr << "[예외 발생] " << e.what() << std::endl;
    }
}
void RequestHandler::process(int client_fd, int python_fd, const string& jsonStr, string client_ip) {
    try {
        json j = json::parse(jsonStr);
        int protocol = stoi(j["PROTOCOL"].get<string>());
        ip = client_ip;
        auto it = handlers.find(protocol);
        if (it != handlers.end()) it->second(client_fd, python_fd, j);  // 해당 함수 호출
    } catch (const exception& e) {
        cerr << "process() JSON parse error: " << e.what() << endl;
    }
}

void RequestHandler::send_json(int fd, const json& header, const string& body) {
    string headerStr = header.dump();

    write(fd, headerStr.c_str(), headerStr.size());
    write(fd, body.c_str(), body.size());
    cout << "header : " << headerStr<< endl;
    cout << "body: " << body << endl;
}