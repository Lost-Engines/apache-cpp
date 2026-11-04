#pragma once
#include "resources/data_store.h"
#include <iosfwd>
void inspect_combat_update(std::istream &input, std::ostream &output, apache::resources::data_store const &data);
