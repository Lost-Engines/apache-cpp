#include "inspect_flight.h"
#include <bit>
#include <iomanip>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include "simulation/flight_systems.h"

void inspect_flight_systems(std::istream &input, std::ostream &output) {
  using namespace apache::simulation;
  std::string operation;
  while(input >> operation) {
    powerplant power; flight_controls controls; hover_control hover; flight_events events;
    double dt{0}, altitude{0}, u{0}, v{0}, climb{0}, pitch{0};
    int32_t delta_time{0}, ground{0}, lift{0}, maximum_lift{0}, airspeed{0}, rotor_rate{0};
    bool infinite{false}, invulnerable{false}, overtorque{false}, carrier{false};
    std::array<int32_t, 4> raw_controls{}; unsigned int surface{0}, previous_death{0};
    input >> dt >> delta_time >> altitude >> ground >> lift >> maximum_lift >> airspeed >> infinite >> invulnerable >> overtorque;
    std::array const doubles{&power.fuel, &power.weight, &power.maximum_weight, &power.maximum_fuel_consumption, &power.weight_factor, &power.fuel_consumption, &power.fuel_rate, &power.endurance, &power.collective, &power.engine_rpm, &power.rotor_rpm, &power.engine_torque, &power.rotor_torque, &power.descent_ratio, &power.engine_rpm_target, &power.torque_target, &power.rotor_rpm_target, &power.left_rpm, &power.right_rpm, &power.temperature, &power.left_temperature, &power.right_temperature, &power.overtorque_timer, &power.warning_timer};
    for(auto *value : doubles) input >> *value;
    input >> power.faults >> power.overheat_message >> power.overtorque_message >> power.rotor_sound;
    for(auto &value : raw_controls) input >> value;
    for(auto &value : controls.values) input >> value;
    for(auto &value : controls.previous) input >> value;
    input >> controls.changed >> carrier >> hover.active >> hover.previous >> u >> v >> climb >> pitch >> surface >> previous_death;
    if(!input) throw std::runtime_error{"Incomplete flight-system case"};
    landing_result landing;
    if(operation == "fuel") power.consume_fuel(dt, airspeed, infinite, invulnerable, events);
    else if(operation == "engine") rotor_rate = power.update_engines(dt, altitude, ground, lift, maximum_lift);
    else if(operation == "temperature") power.update_temperature(dt);
    else if(operation == "faults") power.update_faults(dt, delta_time, overtorque, events);
    else if(operation == "controls") controls.update(raw_controls, carrier);
    else if(operation == "hover") hover.update(u, v, climb, pitch, controls.changed, power.collective, events);
    else if(operation == "toggle") hover.toggle(u);
    else if(operation == "landing") landing = evaluate_landing(u, climb, surface, carrier, previous_death);
    else throw std::runtime_error{"Unknown flight-system operation"};
    output << std::hex;
    for(auto const *value : doubles) output << std::bit_cast<uint64_t>(*value) << ' ';
    for(auto const value : controls.values) output << std::bit_cast<uint64_t>(value) << ' ';
    for(auto const value : controls.previous) output << std::bit_cast<uint64_t>(value) << ' ';
    output << std::bit_cast<uint64_t>(u) << ' ' << std::bit_cast<uint64_t>(v) << std::dec << ' ';
    output << power.faults << ' ' << power.overheat_message << ' ' << power.overtorque_message << ' ' << power.rotor_sound << ' ' << rotor_rate << ' ' << controls.changed << ' ' << hover.active << ' ' << hover.previous << ' ' << landing.grade << ' ' << landing.death << ' ' << landing.landed << ' ' << landing.award;
    for(auto const event : events) output << ' ' << static_cast<unsigned int>(event);
    output << '\n';
  }
}
