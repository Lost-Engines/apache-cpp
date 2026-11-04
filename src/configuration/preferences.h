#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace apache::configuration {
class preferences {
  std::vector<std::pair<std::string, std::string>> entries;

public:
  void load(std::string_view text);
  std::string serialise() const;
  std::optional<std::string_view> get(std::string_view key) const;
  std::optional<int> integer(std::string_view key) const;
  void set(std::string_view key, std::string_view value);
  void set(std::string_view key, int value);
};
} // namespace apache::configuration
