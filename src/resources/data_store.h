#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "reader.h"

namespace apache::resources {
class data_store {
  std::filesystem::path root;
  std::map<std::string, std::filesystem::path, std::less<>> files;

public:
  data_store(std::filesystem::path const &directory);
  std::filesystem::path const &directory() const;
  std::filesystem::path resolve(std::string_view name) const;
  bool contains(std::string_view name) const;
  bytes read(std::string_view name) const;
  std::vector<std::string> names() const;
  static std::string normalise(std::string_view name);
};
} // namespace apache::resources
