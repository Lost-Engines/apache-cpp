#pragma once
#include <iosfwd>
#include "resources/data_store.h"
void inspect_menu(apache::resources::data_store const &data, std::istream &input, std::ostream &output);
void inspect_frontend(apache::resources::data_store const &data, std::istream &input, std::ostream &output);
