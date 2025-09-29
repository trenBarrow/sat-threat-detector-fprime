#pragma once
// Derived implementation of the generated base component
#include <string>
#include <vector>
#include <mutex>
#include "DetectorComponentAi.hpp"
#include "deployments/DetectorRB3/Components/Detector/DetectorComponentAc.hpp"
#include <Fw/Buffer/Buffer.hpp>
#include <Fw/Types/String.hpp>

class DetectorComponentImpl : public ::DetectorRB3::DetectorComponentBase {
  public:
    explicit DetectorComponentImpl(const char* compName, const std::string& config_dir = "config");
    void init(U32 queueDepth, U32 instance);
    // Bring-up helper to feed a buffer directly
    void ingestBufferForBringup(Fw::Buffer& fwBuffer);

  private:
    // Port handler: FeatureIn
    void FeatureIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;

    // Helpers
    void load_threshold(const std::string& config_dir);

    // Runtime
    DetectorComponentAi ai;
    std::mutex mu;
    double tau{0.5};
};
