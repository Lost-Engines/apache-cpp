#include "inspect_collisions.h"
#include "simulation/collisions.h"
#include "simulation/actor_movement.h"
#include <algorithm>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>

void inspect_collisions(std::istream &input, std::ostream &output, apache::resources::data_store const &data) {
  using namespace apache;
  auto const models{resources::read_models(data.read("3DDATA/3DOBJS.BIN"))};
  math::trigonometry const trig{data}; render::mobile_scene mobiles{models}; simulation::craters craters;
  std::vector<resources::scenery_placement> scenery, extra;
  std::unique_ptr<simulation::scenery_collisions> collisions;
  std::optional<resources::terrain> terrain;
  int32_t tallest_crater{0}; for(auto id : {11, 257, 385, 62}) tallest_crater = std::max(tallest_crater, models.at(id)->height);
  auto const point{[&](){ math::vector3 p; for(auto &v : p) input >> v; return p; }};
  auto const pose{[&](){ auto p{point()}; uint32_t h{0}, pitch{0}, r{0}; input >> h >> pitch >> r; return math::pose{p, trig.transformation(h, pitch, r)}; }};
  auto const ignored{[&]()->std::optional<uint32_t>{ int64_t id{-1}; input >> id; return id < 0 ? std::nullopt : std::optional{static_cast<uint32_t>(id)}; }};
  auto const ignored_scenery{[&]()->std::optional<std::array<size_t, 2>>{ int64_t layer{-1}, id{-1}; input >> layer >> id; return layer < 0 || id < 0 ? std::nullopt : std::optional{std::array{static_cast<size_t>(layer), static_cast<size_t>(id)}}; }};
  auto const print_scenery{[&](std::optional<simulation::scenery_hit> const &hit){
    if(hit) output << hit->layer << ' ' << hit->index << ' ' << hit->position[0] << ' ' << hit->position[1] << ' ' << hit->position[2] << '\n';
    else output << "-1 -1 0 0 0\n";
  }};
  std::string command;
  while(input >> command) {
    if(command == "reset") {
      std::string zone; input >> zone;
      scenery = resources::read_scenery(data.read(zone + ".WZ/GNDOBJS.BIN")); extra.clear();
      collisions = std::make_unique<simulation::scenery_collisions>(models, trig, scenery, extra);
      terrain.emplace(data.read(zone + ".WZ/GRIDPOST.BIN")); mobiles.clear(); craters.reset();
    } else if(command == "extra") {
      uint16_t model{0}; int32_t x{0}, y{0}; input >> model >> x >> y;
      extra.push_back({static_cast<uint16_t>((x >> 22) * 64 + (y >> 22)), model, x, y});
      collisions = std::make_unique<simulation::scenery_collisions>(models, trig, scenery, extra);
    } else if(command == "perp") {
      auto const start{point()}, end{point()}, target{point()};
      output << simulation::perpendicular_distance(start, end, target, trig) << '\n';
    } else if(command == "intercept") {
      auto const origin{point()}; int32_t speed{0}; input >> speed;
      simulation::movement_target target; target.position = point(); target.direction = point();
      unsigned int iterations{0}; input >> target.speed >> iterations;
      auto const result{simulation::intercept_point(origin, speed, target, iterations)};
      output << result[0] << ' ' << result[1] << ' ' << result[2] << '\n';
    } else if(command == "point") {
      auto const p{point()}; auto const ignore{ignored_scenery()}; print_scenery(collisions->point(p, ignore));
    } else if(command == "line") {
      auto const start{point()}, end{point()}; auto const ignore{ignored_scenery()}; print_scenery(collisions->line(start, end, ignore));
    } else if(command == "add") {
      uint32_t model{0}; input >> model; auto const transform{pose()}; auto const id{mobiles.add(model, transform)}; render::maintain_model(mobiles.at(id));
    } else if(command == "move") {
      uint32_t id{0}; input >> id; mobiles.move(id, pose()); render::maintain_model(mobiles.at(id));
    } else if(command == "remove") {
      uint32_t id{0}; input >> id; mobiles.remove(id);
    } else if(command == "mobile-point") {
      auto const p{point()}; auto const first{ignored()}, second{ignored()};
      auto const hit{simulation::mobile_point_collision(mobiles, models, p, first, second)}; output << (hit ? static_cast<int64_t>(*hit) : -1) << '\n';
    } else if(command == "mobile-line") {
      auto const start{point()}, end{point()}; auto const first{ignored()}, second{ignored()};
      auto const hit{simulation::mobile_line_collision(mobiles, models, trig, start, end, first, second)}; output << (hit ? static_cast<int64_t>(*hit) : -1) << '\n';
    } else if(command == "box") {
      uint32_t id{0}; input >> id; auto const p{point()}; auto const &object{mobiles.at(id)};
      auto const hit{simulation::bounding_box_hit(object, p, models)};
      output << simulation::rotated_box_contains(object, p, models) << ' ' << (hit ? static_cast<int64_t>(hit->id) : -1) << '\n';
    } else if(command == "mark") {
      size_t index{0}; input >> index; auto &mark{craters.long_lived.at(index)};
      mark.position = point(); input >> mark.model >> mark.heading >> mark.age; mark.scale = point();
    } else if(command == "crater-point") {
      output << simulation::crater_point_collision(craters, models, *terrain, point(), tallest_crater) << '\n';
    } else if(command == "crater-line") {
      auto const start{point()}, end{point()}; output << simulation::crater_line_collision(craters, models, *terrain, trig, start, end, tallest_crater) << '\n';
    } else throw std::runtime_error{"Unknown collision inspection operation"};
    if(!input) throw std::runtime_error{"Incomplete collision inspection operation"};
  }
}
