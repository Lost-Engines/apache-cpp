#pragma once
#include <cstdint>
#include <optional>
namespace apache::platform {
std::optional<uint16_t> scan_code(int key, int modifiers = 0);
}
