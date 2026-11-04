#pragma once
#include <array>
#include <filesystem>
#include "simulation/pilot_log.h"

namespace apache::configuration {
struct pilot_slot {
  unsigned int file{1};
  simulation::pilot_log log;
};
struct user_profile {
  std::string name;
  std::vector<pilot_slot> pilots;
  size_t selected{0};
};
class profiles {
  std::filesystem::path directory;
  void ensure_pilot(uint32_t now);
public:
  std::array<user_profile, 10> users;
  size_t user{0};
  explicit profiles(uint32_t now = 0);
  simulation::pilot_log &current();
  simulation::pilot_log const &current() const;
  void load(std::filesystem::path const &directory, uint32_t now);
  void save() const;
  void select_user(size_t index, uint32_t now);
  bool create(uint32_t now);
  bool erase();
};
std::filesystem::path default_save_directory();
} // namespace apache::configuration
