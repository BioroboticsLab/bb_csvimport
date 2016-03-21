#include "CrayAccess.h"
#include "lib.h"
#include "config.h"

namespace remote
{

    std::vector<std::string> getObserversDirectories()
    {
	char buf[1024];
	std::vector<std::string> list;
	const std::string sshCommand = "ssh " + Config::remote_user + "@" + Config::remote_host +
		" 'cd " + Config::remote_results_dir + " && ls -d */'";

	//sprintf(buf, "getObserversDirectories: %s", sshCommand.c_str());
	//lib::log(buf);
	std::string res = lib::exec(sshCommand);

	if (res.compare("ERROR") == 0)
	{
	    return list;
	}


	list = lib::split(res, '\n');

	return list;

    }

    void moveCompletedArchives()
    {
	char buf[1024];
	const std::string arch_done = Config::archiveNameDone + "*.tgz";
	const std::string sshCommand = "ssh " + Config::remote_user + "@" + Config::remote_host +
		" 'cd " + Config::remote_working_path + " && mkdir completed_but_not_removed; mv --backup=t " + arch_done + " completed_but_not_removed/ '";

	std::string res = lib::exec(sshCommand);
    }

    int getFilesCount(const std::string& observer)
    {
	char buf[1024];
	const std::string done_list = Config::remote_results_done_dir + observer.substr(0, observer.length() - 1) + ".done";
	const std::string sshCommand = "ssh " + Config::remote_user + "@" + Config::remote_host +
		" ' touch " + done_list + " && cd " + Config::remote_results_dir + " && find " + observer +
		" -iname \"*_csv.tar\" -type f 2> /dev/null | sort | grep -Fxvf " + done_list + " | wc -l '";

	std::string res = lib::exec(sshCommand);
	if (res.compare("ERROR") == 0)
	{
	    return -1;
	}

	int count = 0;
	try
	{
	    count = std::stoi(res);
	}
	catch (...)
	{
	    return 0;
	}
	sprintf(buf, "Files on %s at %s: %d\n", Config::remote_host.c_str(), (Config::remote_results_dir + observer).c_str(), count);
	lib::log(buf);
	return count;
    }

    int createArchive(const std::string& observer, int num)
    {
	char buf[1024];
	const std::string out = Config::remote_working_path + "out" + std::to_string(num);
	const std::string arch = Config::remote_working_path + Config::archiveName + std::to_string(num) + ".tgz";
	const std::string arch_done = Config::remote_working_path + Config::archiveNameDone + std::to_string(num) + ".tgz";
	const std::string done_list = Config::remote_results_done_dir + observer.substr(0, observer.length() - 1) + ".done";

	const std::string sshCommand = "ssh " + Config::remote_user + "@" + Config::remote_host +
		" ' cd " + Config::remote_results_dir + " && find " + observer +
		" -iname \"*_csv.tar\" -type f 2> /dev/null | sort | grep -Fxvf " + done_list + " | head -n" + std::to_string(Config::tarFilesProArchive) +
		" > " + out + " && tar -T " + out +
		" -czf " + arch + " && mv " + arch + " " + arch_done + " && cat " + out + " >> " + done_list + " && rm " + out + " && echo ok'";

	//sprintf(buf, "Create archive: %s\n", sshCommand.c_str());
	//lib::log(buf);
	std::string res = lib::exec(sshCommand);
	//sprintf(buf, "res:%s", res.c_str());
	//lib::log(buf);
	if (res.compare("ok\n") != 0)
	{
	    sprintf(buf, "Error making archive %s on %s: %s\n", arch.c_str(), Config::remote_host.c_str(), res.c_str());
	    lib::log(buf);
	    return 1;
	}
	return 0;
    }

    int downloadArchive(int thread, std::string dest)
    {
	const std::string arch = Config::archiveNameDone + std::to_string(thread) + ".tgz";
	const std::string comleted_arch = "completed_" + arch;
	const std::string scpCommand = "scp " + Config::remote_user + "@" + Config::remote_host + ":" + Config::remote_working_path + arch + " " + dest;
	char buf[1024];
	sprintf(buf, "Download: %s", arch.c_str());
	lib::log(buf);
	std::string res = lib::exec(scpCommand);

	if (res.compare("ERROR") == 0)
	{
	    sprintf(buf, "Error downloading archive %s on %s: %s", arch.c_str(), Config::remote_host.c_str(), res.c_str());
	    lib::log(buf);
	    return 1;
	}

	sprintf(buf, "Finished: %s", arch.c_str());
	lib::log(buf);

	if (rename((dest + arch).c_str(), (dest + comleted_arch).c_str()) != 0)
	{
	    sprintf(buf, "ERROR Rename: %s -> %s\n", arch.c_str(), comleted_arch.c_str());
	    lib::log(buf);
	    perror("Error rename downloaded archive");
	    return 1;
	}

	const std::string sshCommand = "ssh " + Config::remote_user + "@" + Config::remote_host +
		" ' cd " + Config::remote_working_path + " && rm " + arch + " && echo ok'";

	res = lib::exec(sshCommand);

	if (res.compare("ok\n") != 0)
	{
	    sprintf(buf, "Error deleting remote archive %s on %s: %s\n", arch.c_str(), Config::remote_host.c_str(), res.c_str());
	    lib::log(buf);
	}
	else
	{
	    sprintf(buf, "Remote archive removed: %s\n", arch.c_str());
	    lib::log(buf);
	}
	return 0;
    }
}
