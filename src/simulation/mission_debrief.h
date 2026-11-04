#pragma once
#include "debrief.h"
#include "mission_scenery.h"

namespace apache::simulation {
class mission_runtime;
std::optional<target_identity> nearest_ground_object(mission_scenery const &scene, int32_t x, int32_t y);
debrief_result evaluate_debrief(mission_runtime const &runtime, debrief_records const &records, uint32_t &random, std::string_view pilot_name);
} // namespace apache::simulation
