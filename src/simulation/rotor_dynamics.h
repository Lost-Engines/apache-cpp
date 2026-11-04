#pragma once
#include "flight_systems.h"

namespace apache::simulation {
struct rotor_dynamics {
  powerplant power;
  flight_controls controls;
  hover_control hover;
  // Positions in feet; body-forward, body-sideways and world-vertical speeds in ft/s.
  double x{0}, y{0}, z{0}, u{0}, v{0}, zdot{0};
  // Pitch, roll and heading in radians; body rates about the corresponding axes.
  double theta{0}, phi{0}, psi{0}, p{0}, q{0}, r{0};
  double cos_theta{1}, sin_theta{0}, tan_theta{0}, cos_phi{1}, sin_phi{0};
  double current_landing_pitch{0};
  double qmax{0}, qdem{0}, pmax{0}, pdem{0}, rmax{0}, rdem{0};
  double turnrate{0}, psidot{0}, phidot{0}, thetadot{0};
  double my_roll{0}, roll_a{2}, roll_b{1.5}, roll_c{1}, roll_d{2.2}, roll_decay_ss{0}, roll_decay{0};
  double rho{0}, kge{1}, kdu{0}, beta{0}, Tz{0}, eta{1}, zdotss{0}, Kt{0}, udot{0}, vdot{0}, abs_v{0}, Kdv{0};
  double g{32.2}, max_speed_timer{0};
  int32_t Tzw{0}, zdotT{0}, zdotTmax{80}, wind_strength{0};
  bool wind_gust{false}, max_speed_message{false};
  void update_rates(double dt, double ground, bool on_carrier, bool landed, bool absolute_throttle, unsigned int input_type, int32_t throttle, flight_events &events);
  void update_aerodynamics(double dt, int32_t delta_time, double ground, bool on_carrier, unsigned int random_sample, flight_events &events);
};
} // namespace apache::simulation
