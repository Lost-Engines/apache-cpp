#pragma once
#include <filesystem>
#include "preferences.h"
#include "resources/data_store.h"
#include "simulation/control_input.h"

namespace apache::configuration {
class settings {
  std::filesystem::path filename;
public:
  preferences values;
  unsigned int video_mode{1};
  unsigned int detail{0};
  unsigned int model{2};
  unsigned int forces{0};
  unsigned int volume{63};
  unsigned int deadband{0};
  bool planner{false};
  bool animations{true};
  bool speech{false};
  bool music{true};
  bool effects{true};
  bool minimal_effects{true};
  simulation::control_configuration controls;
  std::array<simulation::joystick_axis, 4> axes;
  explicit settings(resources::data_store const &data);
  void apply();
  void load(std::filesystem::path const &directory);
  void save() const;
  void set(std::string_view key, int value);
};
} // namespace apache::configuration
