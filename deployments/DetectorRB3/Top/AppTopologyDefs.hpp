#pragma once

#include "Fw/Log/LogString.hpp"
#include "Svc/Health/HealthComponentImpl.hpp"
#include "Svc/Subtopologies/CdhCore/PingEntries.hpp"
#include "Svc/Subtopologies/CdhCore/SubtopologyTopologyDefs.hpp"
#include "Svc/Subtopologies/CdhCore/CdhCoreConfig/FppConstantsAc.hpp"
#include "Svc/Subtopologies/ComFprime/Ports_ComBufferQueueEnumAc.hpp"
#include "Svc/Subtopologies/ComFprime/Ports_ComPacketQueueEnumAc.hpp"
#include "Svc/Subtopologies/ComFprime/ComFprimeConfig/FppConstantsAc.hpp"
#include "Svc/Subtopologies/ComFprime/SubtopologyTopologyDefs.hpp"

namespace DetectorRB3App {
struct TopologyState {
    const char* gdsHostname;
    U16 gdsPort;
    const char* frameSocket;
    const char* frameCsv;
    CdhCore::SubtopologyState cdhCore;
    ComFprime::SubtopologyState comFprime;
};

namespace ConfigObjects {
namespace CdhCore_health {
extern Svc::HealthImpl::PingEntry pingEntries[3];
}
}  // namespace ConfigObjects
}  // namespace DetectorRB3App
