#ifndef FILEIO_H
#define FILEIO_H

#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include "lib.h"
#include "config.h"

namespace io {

    /* returns all lines of file */
    std::vector<std::string> readFileIntoLines(const std::string& filename);
    std::vector<std::string> getCSVFilesList(const std::string& directory, int p);
    void moveFilesLocal(const std::string& path, std::string& res);
    void removeFilesLocal(const std::vector<std::string>& fl, const std::string& path);
    int getFilesCount(const std::string& d);
    bool file_exists(const std::string& name);
}

#endif // FILEIO_H
