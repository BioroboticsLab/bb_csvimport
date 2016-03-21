#ifndef LIB_H
#define LIB_H

#include <vector>
#include <string>
#include <time.h>
#include "config.h"

namespace lib {
    /* splits a string line on a given delimiter */
    std::vector<std::string> split(const std::string& line, const char delim);
    /* executes a command and returns its output */
    std::string exec(const std::string& command);
    void copy(const std::string& source, const std::string& target);
    void deleteFiles(const std::string& files, const std::string& path);
    unsigned long long dateToCentisecsFromExperimentBegin(const std::string& date, std::string offset);
    int bin2dec(const std::string& s_bin);
    void log(char msg[]);
}

#endif // LIB_H
