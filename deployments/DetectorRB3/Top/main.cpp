#include "Topology.hpp"

#include <Os/Os.hpp>
#include <Os/Task.hpp>

#include <Fw/Time/TimeInterval.hpp>

#include <cstdlib>
#include <csignal>
#include <getopt.h>
#include <iostream>
#include <string>

namespace {

using namespace DetectorRB3App;

volatile std::sig_atomic_t g_shouldStop = 0;

void handleSignal(int) {
    g_shouldStop = 1;
}

void printUsage(const char* app) {
    std::cout << "Usage: " << app << " [options]\n"
              << "  -a <addr>   Bind address for the GDS TCP server (default: 0.0.0.0 or DETECTOR_GDS_HOST)\n"
              << "  -p <port>   Port for the GDS TCP server (default: 50000 or DETECTOR_GDS_PORT)\n"
              << "  -s <path>   Unix-domain socket path for feature frames (default: /var/run/detector.frames or DETECTOR_SOCK)\n"
              << "  -f <file>   CSV fallback path for feature frames (default: frames.csv or DETECTOR_CSV)\n"
              << "  -h          Show this help message\n";
}

U16 parsePort(const char* text, U16 fallback) {
    if (text == nullptr) {
        return fallback;
    }
    const long value = std::strtol(text, nullptr, 10);
    if (value <= 0 || value > 65535) {
        return fallback;
    }
    return static_cast<U16>(value);
}

}  // namespace

int main(int argc, char* argv[]) {
    Os::init();

    const char* envHost = std::getenv("DETECTOR_GDS_HOST");
    const char* envPort = std::getenv("DETECTOR_GDS_PORT");
    const char* envSock = std::getenv("DETECTOR_SOCK");
    const char* envCsv = std::getenv("DETECTOR_CSV");

    std::string host = envHost ? envHost : "0.0.0.0";
    U16 port = parsePort(envPort, static_cast<U16>(50000));
    std::string socketPath = envSock ? envSock : "/var/run/detector.frames";
    std::string csvPath = envCsv ? envCsv : "frames.csv";

    int opt = 0;
    while ((opt = ::getopt(argc, argv, "ha:p:s:f:")) != -1) {
        switch (opt) {
            case 'a':
                host = optarg;
                break;
            case 'p':
                port = parsePort(optarg, port);
                break;
            case 's':
                socketPath = optarg;
                break;
            case 'f':
                csvPath = optarg;
                break;
            case 'h':
            default:
                printUsage(argv[0]);
                return (opt == 'h') ? 0 : 1;
        }
    }

    DetectorRB3App::TopologyState state{};
    state.gdsHostname = host.c_str();
    state.gdsPort = port;
    state.frameSocket = socketPath.empty() ? nullptr : socketPath.c_str();
    state.frameCsv = csvPath.empty() ? nullptr : csvPath.c_str();

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    setupTopology(state);
    startRateGroups(Fw::TimeInterval(1, 0));

    while (!g_shouldStop) {
        Os::Task::delay(Fw::TimeInterval(0, 250000000));  // 250ms sleep
    }

    stopRateGroups();
    teardownTopology(state);
    return 0;
}
