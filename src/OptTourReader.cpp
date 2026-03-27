#include "../include/OptTourReader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cctype>

namespace {
    std::string trim(const std::string& s) {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
            start++;
        }

        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
            end--;
        }

        return s.substr(start, end - start);
    }
}

// otwiera plik i sprawdza
std::vector<int> OptTourReader::loadTour(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Nie mozna otworzyc pliku optymalnej trasy: " + path);
    }

    std::vector<int> tour;
    std::string line;
    bool inTourSection = false; // igonruje wszystko dopoki nie w sekcji z trasa

    // czyta linia po linii az dotrze do tour section, konczy gdy eof,
    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty()) {
            continue;
        }

        if (!inTourSection) {
            if (line.find("TOUR_SECTION") != std::string::npos) {
                inTourSection = true;
            }
            continue;
        }

        if (line == "EOF") {
            break;
        }

        // kazda z linii z tour scetion i liczby czytane do zmiennej city
        std::stringstream ss(line);
        int city = 0;

        while (ss >> city) {
            if (city == -1) {
                return tour;
            }

            // TSPLIB ma numeracje od 1, w programie mam od 0
            tour.push_back(city - 1);
        }
    }

    return tour;
}