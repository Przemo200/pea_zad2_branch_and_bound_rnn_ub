#ifndef BRANCHANDBOUNDNODE_H
#define BRANCHANDBOUNDNODE_H

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

constexpr int MAX_BNB_DIMENSION = 64;

struct BranchAndBoundNode {
    unsigned char path[MAX_BNB_DIMENSION];
    int           pathLen     = 0;

    std::uint64_t visitedMask  = 0ULL;
    int           currentVertex = 0;
    int           depth         = 1;
    int           partialCost   = 0;
    int           lowerBound    = 0;

    BranchAndBoundNode() {
    }

    bool isVisited(int vertex) const {
        return (visitedMask & (1ULL << vertex)) != 0ULL;
    }

    void markVisited(int vertex) {
        visitedMask |= (1ULL << vertex);
    }

    std::vector<int> pathToVector() const {
        std::vector<int> result(pathLen);
        for (int i = 0; i < pathLen; ++i) {
            result[i] = static_cast<int>(path[i]);
        }
        return result;
    }

    static void validateDimension(int n) {
        if (n <= 0) {
            throw std::runtime_error("Instancja ma niepoprawny rozmiar");
        }
        if (n > MAX_BNB_DIMENSION) {
            throw std::runtime_error(
                "Rozmiar instancji przekracza MAX_BNB_DIMENSION=" +
                std::to_string(MAX_BNB_DIMENSION));
        }
    }
};

#endif