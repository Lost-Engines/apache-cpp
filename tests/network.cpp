#include "multiplayer/packet.h"
#include "multiplayer/crew.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, char const *message) { if(!condition) throw std::runtime_error{message}; }
}
int main() try {
  using namespace apache::multiplayer;
  std::array<aircraft_state, 16> players;
  auto &first{players[0]}; first.location.position = {0x1234567, -1, 0xfedcba9};
  first.location.rotation = {INT32_MAX, 0, 0, 0, -INT32_MAX, 0, 0, 0, INT32_MAX}; first.speed = -12345; first.model = -1;
  auto const state{server_state(players)};
  // Captured from send_server_data_ and apnet_receive_ in the reference engine.
  std::array<uint8_t, 36> const original{0x1e,0,0,0,0x56,0x34,0x12,0xff,0xff,0xff,0xba,0xdc,0xfe,0xc7,0xcf,0xff,0xff,0xff,0xff,0x7f,0,0,0,0,0,0,0,0x80,0,0,0,0,0,0,0xff,0x7f};
  require(std::ranges::equal(state.bytes().first(36), original), "original compressed aircraft record");
  auto const decoded{server_state(state)[0]};
  require(decoded.location.position == apache::math::vector3{19088736, 268435440, 267242400}, "original 24-bit coordinate decoding");
  require(decoded.location.rotation == apache::math::matrix3{2147418112,0,0,0,-2147418113,0,0,0,2147418112}, "original orientation normalization");
  require(decoded.speed == -12345 && decoded.model == -1, "signed speed and model decoding");
  for(size_t size{0}; size < state.bytes().size(); ++size) require(!packet::decode(state.bytes().first(size)), "truncated server packet rejected");
  packet message{message_type::identity, 3};
  for(size_t size{0}; size < message.bytes().size(); ++size) require(!packet::decode(message.bytes().first(size)), "truncated message rejected");
  message.word(16, 16); require(!packet::decode(message.bytes()), "out-of-range player rejected");
  message.word(16, 3); message.word(0, 0xffffffff); require(!packet::decode(message.bytes()), "unknown message rejected");
  auto options{two_player_settings({3, 1, 2, 1, 0})};
  require(options.bytes().size() == 24 && packet::decode(options.bytes()).has_value(), "short two-player options packet");
  auto batch{kills(0, "Pilot", false, 42, std::array<uint8_t, 252>{})};
  require(packet::decode(batch.bytes()).has_value(), "full mobile casualty batch");
  batch.word(24, 43); require(!packet::decode(batch.bytes()), "oversized mobile casualty count rejected");
  batch = kills(0, "Pilot", true, 28, std::array<uint8_t, 336>{});
  batch.word(24, 29); require(!packet::decode(batch.bytes()), "oversized scenery casualty count rejected");
  crew_state shared;
  for(unsigned int i{0}; i < 10; ++i) require(!shared.notify_key(static_cast<uint16_t>(i + 1)), "ten distinct crew actions accepted");
  require(shared.notify_key(1) && shared.notify_key(11) && shared.keys.size() == 10, "duplicate/full crew queue suppressed");
  shared.message = 10; shared.pilot.landing = 3; shared.pilot.fuel = 2442; shared.pilot.weight = 16000;
  auto payload{encode_crew(crew_role::pilot, shared)};
  require(shared.message == 0 && shared.pilot.landing == 0 && shared.keys.empty(), "one-shot crew events consumed");
  auto crew_value{decode_crew(crew_role::pilot, payload)};
  require(crew_value && crew_value->keys.size() == 10 && crew_value->message == 10 && crew_value->pilot.landing == 3, "crew events preserved on receive");
  for(size_t size{0}; size < payload.size(); ++size) require(!decode_crew(crew_role::pilot, std::span{payload}.first(size)), "truncated crew record rejected");
  payload[44] = 11; require(!decode_crew(crew_role::pilot, payload), "crew key count cannot overrun record");
  payload[44] = 10; payload[47] = 11; require(!decode_crew(crew_role::pilot, payload), "unknown radio message rejected");
  payload[47] = 10; payload[76] = 3; require(!decode_crew(crew_role::gunner, payload), "unknown gunner target type rejected");
  require(!decode_crew(static_cast<crew_role>(2), payload), "single-player role rejected for shared cockpit");
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
