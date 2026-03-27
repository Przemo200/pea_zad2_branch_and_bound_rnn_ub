#include "../include/Generator.h"
#include <random>
#include <stdexcept>

TSPInstance Generator::generateATSP(int n, int weightMin, int weightMax, unsigned int seed) {
    if (n <= 0) {
        throw std::runtime_error("Niepoprawny rozmiar instancji ATSP");
    }

    // generator losowy i takie samo P na wylosowanie wagi z zakresu min max
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(weightMin, weightMax);

    TSPInstance instance;
    instance.name = "generated_atsp_" + std::to_string(n);
    instance.type = "ATSP";
    instance.edgeWeightType = "EXPLICIT";
    instance.dimension = n;
    instance.matrix.assign(n, std::vector<int>(n, 0)); // macierz wypelniona zerami

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                instance.matrix[i][j] = 0;
            } else {
                instance.matrix[i][j] = dist(rng);
            }
        }
    }

    return instance;
}

TSPInstance Generator::generateTSP(int n, int weightMin, int weightMax, unsigned int seed) {
    if (n <= 0) {
        throw std::runtime_error("Niepoprawny rozmiar instancji TSP");
    }

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(weightMin, weightMax);

    TSPInstance instance;
    instance.name = "generated_tsp_" + std::to_string(n);
    instance.type = "TSP";
    instance.edgeWeightType = "EXPLICIT";
    instance.dimension = n;
    instance.matrix.assign(n, std::vector<int>(n, 0)); // macierz na start zera

    for (int i = 0; i < n; i++) {
        instance.matrix[i][i] = 0;
    }

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            int w = dist(rng);
            instance.matrix[i][j] = w;
            instance.matrix[j][i] = w; // jedna waga po obu stronach przekatnej - graf nieskierowany
        }
    }

    return instance;
}