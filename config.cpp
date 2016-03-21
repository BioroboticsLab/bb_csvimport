#include "config.h"
#include "lib.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>



namespace Config
{
    std::string dbuser;
    std::string dbpassword;
    std::string dbdatabase;
    std::string dbhost;
    std::string dbport;

    bool oneTable = 0;

    std::string remote_user;
    std::string remote_host;
    std::string remote_working_path;
    std::string remote_results_dir;
    std::string remote_results_done_dir;

    std::string archiveName;
    std::string archiveNameDone;
    int tarFilesProArchive;
    int maxLocalArchives;
    int maxLocalCsvFiles;
    int parserProcessCount;
    unsigned long int experimentBeginTime;

    void load(const std::string& file)
    {
	std::ifstream fin(file);
	std::string line;
	while (getline(fin, line))
	{
	    std::istringstream sin(line.substr(line.find("=") + 1));
	    if (line.find("dbuser") != -1 && line.find("//dbuser") == -1)
		sin >> dbuser;
	    else if (line.find("dbpassword") != -1 && line.find("//dbpassword") == -1)
		sin >> dbpassword;
	    else if (line.find("dbdatabase") != -1 && line.find("//dbdatabase") == -1)
		sin >> dbdatabase;
	    else if (line.find("dbhost") != -1 && line.find("//dbhost") == -1)
		sin >> dbhost;
	    else if (line.find("dbport") != -1 && line.find("//dbport") == -1)
		sin >> dbport;
	    else if (line.find("oneTable") != -1 && line.find("//oneTable") == -1)
		sin >> oneTable;
	    else if (line.find("remote_user") != -1 && line.find("//remote_user") == -1)
		sin >> remote_user;
	    else if (line.find("remote_host") != -1 && line.find("//remote_host") == -1)
		sin >> remote_host;
	    else if (line.find("remote_working_path") != -1 && line.find("//remote_working_path") == -1)
		sin >> remote_working_path;
	    else if (line.find("remote_results_dir") != -1 && line.find("//remote_results_dir") == -1)
		sin >> remote_results_dir;
	    else if (line.find("remote_results_done_dir") != -1 && line.find("//remote_results_done_dir") == -1)
		sin >> remote_results_done_dir;
	    else if (line.find("archiveName") != -1 && line.find("//archiveName") == -1)
		sin >> archiveName;
	    else if (line.find("archiveDoneName") != -1 && line.find("//archiveDoneName") == -1)
		sin >> archiveNameDone;
	    else if (line.find("tarFilesProArchive") != -1 && line.find("//tarFilesProArchive") == -1)
		sin >> tarFilesProArchive;
	    else if (line.find("maxLocalArchives") != -1 && line.find("//maxLocalArchives") == -1)
		sin >> maxLocalArchives;
	    else if (line.find("maxLocalCsvFiles") != -1 && line.find("//maxLocalCsvFiles") == -1)
		sin >> maxLocalCsvFiles;
	    else if (line.find("parserProcessCount") != -1 && line.find("//parserProcessCount") == -1)
		sin >> parserProcessCount;
	    else if (line.find("experimentBeginTime") != -1 && line.find("//experimentBeginTime") == -1)
		sin >> experimentBeginTime;

	}
    }
}
