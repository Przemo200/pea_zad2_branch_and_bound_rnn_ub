#include "../include/BranchAndBoundSolver.h"
#include "../include/BranchAndBoundNode.h"
#include "../include/LowerBound.h"
#include "../include/MemoryUsage.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <deque>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

   // np. BFS, bfs albo Bfs
    std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    // komparator dla kolejki priorytetowej w LC
    // 1) mniejszy lowerBound
    // 2) przy remisie większa głębokość
    // 3) przy dalszym remisie mniejszy koszt częściowy
    struct LowestBoundCompare {
        bool operator()(const BranchAndBoundNode& a, const BranchAndBoundNode& b) const {
            if (a.lowerBound != b.lowerBound) {
                return a.lowerBound > b.lowerBound;
            }
            if (a.depth != b.depth) {
                return a.depth < b.depth;
            }
            return a.partialCost > b.partialCost;
        }
    };
}

// zamienia tekst z configu na enum strategii
SearchStrategy BranchAndBoundSolver::parseStrategy(const std::string& text) {
    std::string value = toLower(text);
    if (value == "bfs") return SearchStrategy::BFS;
    if (value == "dfs") return SearchStrategy::DFS;
    if (value == "lc" || value == "lowest_cost" || value == "best_first") return SearchStrategy::LOWEST_COST;
    throw std::runtime_error("Nieznana strategia przeszukiwania: " + text);
}

// zamienia enum strategii na krótki tekst używany w csv
std::string BranchAndBoundSolver::strategyToString(SearchStrategy strategy) {
    switch (strategy) {
        case SearchStrategy::BFS:          return "bfs";
        case SearchStrategy::DFS:          return "dfs";
        case SearchStrategy::LOWEST_COST:  return "lc";
    }
    return "unknown";
}


// rozwiązuje instancję z wybraną strategią przeszukiwania, opc. ub oraz limitami czasu i pamięci
BranchAndBoundResult BranchAndBoundSolver::solve(const TSPInstance& instance,
                                                  SearchStrategy strategy,
                                                  const InitialUpperBound& initialUpperBound,
                                                  int timeLimitMs,
                                                  int memoryLimitMb) {
    // rozmiar instancji nie może przekroczyć ustalonego limitu.
    BranchAndBoundNode::validateDimension(instance.dimension);

    const int n   = instance.dimension;
    const int INF = std::numeric_limits<int>::max() / 4;

    // MB na KB
    const long long memoryLimitKb =
        (memoryLimitMb > 0) ? static_cast<long long>(memoryLimitMb) * 1024LL : 0LL;

    BranchAndBoundResult result;

    // mamy startowe UB, to od razu ustawia je jako aktualnie najlepszy koszt i mozna ciac
    result.initialUpperBound = initialUpperBound;
    result.bestCost = initialUpperBound.available ? initialUpperBound.cost : INF;
    result.bestTour = initialUpperBound.available ? initialUpperBound.tour : std::vector<int>{};


    result.peakRssKb = MemoryUsage::getCurrentRSSkB();

    // biezacy rss
    auto sampleCurrentRss = [&]() -> long {
        long rss = MemoryUsage::getCurrentRSSkB();
        if (rss > result.peakRssKb) {
            result.peakRssKb = rss;
        }
        return rss;
    };

    // czy przekroczono limit pamięci.
    auto memoryExceeded = [&]() -> bool {
        if (memoryLimitKb <= 0) {
            return false;
        }
        long rss = sampleCurrentRss();
        return rss > 0 && rss >= memoryLimitKb;
    };

    // start
    // zaczynam zawsze z miasta 0, koszt częściowy = 0, głębokość = 1.
    BranchAndBoundNode root;
    root.path[0]     = static_cast<unsigned char>(0);
    root.pathLen     = 1;
    root.visitedMask = 0ULL;
    root.markVisited(0);
    root.currentVertex = 0;
    root.depth         = 1;
    root.partialCost   = 0;
    root.lowerBound    = LowerBound::compute(instance, root);

    // frontier
    // - BFS i DFS używają deque
    // - LOWEST_COST używa priority_queue
    std::deque<BranchAndBoundNode> dequeContainer;
    std::priority_queue<BranchAndBoundNode,
                        std::vector<BranchAndBoundNode>,
                        LowestBoundCompare> pq;

    //  aktualna liczba węzłów w frontierze
    auto frontierSize = [&]() -> std::size_t {
        return (strategy == SearchStrategy::LOWEST_COST) ? pq.size() : dequeContainer.size();
    };

    //  węzeł do odpowiedniego frontier i aktualizuje maksymalny jego rozmiar
    auto pushNode = [&](BranchAndBoundNode node) {
        if (strategy == SearchStrategy::LOWEST_COST) {
            pq.push(std::move(node));
        } else {
            dequeContainer.push_back(std::move(node));
        }
        result.frontierPeak = std::max(result.frontierPeak, frontierSize());
    };

    // Pobiera kolejny węzeł zgodnie z strategią
    // BFS z przodu kolejki
    // DFS z końca kolejki
    // LC z kolejki priorytetowej
    auto popNode = [&]() -> BranchAndBoundNode {
        if (strategy == SearchStrategy::BFS) {
            BranchAndBoundNode node = dequeContainer.front();
            dequeContainer.pop_front();
            return node;
        }
        if (strategy == SearchStrategy::DFS) {
            BranchAndBoundNode node = dequeContainer.back();
            dequeContainer.pop_back();
            return node;
        }
        // LOWEST_COST
        BranchAndBoundNode node = pq.top();
        pq.pop();
        return node;
    };

    // inicjalizacja frontieru korzeniem
    pushNode(root);
    result.nodesGenerated = 1;
    result.maxDepth       = 1;

    const auto startTime = std::chrono::steady_clock::now();

    // aktualizuje czas działania od startu solvera
    auto updateElapsed = [&]() {
        const auto now = std::chrono::steady_clock::now();
        result.timeMs = std::chrono::duration<double, std::milli>(now - startTime).count();
    };

    try {
        while (frontierSize() > 0) {
            updateElapsed();

            // stop po przekroczeniu limitu czasu
            if (result.timeMs > static_cast<double>(timeLimitMs)) {
                result.hitTimeLimit = true;
                break;
            }

            // co pewien czas sprawdzam memory
            if (((result.nodesExpanded & 255LL) == 0LL) && memoryExceeded()) {
                result.hitMemoryLimit = true;
                break;
            }

            // kolejny węzel do rozwinięcia
            BranchAndBoundNode node = popNode();
            result.maxDepth = std::max(result.maxDepth, node.depth);

            // jeśli lower bound tego węzła nie daje już szansy poprawić bestCost cały podproblem można odrzucić
            if (node.lowerBound >= result.bestCost) {
                result.nodesPruned++;
                continue;
            }

            result.nodesExpanded++;

            // jeśli ścieżka odwiedziła już wszystkie miasta wystarczy domknąć cykl powrotem do 0 i sprawdzić pełny koszt.
            if(node.depth == n) {
                int fullCost = node.partialCost + instance.matrix[node.currentVertex][0];
                if (fullCost < result.bestCost) {
                    result.bestCost = fullCost;
                    result.bestTour = node.pathToVector();
                }
                continue;
            }

            // obsługuje wygenerowanie jednego dziecka dla przejścia do miasta next
            auto processCandidate = [&](int next) {
                // koszt częściowy po dołożeniu nowej krawędzi
                const int newPartialCost =
                    node.partialCost + instance.matrix[node.currentVertex][next];

                result.nodesGenerated++;
                result.maxDepth = std::max(result.maxDepth, node.depth + 1);

                // mem check
                if (((result.nodesGenerated & 511LL) == 0LL) && memoryExceeded()) {
                    result.hitMemoryLimit = true;
                    return;
                }

                // Szybkie przycięcie jeśli sam koszt częściowy już nie poprawi bestCost, nie ma sensu liczyć LB
                if (newPartialCost >= result.bestCost) {
                    result.nodesPruned++;
                    return;
                }

                // Tworzymy dziecko przez skopiowanie płaskiej struktury rodzic i dopisanie nowego miasta na końcu ścieżki
                BranchAndBoundNode child = node;
                child.path[child.pathLen++] = static_cast<unsigned char>(next);
                child.markVisited(next);
                child.currentVertex = next;
                child.depth         = node.depth + 1;
                child.partialCost   = newPartialCost;
                child.lowerBound    = LowerBound::compute(instance, child);

                // Jeśli dziecko jest nierealizowalne albo jego LB jest za duży również można je od razu odrzucić
                if (child.lowerBound >= INF || child.lowerBound >= result.bestCost) {
                    result.nodesPruned++;
                    return;
                }

                // Jeśli dziecko właśnie domknęło ścieżkę na wszystkich miastach to jego lower bound odpowiada już pełnemu kosztowi cyklu
                if (child.depth == n) {
                    if (child.lowerBound < result.bestCost) {
                        result.bestCost = child.lowerBound;
                        result.bestTour = child.pathToVector();
                    }
                    return;
                }

                // W przeciwnym razie wrzucam dziecko do frontieru
                pushNode(std::move(child));
            };

            // Dla DFS przechodzi w odwrotnej kolejności żeby numerycznie mniejsze miasta były zdejmowane wcześniej z końca deque
            if (strategy == SearchStrategy::DFS) {
                for (int next = n - 1; next >= 1; --next) {
                    if (!node.isVisited(next)) {
                        processCandidate(next);
                        if (result.hitMemoryLimit) break;
                    }
                }
            } else {
                // BFS i LOWEST_COST generują kandydatów rosnąco po numerach miast
                for (int next = 1; next < n; ++next) {
                    if (!node.isVisited(next)) {
                        processCandidate(next);
                        if (result.hitMemoryLimit) break;
                    }
                }
            }

            if (result.hitMemoryLimit) break;
        }
    } catch (const std::bad_alloc&) {
        // jeśli system nie da już pamięci to jak memory limit
        result.hitMemoryLimit = true;
    }

    updateElapsed();

    // ostatnia próba odczytu pamięci po zakończeniu
    try { sampleCurrentRss(); } catch (...) {}

    // jawne zwolnienie frontierów przed wyjściem z funkcji
    {
        std::deque<BranchAndBoundNode> empty;
        std::swap(dequeContainer, empty);
    }
    {
        std::priority_queue<BranchAndBoundNode,
                            std::vector<BranchAndBoundNode>,
                            LowestBoundCompare> empty;
        std::swap(pq, empty);
    }

    // jeśli nie znaleziono żadnego rozwiązania to  -1 jako znacznik braku wyniku
    if (result.bestCost == INF) {
        result.bestCost = -1;
    }

    // solved = true tylko wtedy, gdy nie było timeoutu, memory limitu i znaleziono poprawny koszt
    result.solved = !result.hitTimeLimit && !result.hitMemoryLimit && result.bestCost != -1;
    return result;
}