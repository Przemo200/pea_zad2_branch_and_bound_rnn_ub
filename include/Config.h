#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    std::string mode = "single_run";          // test_read, single_run, benchmark_random, benchmark_tsplib, compare_ub
    std::string strategy = "all";             // bfs, dfs, lc, all

    std::string instance_file;
    std::string opt_tour_file;
    std::string list_file;
    std::string output_csv = "results/out.csv";

    std::string generated_type = "TSP";       // TSP albo ATSP

    bool show_matrix = false;
    bool progress = true;
    bool use_rnn_upper_bound = true;          // dla single_run / benchmark_*

    int single_opt_cost = -1;

    int min_n = 6;
    int max_n = 12;
    int instances_per_size = 20;

    int weight_min = 1;
    int weight_max = 1000;

    int time_limit_ms = 900000;               // 15 min
    int memory_limit_mb = 0;                 // 0 = brak limitu
    unsigned int seed = 12345;
};

class ConfigLoader {
public:
    static Config loadFromFile(const std::string& path);

private:
    static void validate(const Config& config);
};

#endif
