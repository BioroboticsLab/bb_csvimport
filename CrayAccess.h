#ifndef CRAYACCESS_H
#define CRAYACCESS_H

#include <vector>
#include <string>

namespace remote {
    std::vector<std::string> getObserversDirectories();
    void moveCompletedArchives();
    int createArchive(const std::string& observer, int thread);
    int getFilesCount(const std::string& observer);
    int downloadArchive(int thread, std::string dest);
}

#endif // CRAYACCESS_H
