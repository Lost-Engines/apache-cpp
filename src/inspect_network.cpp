#include "inspect_network.h"
#include "multiplayer/packet.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace apache {
namespace {
resources::bytes hex_bytes(std::string const &hex) {
  if(hex.size() % 2) throw std::runtime_error{"Odd hex packet length"};
  resources::bytes result;
  for(size_t i{0}; i < hex.size(); i += 2) result.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
  return result;
}
}
void inspect_network(std::istream &input, std::ostream &output) {
  std::string line;
  while(std::getline(input, line)) {
    if(line.empty()) continue;
    std::istringstream in{line}; std::string command; in >> command;
    if(command == "unpack" || command == "valid") {
      std::string hex; in >> hex; auto const message{multiplayer::packet::decode(hex_bytes(hex))};
      if(command == "valid") { output << static_cast<int>(message.has_value()) << '\n'; continue; }
      if(!message) throw std::runtime_error{"Invalid packet"};
      for(auto const &player : multiplayer::server_state(*message)) {
        for(auto v : player.location.position) output << v << ' ';
        for(auto v : player.location.rotation) output << v << ' ';
        output << player.speed << ' ' << player.model << ' ';
      }
      output << '\n'; continue;
    }
    multiplayer::packet result{multiplayer::message_type::identity, 0};
    if(command == "server") {
      std::array<multiplayer::aircraft_state, 16> players;
      for(auto &player : players) {
        for(auto &v : player.location.position) in >> v;
        for(auto &v : player.location.rotation) in >> v;
        in >> player.speed >> player.model;
      }
      result = multiplayer::server_state(players);
    } else {
      uint32_t sender{0}; std::string name; in >> sender >> std::quoted(name);
      if(command == "simple") { uint32_t type; in >> type; result = {static_cast<multiplayer::message_type>(type), sender, name}; }
      else if(command == "nomination") { double time; in >> time; result = multiplayer::nomination(sender, name, time); }
      else if(command == "settings") { std::array<int32_t, 5> values; for(auto &v : values) in >> v; result = multiplayer::two_player_settings(values); }
      else if(command == "chat") { std::string text; in >> std::quoted(text); result = multiplayer::chat(sender, name, text); }
      else if(command == "crew") { std::string hex; in >> hex; auto const data{hex_bytes(hex)}; if(data.size() != 104) throw std::runtime_error{"Wrong crew payload size"}; result = multiplayer::crew(sender, name, std::span<uint8_t const, 104>{data.data(), 104}); }
      else if(command == "kills") { bool scenery; unsigned int count; std::string hex; in >> scenery >> count >> hex; auto const data{hex_bytes(hex)}; result = multiplayer::kills(sender, name, scenery, static_cast<uint8_t>(count), data); }
      else if(command == "flag") result = multiplayer::flag_won(sender, name);
      else if(command == "preferences") { std::array<int32_t, 5> values; for(auto &v : values) in >> v; result = multiplayer::preferences(sender, values); }
      else if(command == "weather") { int32_t strength, direction; in >> strength >> direction; result = multiplayer::weather(sender, name, strength, direction); }
      else if(command == "peer") { uint32_t type, model; in >> type >> model; result = multiplayer::identification(static_cast<multiplayer::message_type>(type), sender, name, static_cast<int32_t>(model)); }
      else if(command == "pose") {
        uint32_t type; math::pose value; int32_t speed; multiplayer::target_state target; double delta;
        in >> type; for(auto &v : value.position) in >> v; for(auto &v : value.rotation) in >> v;
        in >> speed >> target.kind; for(auto &v : target.data) in >> v; in >> delta;
        if(type == 12) result = multiplayer::position(sender, value, speed, target);
        else if(type == 20) result = multiplayer::restarted(sender, name, value);
        else result = multiplayer::client_state(sender, value, speed, delta);
      } else if(command == "scores") { std::array<int32_t, 16> values; for(auto &v : values) in >> v; result = multiplayer::scores(sender, name, values); }
      else if(command == "event") { uint32_t type; int32_t first, second; in >> type >> first >> second; result = multiplayer::event(static_cast<multiplayer::message_type>(type), sender, name, first, second); }
      else if(command == "menu") { uint32_t selected; std::array<int32_t, 5> values; in >> selected; for(auto &v : values) in >> v; result = multiplayer::menu_selection(sender, name, selected, values); }
      else if(command == "initial") { math::vector3 position; uint32_t war, weapon; for(auto &v : position) in >> v; in >> war >> weapon; result = multiplayer::initial_state(sender, name, position, war, weapon); }
      else if(command == "fire") { uint32_t weapon, mount; multiplayer::target_state target; math::vector3 gun; in >> weapon >> mount >> target.kind; for(auto &v : target.data) in >> v; for(auto &v : gun) in >> v; result = multiplayer::fired(sender, name, weapon, mount, target, gun); }
      else throw std::runtime_error{"Unknown network inspection command: " + command};
    }
    if(!in) throw std::runtime_error{"Incomplete network inspection command"};
    for(auto const b : result.bytes()) output << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(b);
    output << std::dec << '\n';
  }
}
} // namespace apache
