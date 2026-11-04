#pragma once
#include <iosfwd>
#include "resources/data_store.h"
void inspect_arcade(std::istream &input, std::ostream &output);
void inspect_arcade_trace(std::istream &input, std::ostream &output, apache::resources::data_store const &data);
