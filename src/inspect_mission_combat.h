#pragma once
#include "resources/data_store.h"
#include <iosfwd>
void inspect_mission_combat(std::istream &, std::ostream &, apache::resources::data_store const &, std::string const &, std::string const &, bool armed = false, bool hurt = false);
