#ifndef OPTTOURREADER_H
#define OPTTOURREADER_H

#include <string>
#include <vector>

class OptTourReader {
public:
    static std::vector<int> loadTour(const std::string& path);
};

#endif