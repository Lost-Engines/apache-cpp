#pragma once
#include <iosfwd>
#include <string_view>
#include "resources/data_store.h"
bool inspect_briefing(apache::resources::data_store const &data, std::string_view command, std::istream &input, std::ostream &output, std::string_view dump);
