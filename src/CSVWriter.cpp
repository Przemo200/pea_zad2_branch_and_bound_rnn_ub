#include "../include/CSVWriter.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>

void CSVWriter::writeBnBHeaderIfNeeded(const std::string& path) {
    bool needHeader = true;
    if (std::filesystem::exists(path)) {
        std::ifstream in(path);
        if (in.good() && in.peek() != std::ifstream::traits_type::eof()) {
            needHeader = false;
        }
    }

    if (needHeader) {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        std::ofstream out(path, std::ios::app);
        if (!out.is_open()) {
            throw std::runtime_error("Nie mozna otworzyc pliku CSV: " + path);
        }

        out << "mode,search_strategy,ub_mode,instance_name,instance_type,n,instance_id,seed,"
               "initial_ub_cost,initial_ub_time_ms,best_cost,opt_cost,relative_error_percent,"
               "time_ms,total_time_ms,rss_kb,frontier_peak,max_depth,nodes_generated,nodes_expanded,nodes_pruned,"
               "solved,hit_time_limit,hit_memory_limit\n";
    }
}

void CSVWriter::appendBnBRow(const std::string& path, const BnBCsvRow& row) {
    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) {
        throw std::runtime_error("Nie mozna dopisac do pliku CSV: " + path);
    }

    out << row.mode << ","
        << row.searchStrategy << ","
        << row.ubMode << ","
        << row.instanceName << ","
        << row.instanceType << ","
        << row.n << ","
        << row.instanceId << ","
        << row.seed << ","
        << row.initialUbCost << ","
        << row.initialUbTimeMs << ","
        << row.bestCost << ","
        << row.optCost << ","
        << row.relativeErrorPercent << ","
        << row.timeMs << ","
        << row.totalTimeMs << ","
        << row.rssKb << ","
        << row.frontierPeak << ","
        << row.maxDepth << ","
        << row.nodesGenerated << ","
        << row.nodesExpanded << ","
        << row.nodesPruned << ","
        << (row.solved ? 1 : 0) << ","
        << (row.hitTimeLimit ? 1 : 0) << ","
        << (row.hitMemoryLimit ? 1 : 0) << "\n";
}
