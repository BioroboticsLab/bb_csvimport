#ifndef DBACCESS_H
#define DBACCESS_H

#include <vector>
#include <string>
#include "./include/libpq-fe.h"
#include "DecoderData.h"
#include "lib.h"

namespace db {
    PGconn* connectDB();
    void closeConn(PGconn* dbConnection);
    std::string createDecoderDataTableQuery(const std::string& tableName);
    std::string buildInsertBeesIDIntoSQL(const std::string &tableName, const std::map<int, DecoderData>& data);
    std::string buildInsertIntoSQL(const std::string &tableName, const std::map<int, DecoderData>& data);
    std::string replace(std::string s, const std::string& from, const std::string& to);
}

#endif // DBACCESS_H
