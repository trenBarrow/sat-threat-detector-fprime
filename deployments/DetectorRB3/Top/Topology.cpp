#include "Topology.hpp"

#include <deployments/DetectorRB3/Components/Detector/DetectorComponentImpl.hpp>
#include <AppTopologyAc.hpp>

#include <Drv/Ip/IpSocket.hpp>
#include <Drv/TcpServer/TcpServerComponentImpl.hpp>
#include <Fw/Buffer/Buffer.hpp>
#include <Fw/Logger/Logger.hpp>
#include <Fw/Types/String.hpp>
#include <Svc/ActiveRateGroup/ActiveRateGroup.hpp>
#include <Svc/RateGroupDriver/RateGroupDriver.hpp>

#include <Os/Task.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace DetectorRB3App;

namespace DetectorRB3App {
namespace ConfigObjects {
namespace CdhCore_health {
Svc::HealthImpl::PingEntry pingEntries[3] = {
    {PingEntries::CdhCore_cmdDisp::WARN, PingEntries::CdhCore_cmdDisp::FATAL, Fw::String("cmdDisp")},
    {PingEntries::CdhCore_events::WARN, PingEntries::CdhCore_events::FATAL, Fw::String("events")},
    {PingEntries::CdhCore_tlmSend::WARN, PingEntries::CdhCore_tlmSend::FATAL, Fw::String("tlmSend")},
};
}  // namespace CdhCore_health
}  // namespace ConfigObjects
}  // namespace DetectorRB3App

namespace {

// ----------------------------------------------------------------------
// Scheduler configuration
// ----------------------------------------------------------------------

constexpr FwTaskPriorityType kComDriverPriority = 100;
constexpr Os::Task::ParamType kComDriverStack = Os::Task::TASK_DEFAULT;
constexpr Os::Task::ParamType kComDriverCpu = Os::Task::TASK_DEFAULT;

Svc::RateGroupDriver::DividerSet g_rateDivisors{{{1, 0}}};
U32 g_rateGroupContext[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};

// ----------------------------------------------------------------------
// Frame ingress worker state
// ----------------------------------------------------------------------

struct PipelineConfig {
    std::string socketPath{"/var/run/detector.frames"};
    std::string csvPath{"frames.csv"};
    std::size_t featureTokenCount{16};
    std::size_t guardTokenIndex{17};

    [[nodiscard]] std::size_t frameFloatCount() const {
        return featureTokenCount + 1;  // guard bits occupy the last slot
    }
};

struct WorkerState {
    DetectorComponentImpl* detector{nullptr};
    PipelineConfig config{};
    std::vector<std::string> tokens{};
    std::vector<float> frame{};
};

std::atomic<bool> g_workerRunning{false};
std::thread g_workerThread;
WorkerState g_workerState;

// ----------------------------------------------------------------------
// Utility helpers
// ----------------------------------------------------------------------

void splitCsv(const std::string& line, std::vector<std::string>& out) {
    out.clear();
    std::size_t start = 0;
    while (start <= line.size()) {
        const auto comma = line.find(',', start);
        if (comma == std::string::npos) {
            out.emplace_back(line.substr(start));
            break;
        }
        out.emplace_back(line.substr(start, comma - start));
        start = comma + 1;
    }
}

float parseFloat(const std::string& token) {
    char* end = nullptr;
    const float value = std::strtof(token.c_str(), &end);
    return (end == token.c_str()) ? 0.0f : value;
}

unsigned int parseUnsigned(const std::string& token) {
    char* end = nullptr;
    const unsigned long value = std::strtoul(token.c_str(), &end, 10);
    return (end == token.c_str()) ? 0U : static_cast<unsigned int>(value);
}

void loadSchema(PipelineConfig& config) {
    std::ifstream in("config/feature_schema.csv");
    if (!in) {
        return;  // Leave defaults in place
    }
    std::string header;
    if (!std::getline(in, header)) {
        return;
    }
    std::vector<std::string> columns;
    splitCsv(header, columns);
    if (columns.size() < 3) {
        return;
    }

    config.guardTokenIndex = columns.size() - 1;
    if (config.guardTokenIndex <= 1) {
        config.guardTokenIndex = 17;  // fallback to defaults
        config.featureTokenCount = 16;
    } else {
        config.featureTokenCount = config.guardTokenIndex - 1;  // skip ts column
    }
}

void processRecord(const std::string& record) {
    if (!g_workerState.detector || record.empty()) {
        return;
    }
    if (record.rfind("ts,", 0) == 0) {
        return;  // Skip header repeaters
    }

    splitCsv(record, g_workerState.tokens);
    if (g_workerState.tokens.size() <= 1) {
        return;  // Nothing but timestamp
    }

    const auto& cfg = g_workerState.config;
    if (g_workerState.tokens.size() <= cfg.guardTokenIndex) {
        // Not enough tokens to cover guard bits. Treat as malformed frame.
        return;
    }

    if (g_workerState.frame.size() != cfg.frameFloatCount()) {
        g_workerState.frame.assign(cfg.frameFloatCount(), 0.0f);
    } else {
        std::fill(g_workerState.frame.begin(), g_workerState.frame.end(), 0.0f);
    }

    const std::size_t featureColumns = std::min(cfg.featureTokenCount, g_workerState.tokens.size() - 1);
    for (std::size_t i = 0; i < featureColumns && i < g_workerState.frame.size(); ++i) {
        // Offset by one to skip the timestamp column.
        g_workerState.frame[i] = parseFloat(g_workerState.tokens[1 + i]);
    }

    const unsigned int guardBits = parseUnsigned(g_workerState.tokens[cfg.guardTokenIndex]);
    if (cfg.featureTokenCount < g_workerState.frame.size()) {
        g_workerState.frame[cfg.featureTokenCount] = static_cast<float>(guardBits);
    }

    Fw::Buffer buffer(reinterpret_cast<U8*>(g_workerState.frame.data()),
                      static_cast<FwSizeType>(g_workerState.frame.size() * sizeof(float)));
    g_workerState.detector->ingestBufferForBringup(buffer);
}

void runSocketLoop(int fd) {
    std::string carry;
    std::vector<char> buffer(4096, 0);
    while (g_workerRunning.load()) {
        const ssize_t count = ::recv(fd, buffer.data(), buffer.size(), 0);
        if (count <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        carry.append(buffer.data(), static_cast<std::size_t>(count));
        std::size_t searchStart = 0;
        for (;;) {
            const auto newline = carry.find('\n', searchStart);
            if (newline == std::string::npos) {
                carry.erase(0, searchStart);
                break;
            }
            const std::string record = carry.substr(searchStart, newline - searchStart);
            searchStart = newline + 1;
            processRecord(record);
        }
    }
}

int connectSocket(const std::string& path) {
    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path.c_str());
    if (::connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

void socketIngress(const std::string& path) {
    const int fd = connectSocket(path);
    if (fd < 0) {
        return;
    }
    runSocketLoop(fd);
    ::close(fd);
}

void csvIngress(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return;
    }
    std::string line;
    // Skip header if present
    if (std::getline(in, line)) {
        if (line.rfind("ts,", 0) != 0) {
            processRecord(line);
        }
    }
    while (g_workerRunning.load()) {
        if (!std::getline(in, line)) {
            in.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        processRecord(line);
    }
}

void ingressWorker() {
    loadSchema(g_workerState.config);

    if (!g_workerState.config.socketPath.empty()) {
        socketIngress(g_workerState.config.socketPath);
        if (!g_workerRunning.load()) {
            return;
        }
    }

    if (!g_workerState.config.csvPath.empty()) {
        csvIngress(g_workerState.config.csvPath);
    }
}

void configurePipeline(const TopologyState& state) {
    g_workerState.detector = &detector;
    g_workerState.config.socketPath = state.frameSocket ? state.frameSocket : "/var/run/detector.frames";
    g_workerState.config.csvPath = state.frameCsv ? state.frameCsv : "frames.csv";
    g_workerState.config.featureTokenCount = 16;
    g_workerState.config.guardTokenIndex = g_workerState.config.featureTokenCount + 1;
    g_workerState.frame.assign(g_workerState.config.frameFloatCount(), 0.0f);
}

void configureComponents(const TopologyState& state) {
    rateGroupDriverComp.configure(g_rateDivisors);
    rateGroup1Comp.configure(g_rateGroupContext, FW_NUM_ARRAY_ELEMENTS(g_rateGroupContext));

    const char* host = state.gdsHostname ? state.gdsHostname : "0.0.0.0";
    const U16 port = state.gdsPort != 0 ? state.gdsPort : static_cast<U16>(50000);
    const Drv::SocketIpStatus status = comDriver.configure(host, port);
    if (status != Drv::SOCK_SUCCESS) {
        Fw::Logger::log("[WARN] TcpServer configure(%s:%hu) failed with status %d\n", host, port, status);
    }
    comDriver.setAutomaticOpen(true);
}

}  // namespace

namespace DetectorRB3App {

void setupTopology(const TopologyState& state) {
    configurePipeline(state);

    initComponents(state);
    setBaseIds();
    connectComponents();
    regCommands();
    configComponents(state);
    configureComponents(state);
    loadParameters();
    startTasks(state);

    // Start the TCP server read task
    Os::TaskString recvTask("TcpServer");
    comDriver.start(recvTask, kComDriverPriority, kComDriverStack, kComDriverCpu);

    // Launch ingest worker
    g_workerRunning.store(true);
    g_workerThread = std::thread(ingressWorker);
}

void startRateGroups(const Fw::TimeInterval& interval) {
    linuxTimer.startTimer(interval);
}

void stopRateGroups() {
    linuxTimer.quit();
}

void teardownTopology(const TopologyState& state) {
    // Stop ingestion thread first to prevent enqueueing after teardown begins
    g_workerRunning.store(false);
    if (g_workerThread.joinable()) {
        g_workerThread.join();
    }

    // Stop server tasks
    comDriver.stopReconnect();
    comDriver.stop();
    (void)comDriver.joinReconnect();
    (void)comDriver.join();

    stopTasks(state);
    freeThreads(state);
    tearDownComponents(state);
}

}  // namespace DetectorRB3App
