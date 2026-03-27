#include "../include/TourUtils.h"
#include <sstream>

namespace TourUtils {

bool isValidTour(const std::vector<int>& tour, int n) {
    if (static_cast<int>(tour.size()) != n) {
        return false;
    }

    std::vector<bool> visited(n, false);
    for (int v : tour) {
        if (v < 0 || v >= n || visited[v]) {
            return false;
        }
        visited[v] = true;
    }
    return true;
}

int calculateTourCost(const TSPInstance& instance, const std::vector<int>& tour) {
    if (!isValidTour(tour, instance.dimension)) {
        return -1;
    }

    int cost = 0;
    int n = instance.dimension;
    for (int i = 0; i < n - 1; ++i) {
        cost += instance.matrix[tour[i]][tour[i + 1]];
    }
    cost += instance.matrix[tour[n - 1]][tour[0]];
    return cost;
}

std::string tourToString(const std::vector<int>& tour) {
    std::ostringstream out;
    for (size_t i = 0; i < tour.size(); ++i) {
        out << tour[i];
        if (i + 1 < tour.size()) {
            out << " -> ";
        }
    }
    if (!tour.empty()) {
        out << " -> " << tour[0];
    }
    return out.str();
}

double computeRelativeErrorPercent(int value, int optimum) {
    if (value < 0 || optimum <= 0) {
        return -1.0;
    }
    return 100.0 * static_cast<double>(value - optimum) / static_cast<double>(optimum);
}

}
