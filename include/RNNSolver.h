#ifndef RNNSOLVER_H
#define RNNSOLVER_H

#include <vector>
#include "TSPInstance.h"

struct RNNResult {
    std::vector<int> bestTour;
    int bestCost = -1;
    double timeMs = 0.0;
    int bestStartVertex = -1;
};

class RNNSolver {
public:
    static RNNResult solveWithTies(const TSPInstance& instance);
};

#endif
