#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>
#include "TSPInstance.h"

class Generator {
public:
    static TSPInstance generateATSP(int n, int weightMin, int weightMax, unsigned int seed);
    static TSPInstance generateTSP(int n, int weightMin, int weightMax, unsigned int seed);
};

#endif