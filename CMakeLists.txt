cmake_minimum_required(VERSION 3.16)
project(pea_zad2_branch_and_bound_rnn_ub)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(include)

add_executable(pea_zad2
        src/main.cpp
        src/Config.cpp
        src/FileReader.cpp
        src/Generator.cpp
        src/InstanceListReader.cpp
        src/OptTourReader.cpp
        src/TourUtils.cpp
        src/CSVWriter.cpp
        src/MemoryUsage.cpp
        src/RNNSolver.cpp
        src/LowerBound.cpp
        src/BranchAndBoundSolver.cpp
)