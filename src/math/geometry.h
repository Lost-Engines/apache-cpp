#pragma once
#include "fixed.h"

namespace apache::math {
inline matrix3 constexpr identity{INT32_MAX, 0, 0, 0, INT32_MAX, 0, 0, 0, INT32_MAX};
struct pose {
  vector3 position{0, 0, 0};
  matrix3 rotation{identity};
};
vector3 normalise_q31(vector3 value);
vector3 transform(matrix3 const &matrix, vector3 const &vector);
matrix3 transpose(matrix3 const &matrix);
matrix3 multiply(matrix3 const &a, matrix3 const &b);
pose attach(pose const &parent, pose const &relative);
int32_t scale_q24(int32_t value, int32_t scale);
std::array<int32_t, 65> subdivide_grid(int32_t coefficient, unsigned int shift);
} // namespace apache::math
