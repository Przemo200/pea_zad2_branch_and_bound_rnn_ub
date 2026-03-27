#ifndef FILEREADER_H
#define FILEREADER_H

#include <string>
#include "TSPInstance.h"

class FileReader {
public:
    static TSPInstance loadInstance(const std::string& path);
};

#endif