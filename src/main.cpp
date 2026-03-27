#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>


#include "../include/BranchAndBoundSolver.h"
#include "../include/CSVWriter.h"
#include "../include/Config.h"
#include "../include/FileReader.h"
#include "../include/Generator.h"
#include "../include/InstanceListReader.h"
#include "../include/MemoryUsage.h"
#include "../include/OptTourReader.h"
#include "../include/RNNSolver.h"
#include "../include/TourUtils.h"

struct OptCostInfo {
    bool available = false;
    int cost = -1;
    std::string source;
};

struct AggregateStats {
    double timeSum = 0.0;
    double ubTimeSum = 0.0;
    double totalTimeSum = 0.0;
    long long generatedSum = 0;
    long long expandedSum = 0;
    long long prunedSum = 0;
    long long rssSum = 0;
    int rssCount = 0;
    int count = 0;
    int solvedCount = 0;
    int memoryLimitedCount = 0;
};

class AddressSpaceLimitGuard {
public:
    explicit AddressSpaceLimitGuard(int) {}
};

static std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

static void printMatrix(const TSPInstance& instance) {
    std::cout << "\nMacierz kosztow:\n";
    for (int i = 0; i < instance.dimension; ++i) {
        for (int j = 0; j < instance.dimension; ++j) {
            std::cout << std::setw(6) << instance.matrix[i][j] << " ";
        }
        std::cout << "\n";
    }
}

static void printProgress(bool enabled, int currentStep, int totalSteps, const std::string& label) {
    if (!enabled || totalSteps <= 0) {
        return;
    }
    double percent = 100.0 * static_cast<double>(currentStep) / static_cast<double>(totalSteps);
    std::cout << "[POSTEP] " << currentStep << "/" << totalSteps << " (" << percent << "%) - " << label << "\n";
}

static double safeAverage(double sum, int count) {
    return (count > 0) ? (sum / static_cast<double>(count)) : 0.0;
}


static std::vector<SearchStrategy> selectedStrategies(const Config& config) {
    if (config.strategy == "all") {
        return {SearchStrategy::BFS, SearchStrategy::DFS, SearchStrategy::LOWEST_COST};
    }
    return {BranchAndBoundSolver::parseStrategy(config.strategy)};
}

static TSPInstance generateInstance(const Config& config, int n, unsigned int localSeed) {
    std::string type = toUpper(config.generated_type);
    if (type == "TSP") {
        return Generator::generateTSP(n, config.weight_min, config.weight_max, localSeed);
    }
    if (type == "ATSP") {
        return Generator::generateATSP(n, config.weight_min, config.weight_max, localSeed);
    }
    throw std::runtime_error("generated_type musi byc TSP albo ATSP");
}

// z pliku config
static OptCostInfo getOptCostForSingleRun(const Config& config, const TSPInstance& instance) {
    OptCostInfo info;

    if (!config.opt_tour_file.empty()) {
        std::vector<int> optTour = OptTourReader::loadTour(config.opt_tour_file);
        if (!TourUtils::isValidTour(optTour, instance.dimension)) {
            throw std::runtime_error("Niepoprawny plik opt_tour_file");
        }
        info.available = true;
        info.cost = TourUtils::calculateTourCost(instance, optTour);
        info.source = ".opt.tour";
        return info;
    }

    if (config.single_opt_cost > 0) {
        info.available = true;
        info.cost = config.single_opt_cost;
        info.source = "single_opt_cost";
    }

    return info;
}

// z tej listy instancji do benchmark
static OptCostInfo getOptCostFromEntry(const InstanceListEntry& entry, const TSPInstance& instance) {
    OptCostInfo info;

    if (!entry.optTourFile.empty()) {
        std::vector<int> optTour = OptTourReader::loadTour(entry.optTourFile);
        if (!TourUtils::isValidTour(optTour, instance.dimension)) {
            throw std::runtime_error("Niepoprawny plik opt_tour dla instancji: " + entry.name);
        }
        info.available = true;
        info.cost = TourUtils::calculateTourCost(instance, optTour);
        info.source = ".opt.tour";
        return info;
    }

    if (entry.optCost > 0) {
        info.available = true;
        info.cost = entry.optCost;
        info.source = "opt_cost";
    }

    return info;
}

static InitialUpperBound computeInitialUb(const TSPInstance& instance, bool useRnnUpperBound) {
    InitialUpperBound ub;
    if (!useRnnUpperBound) {
        return ub;
    }

    RNNResult rnn = RNNSolver::solveWithTies(instance);
    ub.available = (rnn.bestCost >= 0);
    ub.cost = rnn.bestCost;
    ub.tour = rnn.bestTour;
    ub.timeMs = rnn.timeMs;
    return ub;
}

static BranchAndBoundResult solveWithLimits(const TSPInstance& instance,
                                            SearchStrategy strategy,
                                            const InitialUpperBound& ub,
                                            const Config& config) {
    AddressSpaceLimitGuard guard(config.memory_limit_mb);
    return BranchAndBoundSolver::solve(instance,
                                       strategy,
                                       ub,
                                       config.time_limit_ms,
                                       config.memory_limit_mb);
}

static BnBCsvRow buildCsvRow(const std::string& mode,
                             const TSPInstance& instance,
                             int instanceId,
                             unsigned int seed,
                             const OptCostInfo& optInfo,
                             SearchStrategy strategy,
                             const BranchAndBoundResult& result,
                             bool usedRnnUb,
                             long rssKb) {
    BnBCsvRow row;
    row.mode = mode;
    row.searchStrategy = BranchAndBoundSolver::strategyToString(strategy);
    row.ubMode = usedRnnUb ? "rnn_ties" : "none";
    row.instanceName = instance.name;
    row.instanceType = instance.type;
    row.n = instance.dimension;
    row.instanceId = instanceId;
    row.seed = seed;
    row.initialUbCost = result.initialUpperBound.available ? result.initialUpperBound.cost : -1;
    row.initialUbTimeMs = result.initialUpperBound.available ? result.initialUpperBound.timeMs : -1.0;
    row.bestCost = result.bestCost;
    row.optCost = optInfo.available ? optInfo.cost : (result.solved ? result.bestCost : -1);
    row.relativeErrorPercent = (row.bestCost > 0 && row.optCost > 0)
        ? TourUtils::computeRelativeErrorPercent(row.bestCost, row.optCost)
        : -1.0;
    row.timeMs = result.timeMs;
    row.totalTimeMs = result.timeMs + (result.initialUpperBound.available ? result.initialUpperBound.timeMs : 0.0);
    row.rssKb = rssKb;
    row.frontierPeak = result.frontierPeak;
    row.maxDepth = result.maxDepth;
    row.nodesGenerated = result.nodesGenerated;
    row.nodesExpanded = result.nodesExpanded;
    row.nodesPruned = result.nodesPruned;
    row.solved = result.solved;
    row.hitTimeLimit = result.hitTimeLimit;
    row.hitMemoryLimit = result.hitMemoryLimit;
    return row;
}

static void printSingleResult(const BranchAndBoundResult& result,
                              SearchStrategy strategy,
                              const OptCostInfo& optInfo) {
    std::cout << "Strategia: " << BranchAndBoundSolver::strategyToString(strategy) << "\n";
    std::cout << "UB startowe: " << (result.initialUpperBound.available ? "RNN z remisami" : "brak") << "\n";
    if (result.initialUpperBound.available) {
        std::cout << "Koszt UB: " << result.initialUpperBound.cost << "\n";
        std::cout << "Czas RNN [ms]: " << result.initialUpperBound.timeMs << "\n";
    }

    std::cout << "Najlepszy koszt: " << result.bestCost << "\n";
    if (!result.bestTour.empty()) {
        std::cout << "Trasa: " << TourUtils::tourToString(result.bestTour) << "\n";
    }
    if (optInfo.available && result.bestCost > 0) {
        std::cout << "Optimum: " << optInfo.cost
                  << " | blad [%] = " << TourUtils::computeRelativeErrorPercent(result.bestCost, optInfo.cost) << "\n";
    }
    std::cout << "Czas BnB [ms]: " << result.timeMs << "\n";
    double totalTimeMs = result.timeMs + (result.initialUpperBound.available ? result.initialUpperBound.timeMs : 0.0);
    std::cout << "Czas calkowity [ms]: " << totalTimeMs << "\n";
    std::cout << "Wygenerowane wezly: " << result.nodesGenerated << "\n";
    std::cout << "Rozwiniete wezly: " << result.nodesExpanded << "\n";
    std::cout << "Przyciete wezly: " << result.nodesPruned << "\n";
    std::cout << "Peak frontier: " << result.frontierPeak << "\n";
    std::cout << "Max glebokosc: " << result.maxDepth << "\n";
    std::cout << "Pamiec procesu [kB]: " << result.peakRssKb << "\n";
    std::cout << "Status: ";
    if (result.solved) {
        std::cout << "zakonczono";
    } else if (result.hitMemoryLimit) {
        std::cout << "przerwano limitem pamieci/alokacji";
    } else if (result.hitTimeLimit) {
        std::cout << "przerwano limitem czasu";
    } else {
        std::cout << "zakonczono bez potwierdzenia optimum";
    }
    std::cout << "\n\n";
}

static void runSingle(const Config& config) {
    TSPInstance instance = FileReader::loadInstance(config.instance_file);
    OptCostInfo optInfo = getOptCostForSingleRun(config, instance);
    std::vector<SearchStrategy> strategies = selectedStrategies(config);

    std::cout << "=== SINGLE RUN ===\n";
    std::cout << "Instancja: " << instance.name << " | typ = " << instance.type << " | n = " << instance.dimension << "\n";
    if (config.show_matrix) {
        printMatrix(instance);
    }
    if (optInfo.available) {
        std::cout << "Znane optimum: " << optInfo.cost << " (" << optInfo.source << ")\n";
    }
    std::cout << "\n";

    CSVWriter::writeBnBHeaderIfNeeded(config.output_csv);

    int step = 0;
    for (SearchStrategy strategy : strategies) {
        ++step;
        printProgress(config.progress, step, static_cast<int>(strategies.size()),
                      BranchAndBoundSolver::strategyToString(strategy));

        InitialUpperBound ub = computeInitialUb(instance, config.use_rnn_upper_bound);
        BranchAndBoundResult result = solveWithLimits(instance, strategy, ub, config);
        long rss = result.peakRssKb;

        printSingleResult(result, strategy, optInfo);
        CSVWriter::appendBnBRow(config.output_csv,
                                buildCsvRow(config.mode, instance, 1, config.seed,
                                            optInfo, strategy, result,
                                            config.use_rnn_upper_bound, rss));
    }
}

static void runBenchmarkRandom(const Config& config, bool compareUb) {
    std::vector<SearchStrategy> strategies = selectedStrategies(config);
    CSVWriter::writeBnBHeaderIfNeeded(config.output_csv);

    std::vector<bool> ubModes = compareUb ? std::vector<bool>{false, true}
                                          : std::vector<bool>{config.use_rnn_upper_bound};

    int totalSteps = (config.max_n - config.min_n + 1) * config.instances_per_size *
                     static_cast<int>(strategies.size()) * static_cast<int>(ubModes.size());
    int currentStep = 0;

    std::cout << "=== BENCHMARK RANDOM ===\n";
    std::cout << "Typ: " << config.generated_type
              << " | n: " << config.min_n << "-" << config.max_n
              << " | instancji/rozmiar: " << config.instances_per_size << "\n\n";

    for (int n = config.min_n; n <= config.max_n; ++n) {
        std::map<std::string, AggregateStats> summary;
        std::cout << "Rozmiar n = " << n << "\n";

        for (int instanceId = 1; instanceId <= config.instances_per_size; ++instanceId) {
            unsigned int localSeed = config.seed + static_cast<unsigned int>(n * 1000 + instanceId);
            TSPInstance instance = generateInstance(config, n, localSeed);
            instance.name += "_id" + std::to_string(instanceId);

            for (SearchStrategy strategy : strategies) {
                for (bool useRnnUb : ubModes) {
                    ++currentStep;
                    printProgress(config.progress, currentStep, totalSteps,
                                  "n=" + std::to_string(n) + " id=" + std::to_string(instanceId) + " " +
                                  BranchAndBoundSolver::strategyToString(strategy) +
                                  " UB=" + std::string(useRnnUb ? "rnn_ties" : "none"));

                    InitialUpperBound ub = computeInitialUb(instance, useRnnUb);
                    BranchAndBoundResult result = solveWithLimits(instance, strategy, ub, config);
                    long rss = result.peakRssKb;

                    OptCostInfo optInfo;
                    if (result.solved) {
                        optInfo.available = true;
                        optInfo.cost = result.bestCost;
                        optInfo.source = "bnb_exact";
                    }

                    CSVWriter::appendBnBRow(config.output_csv,
                                            buildCsvRow(compareUb ? "compare_ub" : config.mode,
                                                        instance,
                                                        instanceId,
                                                        localSeed,
                                                        optInfo,
                                                        strategy,
                                                        result,
                                                        useRnnUb,
                                                        rss));

                    std::string key = BranchAndBoundSolver::strategyToString(strategy) + "|" + (useRnnUb ? "rnn_ties" : "none");
                    AggregateStats& agg = summary[key];
                    agg.timeSum += result.timeMs;
                    agg.ubTimeSum += (ub.available ? ub.timeMs : 0.0);
                    double totalTimeMs = result.timeMs + (ub.available ? ub.timeMs : 0.0);
                    agg.totalTimeSum += totalTimeMs;
                    agg.generatedSum += result.nodesGenerated;
                    agg.expandedSum += result.nodesExpanded;
                    agg.prunedSum += result.nodesPruned;
                    if (rss >= 0) {
                        agg.rssSum += rss;
                        agg.rssCount++;
                    }
                    agg.count++;
                    if (result.solved) {
                        agg.solvedCount++;
                    }
                    if (result.hitMemoryLimit) {
                        agg.memoryLimitedCount++;
                    }
                }
            }
        }

        for (const auto& [key, agg] : summary) {
            std::cout << "  " << key
                      << " | sr. czas BnB [ms] = " << safeAverage(agg.timeSum, agg.count)
                      << " | sr. czas UB [ms] = " << safeAverage(agg.ubTimeSum, agg.count)
                      << " | sr. czas total [ms] = " << safeAverage(agg.totalTimeSum, agg.count)
                      << " | sr. generated = " << safeAverage(static_cast<double>(agg.generatedSum), agg.count)
                      << " | sr. expanded = " << safeAverage(static_cast<double>(agg.expandedSum), agg.count)
                      << " | sr. pruned = " << safeAverage(static_cast<double>(agg.prunedSum), agg.count)
                      << " | sr. pamiec [kB] = " << (agg.rssCount > 0 ? safeAverage(static_cast<double>(agg.rssSum), agg.rssCount) : -1.0)
                      << " | solved = " << agg.solvedCount << "/" << agg.count
                      << " | memory_limit = " << agg.memoryLimitedCount << "/" << agg.count
                      << "\n";
        }
        std::cout << "\n";
    }
}

static void runBenchmarkTsplib(const Config& config, bool compareUb) {
    std::vector<InstanceListEntry> entries = InstanceListReader::loadList(config.list_file);
    std::vector<SearchStrategy> strategies = selectedStrategies(config);
    CSVWriter::writeBnBHeaderIfNeeded(config.output_csv);

    std::vector<bool> ubModes = compareUb ? std::vector<bool>{false, true}
                                          : std::vector<bool>{config.use_rnn_upper_bound};

    int totalSteps = static_cast<int>(entries.size()) * static_cast<int>(strategies.size()) * static_cast<int>(ubModes.size());
    int currentStep = 0;

    std::cout << "=== BENCHMARK TSPLIB ===\n";
    std::cout << "Lista: " << config.list_file << "\n\n";

    for (size_t i = 0; i < entries.size(); ++i) {
        const InstanceListEntry& entry = entries[i];
        TSPInstance instance = FileReader::loadInstance(entry.instanceFile);
        OptCostInfo optInfo = getOptCostFromEntry(entry, instance);

        std::cout << "Instancja: " << entry.name
                  << " | typ = " << instance.type
                  << " | n = " << instance.dimension;
        if (optInfo.available) {
            std::cout << " | optimum = " << optInfo.cost;
        }
        std::cout << "\n";

        for (SearchStrategy strategy : strategies) {
            for (bool useRnnUb : ubModes) {
                ++currentStep;
                printProgress(config.progress, currentStep, totalSteps,
                              entry.name + " " + BranchAndBoundSolver::strategyToString(strategy) +
                              " UB=" + std::string(useRnnUb ? "rnn_ties" : "none"));

                InitialUpperBound ub = computeInitialUb(instance, useRnnUb);
                BranchAndBoundResult result = solveWithLimits(instance, strategy, ub, config);
                long rss = result.peakRssKb;
                unsigned int localSeed = config.seed + static_cast<unsigned int>(1000 + i);

                CSVWriter::appendBnBRow(config.output_csv,
                                        buildCsvRow(compareUb ? "compare_ub" : config.mode,
                                                    instance,
                                                    static_cast<int>(i + 1),
                                                    localSeed,
                                                    optInfo,
                                                    strategy,
                                                    result,
                                                    useRnnUb,
                                                    rss));
                double totalTimeMs = result.timeMs + (ub.available ? ub.timeMs : 0.0);
                std::cout << "  " << BranchAndBoundSolver::strategyToString(strategy)
                          << " | UB=" << (useRnnUb ? "rnn_ties" : "none")
                          << " | koszt = " << result.bestCost
                          << " | czas [ms] = " << result.timeMs
                          << " | total [ms] = " << totalTimeMs
                          << " | generated = " << result.nodesGenerated
                          << " | solved = " << (result.solved ? "tak" : "nie");
                if (result.hitMemoryLimit) {
                    std::cout << " | stop=memory";
                } else if (result.hitTimeLimit) {
                    std::cout << " | stop=time";
                }
                std::cout << "\n";
            }
        }

        std::cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        std::string configPath = "config/00_test_read.txt";
        if (argc > 1) {
            configPath = argv[1];
        }

        Config config = ConfigLoader::loadFromFile(configPath);

        if (config.mode == "test_read") {
            TSPInstance instance = FileReader::loadInstance(config.instance_file);
            std::cout << "=== TEST READ ===\n";
            std::cout << "Nazwa: " << instance.name << "\n";
            std::cout << "Typ: " << instance.type << "\n";
            std::cout << "EDGE_WEIGHT_TYPE: " << instance.edgeWeightType << "\n";
            std::cout << "Rozmiar: " << instance.dimension << "\n";
            if (config.show_matrix) {
                printMatrix(instance);
            }
            return 0;
        }

        if (config.mode == "single_run") {
            runSingle(config);
            return 0;
        }

        if (config.mode == "benchmark_random") {
            runBenchmarkRandom(config, false);
            return 0;
        }

        if (config.mode == "benchmark_tsplib") {
            runBenchmarkTsplib(config, false);
            return 0;
        }

        if (config.mode == "compare_ub") {
            if (!config.list_file.empty()) {
                runBenchmarkTsplib(config, true);
            } else {
                runBenchmarkRandom(config, true);
            }
            return 0;
        }

        throw std::runtime_error("Nieobslugiwany mode: " + config.mode);
    }
    catch (const std::exception& ex) {
        std::cerr << "BLAD: " << ex.what() << "\n";
        return 1;
    }
}