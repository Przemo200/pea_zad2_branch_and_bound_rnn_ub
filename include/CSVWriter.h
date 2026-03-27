#ifndef CSVWRITER_H
#define CSVWRITER_H

#include <cstddef>
#include <string>

struct BnBCsvRow {
    std::string mode;
    std::string searchStrategy;
    std::string ubMode;               // none albo rnn_ties
    std::string instanceName;
    std::string instanceType;
    int n = 0;
    int instanceId = 0;
    unsigned int seed = 0;
    int initialUbCost = -1;
    double initialUbTimeMs = -1.0;
    int bestCost = -1;
    int optCost = -1;
    double relativeErrorPercent = -1.0;
    double timeMs = 0.0;
    double totalTimeMs = 0.0;
    long rssKb = -1;
    std::size_t frontierPeak = 0;
    int maxDepth = 0;
    long long nodesGenerated = 0;
    long long nodesExpanded = 0;
    long long nodesPruned = 0;
    bool solved = false;
    bool hitTimeLimit = false;
    bool hitMemoryLimit = false;
};

class CSVWriter {
public:
    static void writeBnBHeaderIfNeeded(const std::string& path);
    static void appendBnBRow(const std::string& path, const BnBCsvRow& row);
};

#endif
