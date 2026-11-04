#pragma once
#include "mission_scenery.h"
#include "war_grid.h"

namespace apache::simulation {
void initialise_mission_radars(war_grid &grid, mission_scenery const &scenery, mission_actors const &actors, std::span<shared_target const> targets, resources::terrain const &terrain);
} // namespace apache::simulation
