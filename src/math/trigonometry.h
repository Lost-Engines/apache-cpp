#pragma once
#include <vector>
#include "fixed.h"
#include "resources/data_store.h"

namespace apache::math {
class trigonometry {
  std::vector<int32_t> sin_cos;

public:
  trigonometry(resources::data_store const &data);
  int32_t sine(uint32_t angle) const;
  int32_t cosine(uint32_t angle) const;
  matrix3 transformation(uint32_t heading, uint32_t pitch, uint32_t roll) const;
};
} // namespace apache::math
