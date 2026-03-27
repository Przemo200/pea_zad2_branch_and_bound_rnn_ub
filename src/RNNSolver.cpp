#include "../include/RNNSolver.h"
#include "../include/TourUtils.h"
#include <chrono>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
    void dfsNearestTies(const TSPInstance& instance,
                        int current,
                        std::vector<bool>& visited,
                        std::vector<int>& currentTour,
                        int& globalBestCost,
                        std::vector<int>& globalBestTour,
                        int startVertex,
                        int& globalBestStartVertex) {
        int n = instance.dimension;

        if (static_cast<int>(currentTour.size()) == n) {
            int cost = TourUtils::calculateTourCost(instance, currentTour);
            if (cost < globalBestCost) {
                globalBestCost = cost;
                globalBestTour = currentTour;
                globalBestStartVertex = startVertex;
            }
            return;
        }

        int minCost = std::numeric_limits<int>::max();
        std::vector<int> candidates;

        for (int v = 0; v < n; ++v) {
            if (visited[v]) {
                continue;
            }

            int edgeCost = instance.matrix[current][v];
            if (edgeCost < minCost) {
                minCost = edgeCost;
                candidates.clear();
                candidates.push_back(v);
            } else if (edgeCost == minCost) {
                candidates.push_back(v);
            }
        }

        for (int next : candidates) {
            visited[next] = true;
            currentTour.push_back(next);

            dfsNearestTies(instance,
                           next,
                           visited,
                           currentTour,
                           globalBestCost,
                           globalBestTour,
                           startVertex,
                           globalBestStartVertex);

            currentTour.pop_back();
            visited[next] = false;
        }
    }
}

RNNResult RNNSolver::solveWithTies(const TSPInstance& instance) {
    if (instance.dimension <= 0) {
        throw std::runtime_error("Instancja ma niepoprawny rozmiar");
    }

    auto startTime = std::chrono::steady_clock::now();

    int bestCost = std::numeric_limits<int>::max();
    std::vector<int> bestTour;
    int bestStartVertex = -1;

    for (int startVertex = 0; startVertex < instance.dimension; ++startVertex) {
        std::vector<bool> visited(instance.dimension, false);
        std::vector<int> currentTour;

        visited[startVertex] = true;
        currentTour.push_back(startVertex);

        dfsNearestTies(instance,
                       startVertex,
                       visited,
                       currentTour,
                       bestCost,
                       bestTour,
                       startVertex,
                       bestStartVertex);
    }

    auto endTime = std::chrono::steady_clock::now();

    RNNResult result;
    result.bestTour = bestTour;
    result.bestCost = (bestCost == std::numeric_limits<int>::max()) ? -1 : bestCost;
    result.bestStartVertex = bestStartVertex;
    result.timeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    return result;
}
