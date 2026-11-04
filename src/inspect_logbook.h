#pragma once
#include <iosfwd>
#include "resources/data_store.h"
void inspect_logbook(apache::resources::data_store const &data, std::istream &input, std::ostream &output);
