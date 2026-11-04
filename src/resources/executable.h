#pragma once
#include <span>
#include "data_store.h"

namespace apache::resources {
// Read embedded artwork and tables as data. No original code is executed.
class executable_data {
  bytes file;
public:
  explicit executable_data(data_store const &data);
  std::span<uint8_t const> at(uint32_t address, size_t count) const;
};
} // namespace apache::resources
