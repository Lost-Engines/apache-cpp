#pragma once
#include <iosfwd>
#include "resources/data_store.h"
void inspect_audio(apache::resources::data_store const &data, std::istream &input, std::ostream &output);
