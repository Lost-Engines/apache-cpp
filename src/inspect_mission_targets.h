#pragma once
#include "resources/data_store.h"
#include <iosfwd>
void inspect_mission_targets(std::istream &, std::ostream &, apache::resources::data_store const &, std::string const &, std::string const &);
