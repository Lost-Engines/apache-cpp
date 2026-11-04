#include "render/mobile_scene.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, char const *message) { if(!condition) throw std::runtime_error{message}; }
}
int main() try {
  using namespace apache;
  resources::model_archive models(1); models[0].emplace();
  render::model_instance actor;
  actor.relative.position = actor.world.position = {1 << 22, 2 << 22, 100};
  actor.children.push_back(std::make_unique<render::model_instance>());
  actor.children.front()->relative.position = {100, 200, 300};
  render::maintain_model(actor);
  render::mobile_scene scene{models};
  auto const id{scene.register_object(actor)};
  require(&scene.at(id) == &actor && scene.sector(66).size() == 1, "registered actor and scene share model state");
  for(unsigned int i{0}; i != 500; ++i) scene.add(0, {{0, 0, 0}, math::identity});
  require(&scene.at(id) == &actor, "scene storage growth does not move registered actors");
  actor.status = 4;
  actor.flags = 17; actor.children.front()->flags = 123;
  actor.world.position = {3 << 22, 4 << 22, 300};
  scene.place(id);
  require(scene.sector(66).empty() && scene.sector(196).front() == id && scene.at(id).flags == 17 && scene.at(id).children.front()->flags == 123, "live movement and animation reach the scene");
  scene.unplace(id);
  require(scene.contains(id) && scene.sector(196).empty(), "destroyed actor can leave sector lists while target identity remains valid");
  scene.place(id); scene.remove(id);
  require(!scene.contains(id) && actor.children.front()->flags == 123, "removing a registered actor does not delete its model");
  render::model_instance attached;
  render::synchronize_model(attached, actor);
  require(attached.flags == 17 && attached.status == 4, "animation and combat status remain independent through synchronization");
  auto *const child{attached.children.front().get()};
  actor.relative.position = {500, 600, 700};
  actor.children.front()->relative.position = {700, 800, 900};
  actor.children.front()->flags = 456;
  actor.children.push_back(std::make_unique<render::model_instance>());
  math::pose const carrier{{10000, 20000, 30000}, math::identity};
  render::maintain_model(actor, &carrier);
  render::synchronize_model(attached, actor);
  require(attached.children.front().get() == child && child->flags == 456 && child->world.position == actor.children.front()->world.position && attached.world.position == actor.world.position, "carrier synchronization retains allocations and updates all animated child transforms");
  actor.children.front().reset(); actor.children.back()->id = 7;
  render::synchronize_model(attached, actor);
  require(!attached.children.front() && attached.children.back()->id == 7, "attachment removals and replacements propagate");
  scene.clear();
  std::cout << "Shared scene ownership and attachment checks passed\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
