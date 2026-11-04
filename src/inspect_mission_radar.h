#pragma once
#include "resources/data_store.h"
#include <iosfwd>
void inspect_mission_radar(std::ostream &output, apache::resources::data_store const &data, std::string const &zone, std::string const &path);
