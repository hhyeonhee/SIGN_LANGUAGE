#include "db_class.h"


using namespace std;

SuhwaDB::SuhwaDB()
    : host("10.10.20.107"), db("suhwa"), user("jinyoung"), pass("1234")
{
    tableMap = {
        {1, "Category_Concept"}, {2, "Category_Economy"}, {3, "Category_Edu"}, {4, "Category_Etc"},
        {5, "Category_Loca"}, {6, "Category_Animals"}, {7, "Category_Culture"}, {8, "Category_Social"},
        {9, "Category_Life"}, {10, "Category_Food"}, {11, "Category_Cloth"}, {12, "Category_Human"},
        {13, "Category_Nature"}, {14, "Category_Politics"}, {15, "Category_Religion"}, {16, "Category_Daily"}
    };
}

bool SuhwaDB::connect() {
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        conn.reset(driver->connect("tcp://" + host + ":3306", user, pass));
        conn->setSchema(db);
        return true;
    } catch (sql::SQLException& e) {
        cerr << "❌ DB 연결 실패: " << e.what() << endl;
        return false;
    }
}

vector<string> SuhwaDB::fetchWordNamesByCategory(int category_num) {
    vector<string> nameList;

    auto it = tableMap.find(category_num);
    if (it == tableMap.end()) {
        cerr << "❌ 유효하지 않은 category_num입니다.\n";
        return nameList; // 빈 벡터 반환
    }

    string tableName = it->second;

    try {
        string query = "SELECT kr_name FROM " + tableName;
        unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next()) {
            nameList.push_back(res->getString("kr_name"));
        }

        return nameList;
    } catch (sql::SQLException& e) {
        cerr << "SQL 오류1: " << e.what() << endl;
        return nameList;
    }
}

vector<HistoryEntry> SuhwaDB::fetchHistoryEntriesByIP(const string& ip) {
    vector<HistoryEntry> entries;

    try {
        string query = "SELECT history_num, history_date, history_content FROM search_history WHERE history_ip = ? AND status = 1";
        unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, ip);
        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next()) {
            HistoryEntry entry;
            entry.history_num  = res->getInt("history_num");
            entry.history_time = res->getString("history_date");
            entry.text         = res->getString("history_content");
            entries.push_back(entry);
        }

        currentIP = ip;
        return entries;
    } catch (sql::SQLException& e) {
        cerr << "SQL 오류2: " << e.what() << endl;
        return entries; // 빈 벡터 반환
    }
}

string SuhwaDB::getVideoPathByCategoryAndName(int category_num, const string& name) {
    auto it = tableMap.find(category_num);
    if (it == tableMap.end()) {
        cerr << "[DB] 잘못된 카테고리 번호: " << category_num << endl;
        return "";
    }

    string tableName = it->second;

    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("SELECT video_path FROM " + tableName + " WHERE kr_name = ?"));
        pstmt->setString(1, name);
        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            return res->getString("video_path");
        }
    } catch (sql::SQLException& e) {
        cerr << "[DB 오류] getVideoPathByCategoryAndName: " << e.what() << endl;
    }

    return "";
}

void SuhwaDB::deletehistory(int history_num) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("UPDATE search_history SET status = 0 WHERE history_num = ?")
        );
        pstmt->setInt(1, history_num);  // history_num은 int형이어야 함
        int rows = pstmt->executeUpdate();

        if (rows > 0) {
            std::cout << "[✔] 업데이트 성공: " << rows << "개의 행이 수정됨." << std::endl;
        } else {
            std::cout << "[!] 일치하는 데이터가 없습니다." << std::endl;
        }

    } catch (sql::SQLException& e) {
        std::cerr << "[❌] SQL 오류3: " << e.what() << std::endl;
    }
}

void SuhwaDB::deletehistoryall(const std::string& ip) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("UPDATE search_history SET status = 0 WHERE history_ip = ?")
        );
        pstmt->setString(1, ip);
        int rows = pstmt->executeUpdate();

        if (rows > 0) {
            std::cout << "[✔] " << rows << "개의 기록이 status=0 으로 변경되었습니다." << std::endl;
        } else {
            std::cout << "[!] 일치하는 IP가 없습니다: " << ip << std::endl;
        }
    } catch (sql::SQLException& e) {
        std::cerr << "[❌] SQL 오류4: " << e.what() << std::endl;
    }
}


string SuhwaDB::getHistoryContentByNum(int history_num) {
    try {
        unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("SELECT history_content FROM search_history WHERE history_num = ?"));
        pstmt->setInt(1, history_num);

        unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            return res->getString("history_content");
        } else {
            cerr << "[DB] 해당 history_num(" << history_num << ")에 대한 결과 없음." << endl;
            return "";
        }
    } catch (const sql::SQLException& e) {
        cerr << "[DB 오류] getHistoryContentByNum: " << e.what() << endl;
        return "";
    }
}
