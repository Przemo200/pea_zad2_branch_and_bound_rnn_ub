#ifndef BRANCHANDBOUNDSOLVER_H
#define BRANCHANDBOUNDSOLVER_H

#include <cstddef>
#include <string>
#include <vector>
#include "TSPInstance.h"

enum class SearchStrategy {
    BFS,
    DFS,
    LOWEST_COST
};

struct InitialUpperBound {
    bool available = false;
    int cost = -1;
    std::vector<int> tour;
    double timeMs = 0.0;
};

struct BranchAndBoundResult {
    std::vector<int> bestTour;
    int bestCost = -1;
    bool solved = false;
    bool hitTimeLimit = false;
    bool hitMemoryLimit = false;
    double timeMs = 0.0;
    long long nodesGenerated = 0;
    long long nodesExpanded = 0;
    long long nodesPruned = 0;
    std::size_t frontierPeak = 0;
    int maxDepth = 0;
    long peakRssKb = -1;
    InitialUpperBound initialUpperBound;
};

class BranchAndBoundSolver {
public:
    static BranchAndBoundResult solve(const TSPInstance& instance,
                                      SearchStrategy strategy,
                                      const InitialUpperBound& initialUpperBound,
                                      int timeLimitMs,
                                      int memoryLimitMb = 0);

    static SearchStrategy parseStrategy(const std::string& text);
    static std::string strategyToString(SearchStrategy strategy);
};

#endif