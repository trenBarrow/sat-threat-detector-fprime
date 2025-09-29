cmake_minimum_required(VERSION 3.13)
project(RefSat)
# Include the F´ framework CMake from an adjacent checkout
include("${FPRIME_FRAMEWORK_PATH}/cmake/FPrime.cmake")
include("${FPRIME_FRAMEWORK_PATH}/cmake/FPrime-Code.cmake")
register_fprime_executable(
    RefSat
    SOURCES
        Top/Topology.cpp
        Top/main.cpp
)
