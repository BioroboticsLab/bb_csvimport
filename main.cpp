
#include "DecoderData.h"
#include "CrayAccess.h"
#include "FileIO.h"
#include "DBAccess.h"
#include "config.h"
#include "lib.h"

#include <map>
#include<stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <dirent.h>
#include <csignal>
#include <signal.h>
#include <cstring>


const std::string DATAPATH = "csv/";
const std::string WORKING_DIR = "sql/";
const std::string ERROR_DIR = "error/";
const std::string ARCHIVE = "archive/";
const useconds_t ONE_SECOND = 1000000;
const unsigned int MAX_TAGS_PRO_IMAGE = 400;
bool interrupted;

sig_atomic_t child_exit_status;
std::string procName;

#define CHECK(x) if(!(x)) { perror(#x " failed"); exit(1);}
typedef std::string(*GetTable) (std::string);

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

void clean_up_child_process(int signal_num)
{
    /* reomove child process */
    int s;
    wait(&s);
    /* save exit status in a global variable */
    child_exit_status = s;
}

void signalHandler(int signum)
{
    char buf[1024];
    sprintf(buf, "%s signalHandler caught signal %d\n", procName.c_str(), signum);
    lib::log(buf);
    interrupted = true;
}

void acquire(const std::string& name)
{
    int sem = 0;
    do
    {
	sem = mkdir((WORKING_DIR + "d" + name).c_str(), 0777);
	if (sem != 0)
	{
	    usleep(ONE_SECOND);
	}
    }
    while (sem != 0);
}

void release(const std::string& name)
{
    rmdir((WORKING_DIR + "d" + name).c_str());
}

//FOR DEBUG

void outputDecoderData(std::map<int, DecoderData>& data)
{
    int c = 0;
    for (std::map< int, DecoderData>::const_iterator iter = data.begin();
	    iter != data.end(); ++iter)
    {
	if (c > 30)
	{
	    break;
	}
	printf("id: %lld, timestamp: %s\n", iter->second.id, iter->second.timestamp.c_str());
	for (std::map<int, DecoderCandidate>::const_iterator cand = iter->second.candidate.begin();
		cand != iter->second.candidate.end(); ++cand)
	{
	    printf("      tag:%d\n", cand->second.tag);
	}
	c++;
    }
}

void parseCSVLineToDecoderData(int& file_num, const std::string& line, const std::vector<std::string>& fileNameTokens,
			       std::map<int, DecoderData>& data)
{
    const std::vector<std::string> tokens = lib::split(line, ',');
    if (tokens.size() < 10)
    {
	int size = tokens.size();
	char buf[1024];
	sprintf(buf, "Bad file. line: %s\n tokens.size: %d %s %s %s %s\n", line.c_str(), size, fileNameTokens[0].c_str(),
		fileNameTokens[1].c_str(), fileNameTokens[2].c_str(), fileNameTokens[3].c_str());
	lib::log(buf);
	return;
    }

    /**
     * writes a csv file with the following format:
     *         tagIdx       : unique sequential id of the tag
     *         candidateIdx : sequential id of the candidate per tag
     *         gridIdx      : sequential id of the grid/decoding per candidate
     *         xpos         : x coordinate of the grid center
     *         ypos         : y coordinate of the grid center
     *         xRotation    : rotation of the grid in x plane
     *         yRotation    : rotation of the grid in y plane
     *         zRotation    : rotation of the grid in z plane
     *         vote         : candidate score
     *         id           : decoded id
     *
     * Cam_2_20140805100033_2
     *
     */
    int x = std::stoi(tokens[3]);
    int y = std::stoi(tokens[4]);

    //skip detections with negative coordinates
    if (x < 0 || y < 0)
    {
	return;
    }
    unsigned int i, j;
    short tag = stoi(tokens[0]);
    unsigned int index = tag + (file_num * MAX_TAGS_PRO_IMAGE);
    bool calc_id;
    calc_id = false;
    if (data.find(index) == data.end())
    {
	calc_id = true;
    }
    i = index;
    data[i].timestamp = fileNameTokens[2];
    std::string offset = fileNameTokens[3].substr(0, fileNameTokens[3].length() - 4);

    if (offset.length() <= 3)
    {
	data[i].offset = offset;
    }
    else
    {
	data[i].offset = offset.substr(0, 3);
    }

    tag = (short) std::stoi(tokens[0]);
    short cand = (short) std::stoi(tokens[1]);
    short grid = (short) std::stoi(tokens[2]);
    if (calc_id)
    {
	long long centisecs, coord;
	centisecs = lib::dateToCentisecsFromExperimentBegin(data[i].timestamp, data[i].offset);
	data[i].x = x;
	data[i].y = y;
	data[i].camID = std::stoi(fileNameTokens[1]);

	coord = (data[i].y * 4000 + data[i].x);

	while (tag > 255)
	{
	    tag = tag % 255;
	}
	data[i].id = (centisecs << 34) | (coord << 10) | (data[i].camID << 8) | tag;
    }
    tag = (short) std::stoi(tokens[0]);
    j = data[i].candidate.size();
    data[i].candidate[j].tag = (tag << 4) | (cand << 2) | grid;
    data[i].candidate[j].x = std::stoi(tokens[3]);
    data[i].candidate[j].y = std::stoi(tokens[4]);
    data[i].candidate[j].xRotation = (float) std::stod(tokens[5]);
    data[i].candidate[j].yRotation = (float) std::stod(tokens[6]);
    data[i].candidate[j].zRotation = (float) std::stod(tokens[7]);
    data[i].candidate[j].score = std::stoi(tokens[8]);
    data[i].candidate[j].bee = strtoul(tokens[9].c_str(), NULL, 2);
}

std::map<int, DecoderData> fileContentToDecoderData(std::vector<std::string>& fileNames)
{
    std::map<int, DecoderData> data;
    int file_num = 1;
    for (std::string fileName : fileNames)
    {
	const std::vector<std::string> fileNameTokens = lib::split(fileName, '_');
	const std::vector<std::string> lines = io::readFileIntoLines(DATAPATH + fileName);
	for (std::string line : lines)
	{
	    if (line.length() > 0)
	    {
		parseCSVLineToDecoderData(file_num, line, fileNameTokens, data);
	    }
	}
	file_num++;
    }
    return data;
}

std::map<std::string, std::map<int, DecoderData>> splitVectorOnDateFormat(const std::map<int, DecoderData>& decoderData, GetTable getTable)
{
    std::map<std::string, std::map<int, DecoderData>> splitData;

    for (std::map< int, DecoderData>::const_iterator iter = decoderData.begin();
	    iter != decoderData.end(); ++iter)
    {
	const std::string table = getTable(iter->second.timestamp);
	splitData[table][iter->first] = iter->second;
    }

    return splitData;
}

std::string getTableNameFromTimestamp(std::string timestamp)
{
    return "t" + timestamp;
}

std::string splitOnOneMinute(std::string timestamp)
{
    return getTableNameFromTimestamp(timestamp.substr(0, 12));
}

std::string splitOnYear(std::string timestamp)
{
    return getTableNameFromTimestamp(timestamp.substr(0, 4));
}

//generates sql statment for the given list of csv files

void csv2sql(std::vector<std::string>& fileNames, bool create_table)
{
    std::map<int, DecoderData> data;
    std::string file_path;
    std::string file_name;
    std::string table_name;
    data = fileContentToDecoderData(fileNames);
    std::map<std::string, std::map<int, DecoderData>> map = splitVectorOnDateFormat(data, splitOnOneMinute);
    data.clear();
    for (auto iterator : map)
    {
	file_name = iterator.first;
	table_name = iterator.first;
	if (Config::oneTable)
	{
	    table_name = file_name.substr(0, 5);
	}
	acquire(file_name + ".sql");
	FILE * pFile;
	file_path = WORKING_DIR + file_name + ".sql";
	if (Config::oneTable == 0)
	{
	    create_table = false;
	}
	if (Config::oneTable == 0 && !io::file_exists(file_path))
	{
	    create_table = true;
	}
	pFile = fopen(file_path.c_str(), "a");

	if (create_table)
	{
	    if (pFile != NULL)
	    {
		std::string s;
		s = db::createDecoderDataTableQuery(table_name).c_str();
		fputs(s.c_str(), pFile);
	    }
	}

	if (pFile != NULL)
	{
	    std::string s;
	    s = db::buildInsertIntoSQL(table_name, iterator.second).c_str();
	    fputs(s.c_str(), pFile);
	}
	if (pFile != NULL)
	{
	    std::string s;
	    s = db::buildInsertBeesIDIntoSQL(table_name, iterator.second).c_str();
	    fputs(s.c_str(), pFile);
	}
	fclose(pFile);
	release(file_name + ".sql");
    }
    map.clear();
}

void startArchiveProcess()
{
    procName = "Archive";
    lib::log("Archive Process is started");
    setpriority(PRIO_PROCESS, 0, 20);
    char *path = NULL;
    path = getcwd(NULL, 0);
    char *args[] = {"unzip.sh", path, (char *) 0};
    execv(args[0], args);
    lib::log("Archive Process is finished\n");
}

void startParserProcess(int p)
{
    procName = "Parser " + std::to_string(p);
    useconds_t period = ONE_SECOND;
    bool create_table = true;
    char buf[64];
    sprintf(buf, "Starting Parser Process %i\n", p);
    lib::log(buf);
    while (!interrupted)
    {
	std::vector<std::string> fl;
	//get list of files for this parser process
	fl = io::getCSVFilesList(DATAPATH, p);
	if (fl.size() == 0)
	{
	    usleep(period);
	    continue;
	}
	csv2sql(fl, create_table);
	if(create_table)
	{
	    create_table = false;
	}
	io::removeFilesLocal(fl, DATAPATH);
    }
    sprintf(buf, "Parser %d Process is finished\n", p);
    lib::log(buf);
}

void startDBInsertProcess()
{
    procName = "DB";
    DIR *dir;
    struct dirent *ent;
    struct stat filestat;
    PGresult *res;
    std::string query, filepath, name, line, dest;
    std::ifstream fileStream;
    bool error;
    bool first_time = true;
    char buf[2048];
    while (!interrupted)
    {
	PGconn *conn = db::connectDB();
	if (PQstatus(conn) != CONNECTION_OK)
	{
	    sprintf(buf, "Postgresql server connection error: %s\n", PQerrorMessage(conn));
	    lib::log(buf);
	    db::closeConn(conn);
	    usleep(ONE_SECOND * 30);
	    continue;
	}
	if (first_time)
	{
	    first_time = false;
	    PQexec(conn, "SET client_min_messages TO WARNING");
	}
	if ((dir = opendir(WORKING_DIR.c_str())) != NULL)
	{
	    /* print all the files and directories within directory */
	    while (!interrupted && (ent = readdir(dir)) != NULL)
	    {
		name = std::string(ent->d_name);
		filepath = WORKING_DIR + name;
		// If the file is a directory (or is in some way invalid) we'll skip it
		if (stat(filepath.c_str(), &filestat))
		{
		    continue;
		}
		if (S_ISDIR(filestat.st_mode))
		{
		    continue;
		}

		//get lock on a file
		acquire(name);
		fileStream.open(filepath, std::ifstream::out);
		error = false;
		if (fileStream.is_open())
		{
		    while (getline(fileStream, line))
		    {
			res = PQexec(conn, line.c_str());
			if (PQresultStatus(res) != PGRES_COMMAND_OK)
			{
			    error = true;
			    sprintf(buf, "Postgresql execute query error: %s\nMoving sql file %s to error directory\n", PQerrorMessage(conn), name.c_str());
			    lib::log(buf);
			    fileStream.close();
			    dest = ERROR_DIR + name;
			    io::moveFilesLocal(filepath, dest);
			    PQclear(res);
			    break;
			}
			PQclear(res);
		    }
		    fileStream.close();
		    if (!error && remove(filepath.c_str()) != 0)
		    {
			printf("Cannot delete %s\n", filepath.c_str());
			perror("Error deleting sql file");
		    }
		}

		//release lock
		release(name);
	    }
	    closedir(dir);
	}
	else
	{
	    /* could not open directory */
	    sprintf(buf, "could not open directory %s\n", WORKING_DIR.c_str());
	    lib::log(buf);
	    perror("");
	}
	db::closeConn(conn);
	usleep(ONE_SECOND * 1);
    }
    lib::log("DBInsertProcess is finished");
}

void handle_sigchld(int sig)
{
    int saved_errno = errno;
    while (waitpid((pid_t) (-1), 0, WNOHANG) > 0)
    {
    }
    errno = saved_errno;
}

int main()
{
    pid_t pid;
    char buf[1024];
    interrupted = false;
    int num = 0, c = 0, status = 0, status_child = 0, observerFilesCount;
    bool filesFound = false;
    std::vector<std::string> observerList;
    /* Block SIGINT. */
    sigset_t mask, omask;

    // register signal SIGINT and signal handler
    signal(SIGINT, signalHandler);
    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1)
    {
	perror(0);
	exit(1);
    }

    //init directories
    mkdir(DATAPATH.c_str(), 0777);
    mkdir(WORKING_DIR.c_str(), 0777);
    mkdir(ERROR_DIR.c_str(), 0777);
    mkdir(ARCHIVE.c_str(), 0777);


    Config::load("config.txt");

    pid_t archive_pid = fork();
    CHECK(archive_pid >= 0);
    if (archive_pid == 0)
    {
	CHECK(setpgid(0, 0) == 0);
	startArchiveProcess();
	exit(0);

    }
    if (setpgid(archive_pid, archive_pid) < 0 && errno != EACCES)
	exit(1);

    /* Unblock SIGINT */
    CHECK(sigprocmask(SIG_SETMASK, &omask, NULL) == 0);


    pid = fork();
    if (pid == 0)
    {
	procName = "DB";
	startDBInsertProcess();
	exit(0);
    }

    pid_t* pID = new pid_t[Config::parserProcessCount];
    for (int indexOfProcess = 0; indexOfProcess < Config::parserProcessCount; indexOfProcess++)
    {
	pID[indexOfProcess] = fork();
	if (pID[indexOfProcess] == 0)
	{
	    startParserProcess(indexOfProcess);
	    lib::log("Parser Process is finished");
	    exit(0);
	}
    }

    //moves completed archives on cray for futher investigation
    remote::moveCompletedArchives();

    procName = "Main";
    while (!interrupted)
    {
	Config::load("config.txt");
	//get results subdirectories
	observerList = remote::getObserversDirectories();
	for (auto observer : observerList)
	{
	    if (interrupted) break;
	    observerFilesCount = 1;
	    while (!interrupted && observerFilesCount > 0)
	    {
		observerFilesCount = remote::getFilesCount(observer);
		//check if error occurred
		if (observerFilesCount == -1)
		{
		    //log it and skip it
		    sprintf(buf, "remote::getFilesCount %s ERROR", observer.c_str());
		    lib::log(buf);
		    continue;
		}
		else if (observerFilesCount > 0)
		{
		    filesFound = true;
		    while (!interrupted && ((c = io::getFilesCount(ARCHIVE)) >= Config::maxLocalArchives
		    || io::getFilesCount(WORKING_DIR) > Config::maxLocalCsvFiles))
		    {
			usleep(ONE_SECOND * 3);
		    }
		    if (interrupted) break;
		    ++num;
		    pid_t create_pid = fork();
		    CHECK(create_pid >= 0);
		    if (create_pid == 0)
		    {
			//change child process group to disable parent interrupts
			CHECK(setpgid(0, 0) == 0);
			status_child = remote::createArchive(observer, num);
			exit(status_child);

		    }
		    if (setpgid(create_pid, create_pid) < 0 && errno != EACCES)
		    {
			sprintf(buf, "create_pid != EACCES");
			lib::log(buf);
		    }
		    /* Unblock SIGINT */
		    CHECK(sigprocmask(SIG_SETMASK, &omask, NULL) == 0);
		    waitpid(create_pid, &status, 0);
		    if (status != 0)
		    {
			sprintf(buf, "createArchive status: %d\n", status);
			lib::log(buf);
			usleep(ONE_SECOND * 30);
			continue;
		    }
		    /* Spawn child. */
		    pid_t download_pid = fork();
		    CHECK(download_pid >= 0);
		    if (download_pid == 0)
		    {
			CHECK(setpgid(0, 0) == 0);
			status_child = remote::downloadArchive(num, ARCHIVE);
			exit(status_child);
		    }
		    /* Parent */
		    if (setpgid(download_pid, download_pid) < 0 && errno != EACCES)
		    {
			sprintf(buf, "download_pid != EACCES");
			lib::log(buf);
		    }
		    /* Unblock SIGINT */
		    CHECK(sigprocmask(SIG_SETMASK, &omask, NULL) == 0);
		    status = 0;
		    waitpid(download_pid, &status, 0);
		    Config::load("config.txt");
		}
	    }
	}
	if (!interrupted && !filesFound)
	{
	    sprintf(buf, "No files found. Sleep.");
	    lib::log(buf);
	    usleep(ONE_SECOND * 60 * 10);
	}
	filesFound = false;
    }

    //Signal unzip.sh to exit
    FILE * pFile;
    pFile = fopen((ARCHIVE + "bb_exit_unzip").c_str(), "w+");
    fclose(pFile);
    lib::log("Signal unzip.sh to exit");
}
