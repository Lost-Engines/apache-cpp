#pragma once
#include <iosfwd>
#include <string>
#include "resources/data_store.h"
void inspect_sensors(apache::resources::data_store const &data, std::string const &zone, std::string const &path, std::istream &input, std::ostream &output);
