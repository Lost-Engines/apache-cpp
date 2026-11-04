#pragma once
#include <iosfwd>
#include "resources/data_store.h"
void inspect_player_weapons(apache::resources::data_store const &data, std::istream &input, std::ostream &output);
void inspect_player_mounts(apache::resources::data_store const &data, std::istream &input, std::ostream &output);
void inspect_player_sight(apache::resources::data_store const &data, std::istream &input, std::ostream &output);
void inspect_player_warnings(std::istream &input, std::ostream &output);
