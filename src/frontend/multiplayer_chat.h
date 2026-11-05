#pragma once
#include <functional>
#include "render/instrument_font.h"

namespace apache::frontend {
class multiplayer_chat {
  render::instrument_font font;
  std::array<uint8_t, 256> ascii{};
  struct command_info { std::string name; unsigned int arguments; bool listed; std::array<std::string, 3> help; };
  std::array<command_info, 8> commands;
  std::array<std::string, 9> languages;
  std::string version;
  std::array<std::array<uint8_t, 96>, 2> terrain_presets{};
  void terrain_command(std::array<std::string, 4> const &arguments, int32_t now);
  void scroll();
public:
  bool writing{false}, empty{true};
  std::array<char, 100> entry{}, previous{};
  unsigned int cursor{0}, previous_cursor{0};
  unsigned int lines{5}, scroll_seconds{5}, blank_seconds{30}, shadow{1}, colour{1}, language{1};
  int32_t last_message{0}, scroll_time{0};
  std::array<std::string, 24> messages;
  std::function<void(std::string_view)> send;
  std::function<void(unsigned int)> change_language;
  std::array<uint8_t, 96> terrain_colours{};
  std::function<void(std::array<uint8_t, 96> const &)> change_terrain;
  explicit multiplayer_chat(resources::executable_data const &executable);
  void begin();
  void key(uint16_t scan, int32_t now);
  void add(std::string_view text, int32_t now);
  void command(std::string_view text, int32_t now);
  void draw(render::framebuffer &frame, int32_t now);
  std::string_view text() const;
};
} // namespace apache::frontend
