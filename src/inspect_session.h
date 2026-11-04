#pragma once
#include <iosfwd>
#include "resources/data_store.h"
void inspect_session(apache::resources::data_store const &data, int mission, unsigned int video, bool arcade, std::istream &input, std::ostream &output);

void inspect_communications(std::istream &input, std::ostream &output);
