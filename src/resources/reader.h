#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace apache::resources {
using bytes = std::vector<uint8_t>;

class reader {
  std::span<uint8_t const> data;
  std::string context;
  size_t cursor{0};

public:
  reader(std::span<uint8_t const> data, std::string_view context = "resource");
  size_t size() const;
  size_t position() const;
  size_t remaining() const;
  void seek(size_t position);
  void skip(size_t count);
  std::span<uint8_t const> take(size_t count);
  std::span<uint8_t const> slice(size_t position, size_t count) const;
  uint8_t u8();
  uint16_t u16();
  int16_t i16();
  uint32_t u32();
  int32_t i32();
  uint16_t be16();
  uint32_t be32();
  std::string text(size_t count);
  std::string terminated_text(size_t count);
  [[noreturn]] void fail(std::string_view message) const;
};

std::vector<uint32_t> read_offset_table(std::span<uint8_t const> data);
uint64_t fingerprint(std::span<uint8_t const> data);
} // namespace apache::resources
