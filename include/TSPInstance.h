#ifndef TSPINSTANCE_H
#define TSPINSTANCE_H

#include <string>
#include <vector>

struct TSPInstance {
    std::string name;
    std::string type;
    std::string edgeWeightType;
    int dimension = 0;
    std::vector<std::vector<int>> matrix;
};

#endif
