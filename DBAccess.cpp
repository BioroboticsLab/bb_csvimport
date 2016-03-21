#define __STDC_FORMAT_MACROS
#include "DBAccess.h"
#include "config.h"
#include <fstream>
#include <string>
#include <iostream>
#include <cmath>
#include <inttypes.h>
//#include <regex>

namespace db
{

    /* constant used for the splitVector function */
    const int maxItemsPerSplit = 150000;

    /* contains splitted information for a timestamp */
    struct time
    {
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
    };

    time getTimeFromTimeStamp(const std::string& timestamp)
    {
	time t;
	t.year = atoi(timestamp.substr(0, 4).c_str());
	t.month = atoi(timestamp.substr(4, 2).c_str());
	t.day = atoi(timestamp.substr(6, 2).c_str());
	t.hour = atoi(timestamp.substr(8, 2).c_str());
	t.min = atoi(timestamp.substr(10, 2).c_str());
	t.sec = atoi(timestamp.substr(12, 2).c_str());
	return t;
    }

    /* 20110516153638 -> TIMESTAMP '2011-05-16 15:36:38' */
    std::string convertTimeStampToPostGres(const std::string& timestamp, std::string offset)
    {
	std::string pgTimeStamp = "TIMESTAMP '";

	const time t = getTimeFromTimeStamp(timestamp);

	// '2011-05-16 15:36:38.123456'
	pgTimeStamp.append(std::to_string(t.year));
	pgTimeStamp.append("-");
	pgTimeStamp.append(std::to_string(t.month));
	pgTimeStamp.append("-");
	pgTimeStamp.append(std::to_string(t.day));
	pgTimeStamp.append(" ");
	pgTimeStamp.append(std::to_string(t.hour));
	pgTimeStamp.append(":");
	pgTimeStamp.append(std::to_string(t.min));
	pgTimeStamp.append(":");
	pgTimeStamp.append(std::to_string(t.sec));
	pgTimeStamp.append(".");
	pgTimeStamp.append(offset);

	pgTimeStamp.append("'");
	return pgTimeStamp;
    }

    std::string createDecoderDataTableQuery(const std::string& tableName)
    {
	std::string createTableSQL;

	createTableSQL = "CREATE TABLE IF NOT EXISTS " + tableName;
	createTableSQL.append("(id bigint,");
	createTableSQL.append("\"timestamp\" timestamp without time zone,");
	createTableSQL.append("\"camID\" smallint,");
	createTableSQL.append("x smallint,");
	createTableSQL.append("y smallint,");
	createTableSQL.append("\"orientation\" smallint,");

	//additional fields for later usage
	createTableSQL.append("\"trackID\" smallint,");
	createTableSQL.append("\"isDancing\" boolean,");
	createTableSQL.append("\"isFollowing\" boolean,");
	createTableSQL.append("\"followedBeeID\" smallint,");
	createTableSQL.append("PRIMARY KEY (id) ");
	createTableSQL.append("); \r\n");

	createTableSQL.append("CREATE TABLE IF NOT EXISTS " + tableName + "b ");
	createTableSQL.append("(id bigint, ");
	createTableSQL.append("candidateID smallint, ");
	createTableSQL.append("x smallint,");
	createTableSQL.append("y smallint,");
	createTableSQL.append("score smallint, ");
	createTableSQL.append("xRotation real,");
	createTableSQL.append("yRotation real,");
	createTableSQL.append("zRotation real,");
	createTableSQL.append("\"beeID\" smallint,");
	createTableSQL.append("PRIMARY KEY (id, candidateID), ");
	createTableSQL.append("FOREIGN KEY (id) REFERENCES " + tableName + " (id) ON UPDATE CASCADE ON DELETE CASCADE); \r\n");

	return createTableSQL;
    }

    void closeConn(PGconn *dbConnection)
    {
	PQfinish(dbConnection);
    }

    PGconn* connectDB()
    {
	PGconn* dbConnection = NULL;

	const std::string connectionParameter = "user=" + Config::dbuser + " password=" + Config::dbpassword + " dbname=" + Config::dbdatabase + " hostaddr=" + Config::dbhost + " port=" + Config::dbport;
	dbConnection = PQconnectdb(connectionParameter.c_str());

	// Check to see that the backend connection was successfully made
	if (PQstatus(dbConnection) != CONNECTION_OK)
	{
	    fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(dbConnection));
	    closeConn(dbConnection);
	    return NULL;
	}

	return dbConnection;
    }

    std::string buildInsertIntoSQL(const std::string &tableName, const std::map<int, DecoderData>& data)
    {
	std::string sqlStatement;
	sqlStatement.append("INSERT INTO " + tableName);
	sqlStatement.append(" (id, \"timestamp\", x,y, \"camID\", orientation, ");
	sqlStatement.append("\"trackID\", \"isDancing\", \"isFollowing\", \"followedBeeID\"");
	sqlStatement.append(")");
	sqlStatement.append(" VALUES ");

	for (std::map< int, DecoderData>::const_iterator iter = data.begin();
		iter != data.end(); ++iter)
	{
	    sqlStatement.append("(");
	    sqlStatement.append(std::to_string(iter->second.id));
	    sqlStatement.append(", ");
	    const std::string pgTimestamp = convertTimeStampToPostGres(iter->second.timestamp, iter->second.offset);
	    sqlStatement.append(pgTimestamp);
	    sqlStatement.append(", ");
	    std::string point;
	    point = std::to_string(iter->second.x) + ", " + std::to_string(iter->second.y);
	    sqlStatement.append(point);
	    sqlStatement.append(", ");
	    sqlStatement.append(std::to_string(iter->second.camID));
	    sqlStatement.append(", ");
	    sqlStatement.append("0");
	    sqlStatement.append(", ");

	    //following fields are set to standard values, not kown yet
	    sqlStatement.append("-1, FALSE, FALSE, -1");

	    sqlStatement.append("),");
	}
	sqlStatement.pop_back();
	sqlStatement.append("; \r\n");

	return sqlStatement;
    }

    std::string buildInsertBeesIDIntoSQL(const std::string &tableName, const std::map<int, DecoderData>& data)
    {
	std::string sqlStatement;
	sqlStatement.append("INSERT INTO " + tableName + "b");
	sqlStatement.append("(id,candidateID,x,y,score,xRotation,yRotation,zRotation,\"beeID\") ");
	sqlStatement.append(" VALUES ");
	std::string inf;
	for (std::map< int, DecoderData>::const_iterator iter = data.begin();
		iter != data.end(); ++iter)
	{

	    for (std::map< int, DecoderCandidate>::const_iterator iter2 = iter->second.candidate.begin();
		    iter2 != iter->second.candidate.end(); ++iter2)
	    {
		sqlStatement.append("(");
		sqlStatement.append(std::to_string(iter->second.id));
		sqlStatement.append(", ");
		sqlStatement.append(std::to_string(iter2->second.tag));
		sqlStatement.append(", ");
		sqlStatement.append(std::to_string(iter2->second.x));
		sqlStatement.append(", ");
		sqlStatement.append(std::to_string(iter2->second.y));
		sqlStatement.append(", ");
		sqlStatement.append(std::to_string(iter2->second.score));
		sqlStatement.append(", ");
		inf = db::replace(std::to_string(iter2->second.xRotation), "-inf", "'-Infinity'");
		inf = db::replace(inf, "inf", "'Infinity'");
		sqlStatement.append(inf);
		sqlStatement.append(", ");
		inf = db::replace(std::to_string(iter2->second.yRotation), "-inf", "'-Infinity'");
		inf = db::replace(inf, "inf", "'Infinity'");
		sqlStatement.append(inf);
		sqlStatement.append(", ");
		inf = db::replace(std::to_string(iter2->second.zRotation), "-inf", "'-Infinity'");
		inf = db::replace(inf, "inf", "'Infinity'");
		sqlStatement.append(inf);
		sqlStatement.append(", ");
		sqlStatement.append(std::to_string(iter2->second.bee));
		sqlStatement.append("),");
	    }
	}
	sqlStatement.pop_back();
	sqlStatement.append("; \r\n");

	return sqlStatement;
    }

    std::string replace(std::string s, const std::string& from, const std::string& to)
    {
	for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
	    s.replace(pos, from.size(), to);
	return s;
    }
}
