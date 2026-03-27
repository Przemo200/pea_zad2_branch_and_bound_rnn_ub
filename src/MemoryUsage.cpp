#include "../include/MemoryUsage.h"
#include <fstream>
#include <sstream>
#include <string>

namespace {
    long readValueFromStatus(const char* key) {
        std::ifstream file("/proc/self/status");
        std::string line;

        while (std::getline(file, line)) {
            if (line.rfind(key, 0) == 0) {
                std::stringstream ss(line);
                std::string label;
                long value = 0;
                std::string unit;
                ss >> label >> value >> unit;
                return value;
            }
        }

        return -1;
    }
}

long MemoryUsage::getCurrentRSSkB() {
    return readValueFromStatus("VmRSS:");
}

long MemoryUsage::getPeakRSSkB() {
    long vmHwm = readValueFromStatus("VmHWM:");
    if (vmHwm > 0) {
        return vmHwm;
    }
    return getCurrentRSSkB();
}