#include "DetectorComponentImpl.hpp"
#include <fstream>
#include <sstream>
#include <cstring>

DetectorComponentImpl::DetectorComponentImpl(const char* compName, const std::string& config_dir)
: DetectorComponentBase(compName) {
    load_threshold(config_dir);
}

void DetectorComponentImpl::init(U32 queueDepth, U32 instance){
    ::DetectorRB3::DetectorComponentBase::init(queueDepth, instance);
}

void DetectorComponentImpl::load_threshold(const std::string& config_dir){
    std::ifstream in(config_dir + "/calibrator.cfg");
    if(!in) return; // default tau
    std::string k; double v;
    while(in >> k >> v){
        if(k == "threshold" || k == "tau") { tau = v; }
    }
}

void DetectorComponentImpl::ingestBufferForBringup(Fw::Buffer& fwBuffer){
    this->FeatureIn_handler(0, fwBuffer);
}

void DetectorComponentImpl::FeatureIn_handler(FwIndexType, Fw::Buffer& fwBuffer){
    // Expect fixed-order float buffer per feature_schema.csv (excluding ts)
    const U8* data = fwBuffer.getData();
    const FwSizeType sz = fwBuffer.getSize();
    if(data == nullptr || sz < 17*sizeof(float)) return;
    const float* f = reinterpret_cast<const float*>(data);
    const size_t nf = sz / sizeof(float);
    FeatureFrame fr; fr.ts = 0.0; fr.x.assign(18, 0.0);
    const size_t copyN = nf < 16 ? nf : 16;
    for(size_t i=0;i<copyN;++i){ fr.x[i] = static_cast<double>(f[i]); }
    unsigned int gb = 0;
    if(nf >= 17) gb = static_cast<unsigned int>(f[16]);
    else if(nf >= 18) gb = static_cast<unsigned int>(f[17]);
    fr.guard_bits = gb;
    fr.x[17] = static_cast<double>(gb);

    ai.ingest(fr);
    const double risk = ai.lastRisk();
    const std::string reason = ai.lastReason();

    // Write RiskScore telemetry (0x7000) and emit RiskAlert event when risk>tau
    this->tlmWrite_RiskScore(static_cast<F32>(risk));
    if(risk > tau){
        Fw::LogStringArg rsn(reason.c_str());
        this->log_WARNING_HI_RiskAlert(static_cast<F32>(risk), rsn);
    }
}
