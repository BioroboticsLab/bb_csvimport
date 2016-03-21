#define __STDC_FORMAT_MACROS
#include "lib.h"
#include <sstream>
#include <inttypes.h>

namespace lib
{

    std::vector<std::string> split(const std::string& line, const char delim)
    {
	std::vector<std::string> tokens;
	std::string token;
	std::stringstream ss(line);

	while (getline(ss, token, delim))
	{
	    tokens.push_back(token);
	}

	return tokens;
    }

    std::string exec(const std::string& command)
    {
	char buf[1024];
	FILE* pipe = popen(command.c_str(), "r");
	if (!pipe)
	{
	    perror("PIPE ERROR");
	    sprintf(buf, "popen: %s", command.c_str());
	    lib::log(buf);
	    pclose(pipe);
	    return "ERROR";
	}
	std::string result = "";
	while (!feof(pipe))
	{
	    if (fgets(buf, 1024, pipe) != NULL)
	    {
		result += buf;
	    }
	}
	if (pipe)
	{
	    int pclose_exit = pclose(pipe);
	    if (0 != pclose_exit)
	    {
		if (10 != errno)//ignore No child processes
		{
		    perror("");
		    sprintf(buf, "pclose: %d, errno: %d", pclose_exit, errno);
		    lib::log(buf);
		}
	    }
	}

	return result;
    }

    void deleteFiles(const std::string& file, const std::string& path)
    {
	remove((path + file).c_str());

    }

    unsigned long long dateToCentisecsFromExperimentBegin(const std::string& date, std::string offset)
    {
	int year, month, day, hour, min, sec;
	uint64_t centisecs;
	year = atoi(date.substr(0, 4).c_str());
	month = atoi(date.substr(4, 2).c_str());
	day = atoi(date.substr(6, 2).c_str());
	hour = atoi(date.substr(8, 2).c_str());
	min = atoi(date.substr(10, 2).c_str());
	sec = atoi(date.substr(12, 2).c_str());

	struct tm * timeinfo;
	time_t rawtime;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	timeinfo->tm_year = year - 1900;
	timeinfo->tm_mon = month - 1; //months since January - [0,11]
	timeinfo->tm_mday = day; //day of the month - [1,31]
	timeinfo->tm_hour = hour; //hours since midnight - [0,23]
	timeinfo->tm_min = min; //minutes after the hour - [0,59]
	timeinfo->tm_sec = sec; //seconds after the minute - [0,59]
	time_t res;
	res = mktime(timeinfo);

	if ((uint64_t) res < Config::experimentBeginTime)
	{
	    printf("Timestamp %s is wrong\n", date.c_str());
	    return 0;
	}

	centisecs = (uint64_t) res - Config::experimentBeginTime;

	centisecs *= 100;


	if (offset.length() > 2)
	{
	    offset = offset.substr(0, 2);
	}
	centisecs += atoi(offset.c_str());

	return centisecs;
    }

    int bin2dec(const std::string& s_bin)
    {
	long bin;
	int dec = 0, rem, base = 1;
	bin = atol(s_bin.c_str());

	while (bin > 0)
	{
	    rem = bin % 10;
	    dec = dec + rem * base;
	    base = base * 2;
	    bin = bin / 10;
	}
	return dec;
    }

    const std::string currentDateTime()
    {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof (buf), "%Y-%m-%d.%X", &tstruct);

	return buf;
    }

    void log(char msg[])
    {
	printf("[%s] %s\n", currentDateTime().c_str(), msg);
	fflush(stdout);
    }

}
