cmake_minimum_required(VERSION 3.13)
project(DetectorRB3)
# Include the F´ framework CMake from an adjacent checkout
include("${FPRIME_FRAMEWORK_PATH}/cmake/FPrime.cmake")
include("${FPRIME_FRAMEWORK_PATH}/cmake/FPrime-Code.cmake")

# Include Detector component subdirectory to build library + autocoded files
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Components/Detector")
register_fprime_executable(
    DetectorRB3
    AUTOCODER_INPUTS
        Top/instances.fpp
        Top/Topology.fpp
    SOURCES
        Top/Topology.cpp
        Top/main.cpp
    DEPENDS
        Detector
)
