#include "trigonometry.h"

namespace apache::math {
trigonometry::trigonometry(resources::data_store const &data) {
  auto const file{data.read("SINCOS.TBL")};
  resources::reader input{file, "SINCOS.TBL"};
  if(input.size() != 10241 * 4) input.fail("Unexpected sine table size");
  sin_cos.reserve(10241);
  while(input.remaining()) sin_cos.push_back(input.i32());
}
int32_t trigonometry::sine(uint32_t const angle) const {
  return sin_cos[(angle >> 19) + ((angle >> 18) & 1)];
}
int32_t trigonometry::cosine(uint32_t const angle) const {
  return sin_cos[2048 + (angle >> 19) + ((angle >> 18) & 1)];
}
matrix3 trigonometry::transformation(uint32_t const heading, uint32_t const pitch, uint32_t const roll) const {
  int32_t const sh{sine(heading)}, ch{cosine(heading)}, sp{sine(pitch)}, cp{cosine(pitch)}, sr{sine(roll)}, cr{cosine(roll)};
  int32_t const sr_sp{multiply_q31(sr, sp)}, negative_cr_sp{multiply_q31(negate(cr), sp)};
  return {
    rounded_q31(product_bits(cr, ch) + product_bits(sh, sr_sp)),
    rounded_q31(product_bits(negate(cr), sh) + product_bits(ch, sr_sp)),
    multiply_q31(negate(sr), cp),
    multiply_q31(cp, sh),
    multiply_q31(cp, ch),
    sp,
    rounded_q31(product_bits(sr, ch) + product_bits(sh, negative_cr_sp)),
    rounded_q31(product_bits(negate(sr), sh) + product_bits(ch, negative_cr_sp)),
    multiply_q31(cr, cp),
  };
}
} // namespace apache::math
