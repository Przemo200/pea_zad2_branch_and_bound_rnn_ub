#include "../include/LowerBound.h"
#include <algorithm>
#include <limits>

namespace {
    // intuicja
    // - z bieżącego wierzchołka trzeba jeszcze gdzieś wyjść
    // - z każdego jeszcze nieodwiedzonego wierzchołka też trzeba będzie kiedyś wyjść
    int minOutgoingNeeded(const TSPInstance& instance, const BranchAndBoundNode& node) {
        const int INF = std::numeric_limits<int>::max() / 4;
        int n = instance.dimension;

        // jeśli odwiedzono już wszystkie miasta, zostało tylko wrócić do miasta startowego 0
        if (node.depth == n) {
            return instance.matrix[node.currentVertex][0];
        }

        int sum = 0;
        int bestFromCurrent = INF;

        // jeśli został dokładnie jeden nieodwiedzony wierzchołekv to z currentVertex trzeba wyjść właśnie do niego.
        if (node.depth == n - 1) {
            for (int v = 0; v < n; ++v) {
                if (!node.isVisited(v)) {
                    bestFromCurrent = instance.matrix[node.currentVertex][v];
                    break;
                }
            }
        } else {
            // W ogólnym przypadku bierzemy najtańsze możliwe wyjście   z aktualnego wierzchołka do dowolnego jeszcze nieodwiedzonego miasta.
            for (int v = 0; v < n; ++v) {
                if (!node.isVisited(v)) {
                    bestFromCurrent = std::min(bestFromCurrent, instance.matrix[node.currentVertex][v]);
                }
            }
        }

        if (bestFromCurrent >= INF) {
            return INF;
        }
        sum += bestFromCurrent;

        // dla każdego jeszcze nieodwiedzonego miasta szukamy najtańszego możliwego wyjścia
        // albo do miasta startowego 0, albo do innego jeszcze nieodwiedzonego miasta
        // co daje dolne oszacowanie kosztu reszty cyklu

        for (int u = 0; u < n; ++u) {
            if (node.isVisited(u)) {
                continue;
            }

            int best = INF;
            for (int v = 0; v < n; ++v) {
                if (v == u) {
                    continue; // nie mozna do samego siebie
                }
                if (v == 0 || !node.isVisited(v)) {
                    best = std::min(best, instance.matrix[u][v]);
                }
            }

            if (best >= INF) {
                return INF;
            }
            sum += best;
        }

        return sum;
    }

    // intuicja
    // - do miasta startowego 0 ktoś musi na końcu wejść
    // - do każdego jeszcze nieodwiedzonego miasta ktoś też musi wejść
    int minIncomingNeeded(const TSPInstance& instance, const BranchAndBoundNode& node) {
        const int INF = std::numeric_limits<int>::max() / 4;
        int n = instance.dimension;

        // jeśli ścieżka obejmuje już wszystkie miasta, nie potrzeba już dodatkowych wejść bo juz takich nie ma
        if (node.depth == n) {
            return 0;
        }

        int sum = 0;
        int bestIntoStart = INF;

        // Szukamy najtańszego możliwego wejścia do miasta startowego 0 z któregoś jeszcze nieodwiedzonego miasta
        for (int u = 1; u < n; ++u) {
            if (!node.isVisited(u)) {
                bestIntoStart = std::min(bestIntoStart, instance.matrix[u][0]);
            }
        }

        if (bestIntoStart >= INF) {
            return INF;
        }
        sum += bestIntoStart;

        for (int target = 0; target < n; ++target) {
            if (node.isVisited(target)) {
                continue;
            }

            int best = INF;

            // Dla każdego nieodwiedzonego miasta szukamy najtańszego możliwego wejścia albo z currentVertex, albo z innego jeszcze nieodwiedzonego miasta
            for (int src = 0; src < n; ++src) {
                if (src == target) {
                    continue;
                }
                if (src == node.currentVertex || !node.isVisited(src)) {
                    best = std::min(best, instance.matrix[src][target]);
                }
            }

            if (best >= INF) {
                return INF;
            }
            sum += best;
        }

        return sum;
    }
}

// do kosztu już zbudowanej części ścieżki dodajemy oszacowanie kosztu reszty
// bierze max(outgoing, incoming) bo pełny cykl musi spełnić oba warunki - mieć potrzebne wyjścia i potrzebne wejścia.
int LowerBound::compute(const TSPInstance& instance, const BranchAndBoundNode& node) {
    const int INF = std::numeric_limits<int>::max() / 4;
    int outgoing = minOutgoingNeeded(instance, node);
    int incoming = minIncomingNeeded(instance, node);

    if (outgoing >= INF || incoming >= INF) {
        return INF;
    }

    return node.partialCost + std::max(outgoing, incoming);
}