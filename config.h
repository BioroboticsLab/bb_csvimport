#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace Config {
    extern std::string dbuser;
    extern std::string dbpassword;
    extern std::string dbdatabase;
    extern std::string dbhost;
    extern std::string dbport;

    extern bool oneTable;

    extern std::string remote_user;
    extern std::string remote_host;
    extern std::string remote_working_path;
    extern std::string remote_results_dir;
    extern std::string remote_results_done_dir;

    extern std::string archiveName;
    extern std::string archiveNameDone;
    extern int tarFilesProArchive;
    extern int maxLocalArchives;
    extern int maxLocalCsvFiles;
    extern int parserProcessCount;
    extern unsigned long int experimentBeginTime;

    void load(const std::string& file);
}

#endif // CONFIG_H