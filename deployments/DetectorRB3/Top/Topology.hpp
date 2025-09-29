#pragma once

#include "AppTopologyDefs.hpp"
#include <Fw/Time/TimeInterval.hpp>

namespace DetectorRB3App {
void setupTopology(const TopologyState& state);
void startRateGroups(const Fw::TimeInterval& interval);
void stopRateGroups();
void teardownTopology(const TopologyState& state);
}
