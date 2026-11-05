#include "mission_frontend.h"

namespace apache::frontend {
void mission_frontend::network_controls() {
  bool const remote{network.selecting() && network.remote_controlled()};
  for(auto &w : interface.objects()) {
    auto const &action{w.action};
    if(action == "notify_map_yemen_button_" || action == "notify_map_korea_button_" || action == "notify_map_cyprus_button_" || action == "notify_mission_selection_list_" || action == "notify_mission_selection_load_" || action == "notify_takeoff_button_") w.flags = (w.flags & ~0x8000u) | (remote ? 0u : 0x8000u);
  }
  visible("_world_remote_controlled_box", remote);
}
void mission_frontend::start_network() {
  if(!network.peers) return;
  network.peers->freeze();
  auto choice{options}; choice.connection = network.connection();
  auto const mode{choice.connection->game.mode}; result.reset();
  if(mode == multiplayer::game_mode::combat || mode == multiplayer::game_mode::multiplayer || mode == multiplayer::game_mode::capture_the_flag) {
    resources::mission_briefing briefing; briefing.zone = zone = 2;
    briefing.flow = flow = mode == multiplayer::game_mode::capture_the_flag ? 6 : 5;
    briefing.title = flow == 6 ? "Capture the Flag" : "Combat";
    prepared = std::make_unique<mission_session>(data, std::move(briefing), choice); departing = true;
  } else {
    flow = 3; interface.show("_world_screen"); network_controls();
  }
}
void mission_frontend::receive_selection(unsigned int const code, int const value) {
  if(!network.selecting()) return;
  if(code >= 13 && code <= 15) { zone = static_cast<int>(code - 12); preferences.zone = zone; list(); }
  else if(code == 17) select(value);
  else if(code == 16 && selected >= 0) prepare(definitions[listed[static_cast<size_t>(selected)]].id);
  else if(code == 18) departing = prepared != nullptr;
}
void mission_frontend::leave_network() {
  if(network.peers) network.send_selection(19);
  network.close(); planner.close(); prepared.reset(); result.reset(); departing = false;
  network_controls(); interface.show("_flight_screen");
}
void mission_frontend::poll_network(uint32_t const ticks) {
  if(!network.peers) return;
  network.poll(ticks);
  if(network.peers && !network.peers->room.active) {
    leave_network(); interface.popup({{"Other player left the game", 0}, {"OK", 0}}, [](int){});
  }
}
} // namespace apache::frontend
