#include "../include/InstanceListReader.h"
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

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

    std::vector<std::string> splitSemicolon(const std::string& line) {
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, ';')) {
            parts.push_back(trim(item));
        }
        return parts;
    }
}

std::vector<InstanceListEntry> InstanceListReader::loadList(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Nie mozna otworzyc listy instancji: " + path);
    }

    std::vector<InstanceListEntry> entries;
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::vector<std::string> parts = splitSemicolon(line);
        if (parts.size() < 2) {
            throw std::runtime_error("Niepoprawny wiersz listy instancji: " + line);
        }

        InstanceListEntry entry;
        entry.name = parts[0];
        entry.instanceFile = parts[1];

        if (parts.size() >= 3) {
            entry.optTourFile = parts[2];
        }
        if (parts.size() >= 4 && !parts[3].empty()) {
            entry.optCost = std::stoi(parts[3]);
        }

        entries.push_back(entry);
    }

    return entries;
}
