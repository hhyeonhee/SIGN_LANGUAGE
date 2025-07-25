#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cppconn/connection.h>
#include <iostream>
#include <cppconn/driver.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <mysql_driver.h>
#include <mysql_connection.h> 

using namespace std;

struct HistoryEntry {
    int history_num;
    string history_time;
    string text;
};

class SuhwaDB {
public:
    SuhwaDB();

    bool connect();
    string getVideoPathByCategoryAndName(int category_num, const string& name);
    vector<string> fetchWordNamesByCategory(int category_num);
    string getHistoryContentByNum(int history_num);
    vector<HistoryEntry> fetchHistoryEntriesByIP(const string& ip);
    void deletehistory(int history_num);
    void deletehistoryall(const string& ip);

private:
    string host, db, user, pass;
    unique_ptr<sql::Connection> conn;

    map<int, string> tableMap;
    vector<HistoryEntry> historyList;
    string currentTableName;
    string currentIP;
};