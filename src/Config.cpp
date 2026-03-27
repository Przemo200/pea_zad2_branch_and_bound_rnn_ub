#include "../include/Config.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

#include "../include/BranchAndBoundNode.h"

namespace {
    std::string trim(const std::string& s) {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
            ++start;
        }
        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
            --end;
        }
        return s.substr(start, end - start);
    }

    std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    bool toBool(const std::string& value) {
        std::string v = toLower(trim(value));
        return v == "1" || v == "true" || v == "yes" || v == "on";
    }
}

Config ConfigLoader::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Nie mozna otworzyc pliku konfiguracyjnego: " + path);
    }

    Config config;
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }

        std::string key = toLower(trim(line.substr(0, eq)));
        std::string value = trim(line.substr(eq + 1));

        if (key == "mode") config.mode = toLower(value);
        else if (key == "strategy") config.strategy = toLower(value);
        else if (key == "instance_file") config.instance_file = value;
        else if (key == "opt_tour_file") config.opt_tour_file = value;
        else if (key == "list_file") config.list_file = value;
        else if (key == "output_csv") config.output_csv = value;
        else if (key == "generated_type") config.generated_type = value;
        else if (key == "show_matrix") config.show_matrix = toBool(value);
        else if (key == "progress") config.progress = toBool(value);
        else if (key == "use_rnn_upper_bound") config.use_rnn_upper_bound = toBool(value);
        else if (key == "single_opt_cost") config.single_opt_cost = std::stoi(value);
        else if (key == "min_n") config.min_n = std::stoi(value);
        else if (key == "max_n") config.max_n = std::stoi(value);
        else if (key == "instances_per_size") config.instances_per_size = std::stoi(value);
        else if (key == "weight_min") config.weight_min = std::stoi(value);
        else if (key == "weight_max") config.weight_max = std::stoi(value);
        else if (key == "time_limit_ms") config.time_limit_ms = std::stoi(value);
        else if (key == "memory_limit_mb") config.memory_limit_mb = std::stoi(value);
        else if (key == "seed") config.seed = static_cast<unsigned int>(std::stoul(value));
    }

    validate(config);
    return config;
}

void ConfigLoader::validate(const Config& config) {
    if (config.mode.empty()) {
        throw std::runtime_error("Brak pola mode w configu");
    }

    if (config.min_n <= 0 || config.max_n <= 0 || config.min_n > config.max_n) {
        throw std::runtime_error("Niepoprawny zakres min_n / max_n");
    }

    if (config.max_n > MAX_BNB_DIMENSION) {
        throw std::runtime_error(
            "max_n=" + std::to_string(config.max_n) +
            " przekracza MAX_BNB_DIMENSION=" +
            std::to_string(MAX_BNB_DIMENSION));
    }

    if (config.instances_per_size <= 0) {
        throw std::runtime_error("instances_per_size musi byc > 0");
    }

    if (config.weight_min > config.weight_max) {
        throw std::runtime_error("weight_min nie moze byc > weight_max");
    }

    if (config.time_limit_ms <= 0) {
        throw std::runtime_error("time_limit_ms musi byc > 0");
    }

    if (config.memory_limit_mb < 0) {
        throw std::runtime_error("memory_limit_mb nie moze byc < 0");
    }

    if (config.mode == "test_read" || config.mode == "single_run") {
        if (config.instance_file.empty()) {
            throw std::runtime_error("Dla mode=test_read i mode=single_run wymagane jest instance_file");
        }
    } else if (config.mode == "benchmark_tsplib") {
        if (config.list_file.empty()) {
            throw std::runtime_error("Dla mode=benchmark_tsplib wymagane jest list_file");
        }
    } else if (config.mode == "compare_ub") {
        if (config.list_file.empty() && config.max_n < config.min_n) {
            throw std::runtime_error("Niepoprawna konfiguracja compare_ub");
        }
    } else if (config.mode != "benchmark_random") {
        throw std::runtime_error("Nieznany mode: " + config.mode);
    }
}
