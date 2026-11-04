#include "inspect_projectiles.h"
#include "resources/terrain.h"
#include "simulation/projectile_dynamics.h"
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>

void inspect_projectiles(std::istream &input, std::ostream &output, apache::resources::data_store const &data) {
  using namespace apache;
  math::trigonometry const trig{data}; auto const weapons{simulation::weapon_definitions()};
  std::optional<resources::terrain> terrain;
  auto const point{[&](){ math::vector3 v; for(auto &x : v) input >> x; return v; }};
  auto const print{[&](auto const &v){ for(auto x : v) output << x << ' '; }};
  std::string command;
  while(input >> command) {
    if(command == "terrain") {
      std::string zone; input >> zone; terrain.emplace(data.read(zone + ".WZ/GRIDPOST.BIN"));
    } else if(command == "bound") {
      auto const start{point()}, move{point()};
      auto const result{simulation::bound_projectile_move(start, move, [&](int32_t x, int32_t y){ return terrain->elevation(x, y); })};
      output << result.hit_terrain << ' '; print(result.end); print(result.displacement); print(result.impact); output << '\n';
    } else if(command == "closest") {
      auto const start{point()}, end{point()}, target{point()}; auto const result{simulation::closest_point(start, end, target, trig)};
      output << result.distance << ' '; print(result.position); output << '\n';
    } else if(command == "move") {
      size_t weapon{0}; int32_t delta{0}; simulation::projectile_state round;
      unsigned int adjusted{0}; bool has_target{false}, actor{false}; simulation::movement_target target;
      input >> weapon >> delta >> round.motion.speed >> round.motion.distance >> round.motion.flags;
      round.pose.position = point(); for(auto &v : round.pose.rotation) input >> v;
      round.previous = point(); for(auto &v : round.angles) input >> v;
      input >> round.seeker.flags >> round.seeker.retry_time >> adjusted >> has_target >> actor;
      round.seeker.adjusted = static_cast<simulation::seeker_target>(adjusted);
      target.position = point(); target.direction = point(); input >> target.speed;
      auto const result{simulation::calculate_projectile_move(round, weapons.at(weapon), has_target ? std::optional{target} : std::nullopt, actor, trig, delta)};
      output << round.motion.flags << ' '; print(round.pose.position); print(round.pose.rotation); print(round.angles);
      output << round.seeker.flags << ' ' << round.seeker.retry_time << ' ' << static_cast<int>(round.seeker.adjusted) << ' ' << result.notify_target << ' ' << result.explosion.has_value() << ' ';
      print(result.displacement); output << '\n';
    } else throw std::runtime_error{"Unknown projectile operation"};
    if(!input) throw std::runtime_error{"Incomplete projectile operation"};
  }
}
