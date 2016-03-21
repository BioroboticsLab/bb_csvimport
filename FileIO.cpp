#include "FileIO.h"

namespace io
{

    std::vector<std::string> readFileIntoLines(const std::string& filename)
    {
	std::vector<std::string> lines;

	std::ifstream fileStream;
	std::string line;
	fileStream.open(filename, std::ifstream::out);
	if (fileStream.is_open())
	{
	    while (getline(fileStream, line))
	    {
		lines.push_back(line);
	    }
	    fileStream.close();
	}
	return lines;
    }

    std::vector<std::string> getCSVFilesList(const std::string& directory, int p)
    {
	//256 files in tar archive.
	int limit = (int) (256 * Config::tarFilesProArchive / Config::parserProcessCount);
	DIR *dir;
	struct dirent *ent;
	std::vector<std::string> fl;
	if ((dir = opendir(directory.c_str())) != NULL)
	{
	    int c = 0;
	    /* print all the files and directories within directory */
	    while ((ent = readdir(dir)) != NULL)
	    {
		if (c >= limit)
		{
		    return fl;
		}
		std::string name = std::string(ent->d_name);
		//check that file extension equal csv
		if (name.substr(name.find_last_of(".") + 1) == "csv")
		{
		    //get seconds from file name
		    const std::vector<std::string> nameTokens = lib::split(name, '_');
		    std::string str_seconds = nameTokens[2].substr(nameTokens[2].length() - 2);
		    int int_seconds = std::stoi(str_seconds);

		    //check if parser p schould proceed this csv file
		    if ((int_seconds % Config::parserProcessCount) != p)
		    {
			continue;
		    }

		    c++;
		    fl.push_back(name);
		}
	    }
	    closedir(dir);
	}
	else
	{
	    printf("could not open directory %s\n", directory.c_str());
	    perror("");
	}
	return fl;
    }

    void moveFilesLocal(const std::string& path, std::string& res)
    {
	std::string str = "_1.sql";
	char buf[1024];
	while (access(res.c_str(), F_OK) != -1)
	{
	    res.replace(res.end() - 4, res.end(), str);
	}
	if (rename(path.c_str(), res.c_str()) != 0)
	{
	    sprintf(buf, "Error moving file sql file %s to error directory\n", path.c_str());
	    lib::log(buf);
	    perror("Error moving file to error directory");
	}
    }

    void removeFilesLocal(const std::vector<std::string>& fl, const std::string& path)
    {
	for (std::string file : fl)
	{
	    if (remove((path + file).c_str()) != 0)
	    {
		printf("File %s ", (path + file).c_str());
		perror("file error");
	    }
	}
    }

    int getFilesCount(const std::string& d)
    {
	DIR *dir;
	struct dirent *ent;
	int c = 0;
	if ((dir = opendir(d.c_str())) != NULL)
	{
	    while ((ent = readdir(dir)) != NULL)
	    {
		c++;
	    }
	}
	closedir(dir);
	return c - 2;
    }

    bool file_exists(const std::string& name)
    {
	struct stat buffer;
	return ( stat(name.c_str(), &buffer) == 0);
    }
}
