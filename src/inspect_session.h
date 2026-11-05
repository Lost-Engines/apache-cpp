#pragma once
#include <iosfwd>
#include "resources/data_store.h"
void inspect_session(apache::resources::data_store const &data, int mission, unsigned int video, bool arcade, std::istream &input, std::ostream &output, int shared_seat = -1, int network_mode = -1, unsigned int local = 0, unsigned int players = 2);

void inspect_communications(std::istream &input, std::ostream &output);
