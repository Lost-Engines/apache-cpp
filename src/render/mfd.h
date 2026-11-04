#pragma once
#include "cockpit.h"
#include "instrument_font.h"
#include "projection2.h"

namespace apache::render {
enum class mfd_panel { pilot_left, pilot_right, gunner_left, gunner_right, gunner_middle };
struct engine_instruments {
  double torque_left{0};
  double torque_right{0};
  double temperature_left{0};
  double temperature_right{0};
  double rpm_left{0};
  double rpm_right{0};
  double rotor_rpm{0};
  double collective{0};
};
class mfd_renderer {
  cockpit const &panels;
  instrument_font font;
  bool high;
public:
  mfd_renderer(cockpit const &panels, resources::executable_data const &executable);
  rectangle area(mfd_panel panel) const;
  void corners(framebuffer &target, bool gunner) const;
  void off(framebuffer &target, rectangle area) const;
  void damaged(framebuffer &target, rectangle area, uint32_t &seed) const;
  void engines(framebuffer &target, rectangle area, engine_instruments const &state) const;
  void status(framebuffer &target, rectangle area, uint32_t faults, bool ground_radar, bool air_radar) const;
};
} // namespace apache::render
