#ifndef INSTANCELISTREADER_H
#define INSTANCELISTREADER_H

#include <string>
#include <vector>

struct InstanceListEntry {
    std::string name;
    std::string instanceFile;
    std::string optTourFile;
    int optCost = -1;
};

class InstanceListReader {
public:
    static std::vector<InstanceListEntry> loadList(const std::string& path);
};

#endif
