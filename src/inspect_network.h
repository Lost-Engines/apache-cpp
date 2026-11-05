#pragma once
#include <iosfwd>
#include "resources/data_store.h"
namespace apache { void inspect_network(std::istream &input, std::ostream &output);
void inspect_network_live(resources::data_store const &data, unsigned int mode, std::ostream &output); }
