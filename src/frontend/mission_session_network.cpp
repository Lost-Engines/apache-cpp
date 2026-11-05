#include "mission_session.h"

namespace apache::frontend {
void mission_session::initialise_remote_players() {
  if(!network || crew) return;
  auto const mode{network->lifecycle.mode};
  for(uint32_t i{0}; i < network->state.players.size(); ++i) {
    auto &entry{network->state.players[i]};
    if(i == network->state.local || ((entry.flags >> 8) & 255) == 255) continue;
    auto model{entry.aircraft.model}; size_t task{0};
    if(model == 64 || model == 103) model = 64;
    else if(model == 222 || model == 607) { model = 990; task = 1; }
    if(network->assign(i, static_cast<uint32_t>(model), task, mode, briefing.flow, static_cast<unsigned int>(briefing.zone))) {
      // initialise_network_users_ starts the received attitude at identity.
      entry.aircraft.location.rotation = math::identity;
    }
  }
  if(mode == multiplayer::game_mode::combat || mode == multiplayer::game_mode::multiplayer || mode == multiplayer::game_mode::capture_the_flag) network->remove_unassigned();
}
void mission_session::send_frame() {
  auto &state{network->state}; auto const &readings{runtime.pilot->readings()};
  if(state.client_server) {
    if(state.local == state.server) { if(poll_network) poll_network(); }
    else if(state.server_in_game && state.new_server_data && !state.dead && network->send) {
      network->send(multiplayer::client_state(state.local, readings.position, readings.airspeed, frame_time - network_wait_ticks / 200));
      state.new_server_data = false;
    }
  } else {
    network->send_casualties();
    if(network->send) network->send(multiplayer::position(state.local, readings.position, readings.airspeed, network->identify(runtime.sensors->state.current)));
    runtime.random.global_seed = 1;
  }
}
void mission_session::send_server_frame() {
  if(!network || !network->state.client_server || network->state.server != network->state.local || !network->send) return;
  auto &state{network->state}; auto const &readings{runtime.pilot->readings()};
  auto &local{state.players[state.local].aircraft};
  local.location = readings.position; local.speed = readings.airspeed; local.model = static_cast<int32_t>(runtime.actors.units[*runtime.timing.player].object.id);
  std::array<multiplayer::aircraft_state, 16> players;
  for(size_t i{0}; i < players.size(); ++i) players[i] = state.players[i].aircraft;
  network->send(multiplayer::server_state(players));
  for(auto &player : state.players) player.flags &= ~0x40000u;
}
} // namespace apache::frontend
