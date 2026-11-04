#pragma once
#include <array>
#include <bit>
#include <cstdint>

namespace apache::math {
using vector3 = std::array<int32_t, 3>;
using matrix3 = std::array<int32_t, 9>;
inline int32_t rounded_q31(uint64_t const product) {
  return std::bit_cast<int32_t>(static_cast<uint32_t>((product + 0x40000000ull) >> 31));
}
inline uint64_t product_bits(int32_t const a, int32_t const b) {
  return static_cast<uint64_t>(int64_t{a} * b);
}
inline int32_t multiply_q31(int32_t const a, int32_t const b) {
  return rounded_q31(product_bits(a, b));
}
inline int32_t add(int32_t const a, int32_t const b) {
  return std::bit_cast<int32_t>(static_cast<uint32_t>(a) + static_cast<uint32_t>(b));
}
inline int32_t subtract(int32_t const a, int32_t const b) {
  return std::bit_cast<int32_t>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b));
}
inline int32_t negate(int32_t const value) {
  return std::bit_cast<int32_t>(0u - static_cast<uint32_t>(value));
}
} // namespace apache::math
