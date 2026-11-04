#include <charconv>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "frontend/mission_frontend.h"
#include "audio/engine.h"
#include "platform/audio_output.h"
#include "frontend/mission_session.h"
#include "platform/key_map.h"
#include "platform/window.h"
#include "resources/data_store.h"

namespace {
unsigned int number(std::string_view const text) {
  unsigned int result{0};
  auto const [end, error]{std::from_chars(text.data(), text.data() + text.size(), result)};
  if(error != std::errc{} || end != text.data() + text.size()) throw std::runtime_error{"Expected a non-negative integer"};
  return result;
}
struct events {
  struct key { int value, modifiers; bool down; bool character{false}, repeat{false}; };
  std::vector<key> keys;
  struct click { unsigned int button; bool down; };
  std::vector<click> clicks;
};
} // namespace

int main(int const argc, char const *const argv[]) try {
  std::filesystem::path directory{"data"};
  auto save_directory{apache::configuration::default_save_directory()};
  std::string selected_menu{"_main_screen"};
  std::string capture;
  std::string pressed;
  unsigned int scale{0}, frame_limit{0};
  bool no_audio{false};
  bool quick{false}, override_model{false}, override_detail{false}, override_video{false};
  int mission_id{0};
  apache::frontend::mission_options mission_options;
  for(int i{1}; i < argc; ++i) {
    std::string_view const key{argv[i]};
    auto const value{[&]()->std::string_view{
      if(++i == argc) throw std::runtime_error{"Missing value for " + std::string{key}};
      return argv[i];
    }};
    if(key == "--help") {
      std::cout << "Apache C++ reconstruction\n"
                   "Usage: apache [--data-dir DIRECTORY] [--scale N]\n"
                   "  --data-dir DIRECTORY  Original installation or extracted CD contents\n"
                   "  --save-dir DIRECTORY  Pilot and preference storage\n"
                   "  --scale N             Integer scale, or 0 for automatic sizing\n"
                   "  --no-audio            Disable the playback device\n"
                   "Development checks:\n"
                   "  --menu NAME           Select an original frontend screen\n"
                   "  --capture FILE.ppm    Render one menu frame without opening a window\n"
                   "  --press NAME          Show a pressed menu button\n"
                   "  --quick               Show the Quickstart choices\n"
                   "  --mission ID          Start a mission directly\n"
                   "  --realistic           Use the realistic flight model\n"
                   "  --video-mode N        Original 320 (0) or 640 (1) mode\n"
                   "  --detail N            Original detail level, 0 to 9\n"
                   "  --frames N            Close after N window frames\n";
      return EXIT_SUCCESS;
    }
    if(key == "--data-dir") directory = value();
    else if(key == "--save-dir") save_directory = value();
    else if(key == "--scale") {
      scale = number(value());
      if(scale > 16) throw std::runtime_error{"Window scale must be between 0 and 16"};
    } else if(key == "--frames") frame_limit = number(value());
    else if(key == "--menu") selected_menu = "_" + std::string{value()} + "_screen";
    else if(key == "--capture") capture = value();
    else if(key == "--press") pressed = value();
    else if(key == "--no-audio") no_audio = true;
    else if(key == "--quick") quick = true;
    else if(key == "--mission") mission_id = static_cast<int>(number(value()));
    else if(key == "--realistic") { mission_options.arcade = false; override_model = true; }
    else if(key == "--detail") { mission_options.detail = number(value()); override_detail = true; }
    else if(key == "--video-mode") { mission_options.video_mode = number(value()); override_video = true; }
    else throw std::runtime_error{"Unknown option: " + std::string{key}};
  }
  apache::resources::data_store const data{directory};
  std::unique_ptr<apache::frontend::mission_session> mission;
  if(mission_id && !capture.empty()) mission = std::make_unique<apache::frontend::mission_session>(data, mission_id, mission_options);
  apache::frontend::mission_frontend frontend{data, mission_options};
  auto &menu{frontend.interface};
  bool launched_from_menu{false};
  menu.show(selected_menu);
  menu.toggle_quick(quick);
  if(!pressed.empty()) menu.press_named(pressed);
  apache::render::framebuffer frame{640, 480};
  menu.render(frame);
  if(!capture.empty()) {
    if(mission) { for(unsigned int i{0}; i != std::max(frame_limit, 1u); ++i) { mission->step(10); mission->draw(); } }
    apache::resources::write_ppm(capture, mission ? mission->frame.image : frame.image);
    return EXIT_SUCCESS;
  }
  auto const data_root{std::filesystem::weakly_canonical(data.directory())};
  auto const save_root{std::filesystem::weakly_canonical(save_directory)};
  auto const relative{save_root.lexically_relative(data_root)};
  if(relative.empty() || (!relative.empty() && *relative.begin() != "..")) throw std::runtime_error{"Save directory must be outside the original game data"};
  frontend.load_profiles(save_directory);
  if(override_model) frontend.options.arcade = mission_options.arcade;
  if(override_detail) frontend.options.detail = mission_options.detail;
  if(override_video) frontend.options.video_mode = mission_options.video_mode;
  if(mission_id) mission = std::make_unique<apache::frontend::mission_session>(data, mission_id, frontend.options);
  if(!mission_id && selected_menu == "_main_screen" && !quick && pressed.empty()) frontend.startup();
  apache::platform::window window{scale};
  std::unique_ptr<apache::audio::engine> sound;
  std::unique_ptr<apache::platform::audio_output> audio_device;
  if(!no_audio) {
    sound = std::make_unique<apache::audio::engine>(data);
    sound->configure(frontend.settings);
    try {
      audio_device = std::make_unique<apache::platform::audio_output>([](void *context, std::span<float> buffer) noexcept {
        try { static_cast<apache::audio::engine*>(context)->render(buffer); }
        catch(...) { std::ranges::fill(buffer, 0); }
      }, sound.get());
    } catch(std::exception const &error) { std::cerr << "Audio unavailable: " << error.what() << '\n'; sound.reset(); }
  }
  if(sound) {
    frontend.speak = [&](int kind, int flow, int zone, int level, int id){ sound->speech(kind, flow, zone, level, id); };
    frontend.stop_speech = [&](){ sound->stop_speech(); };
    frontend.speech_playing = [&](){ return sound->speaking(); };
  }
  auto const connect_audio{[&](){
    if(!sound || !mission) return;
    sound->mission(true);
    mission->play_sound = [&](auto id, auto volume, auto pan){ sound->play(id, volume, pan); };
    mission->flight_sound = [&](auto event){ sound->flight(event); };
    mission->runtime.events.sound = [&](auto id, bool cockpit){ sound->voice(id, cockpit); };
    mission->runtime.events.launch_sound = [&](auto type){ sound->play(type + 8, 30); };
    mission->runtime.events.incoming_sound = [&](){ sound->play(14, 30); };
    mission->runtime.events.gun_sound = [&](bool active){ sound->gun(active); };
  }};
  connect_audio();
  events input;
  glfwSetWindowUserPointer(window.get(), &input);
  glfwSetKeyCallback(window.get(), [](GLFWwindow *const handle, int const key, int, int const action, int const modifiers){
    static_cast<events*>(glfwGetWindowUserPointer(handle))->keys.push_back({key, modifiers, action != GLFW_RELEASE, false, action == GLFW_REPEAT});
  });
  glfwSetCharCallback(window.get(), [](GLFWwindow *handle, unsigned int value){ static_cast<events*>(glfwGetWindowUserPointer(handle))->keys.push_back({static_cast<int>(value), 0, true, true}); });
  glfwSetMouseButtonCallback(window.get(), [](GLFWwindow *const handle, int const button, int const action, int){
    if(button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) static_cast<events*>(glfwGetWindowUserPointer(handle))->clicks.push_back({button == GLFW_MOUSE_BUTTON_LEFT ? 0u : 1u, action == GLFW_PRESS});
  });
  unsigned int frames{0};
  auto previous_time{std::chrono::steady_clock::now()};
  double tick_fraction{0};
  if(mission) { mission->step(10); mission->draw(); window.title("Apache C++ | " + mission->briefing.title); }
  while(window.poll() && !menu.quit_requested()) {
    if(mission) {
      for(auto const &key : input.keys) if(!key.character && !key.repeat) if(auto scan{apache::platform::scan_code(key.value, key.modifiers)}) mission->key(*scan, key.down);
      input.keys.clear(); input.clicks.clear();
      if(!window.focused()) { mission->controls.held.fill(false); mission->controls.counts.fill(0); }
      if(mission->controls.configuration.input_type != 1) {
        int count{0}; auto const *axes{glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count)};
        for(size_t i{0}; i != mission->controls.joystick.size(); ++i) mission->controls.joystick[i].value = axes && i < static_cast<size_t>(count) ? static_cast<int32_t>(std::lround((std::clamp(axes[i], -1.0f, 1.0f) + 1.0f) * 127.5f)) : 128;
      }
      auto const now{std::chrono::steady_clock::now()};
      tick_fraction += std::chrono::duration<double>(now - previous_time).count() * 200; previous_time = now;
      auto const ticks{static_cast<unsigned int>(std::min(tick_fraction, 200.0))};
      if(ticks) { tick_fraction -= ticks; mission->step(ticks); mission->draw(); }
      if(sound) sound->pause(mission->paused);
      window.present(mission->frame);
      if(mission->finished) {
        if(sound) sound->mission(false);
        if(launched_from_menu) frontend.complete(std::move(mission));
        else { mission.reset(); menu.show("_main_screen"); }
        window.title("Apache C++");
      }
      if(frame_limit && ++frames == frame_limit) break;
      continue;
    }
    auto const now{std::chrono::steady_clock::now()};
    frontend.interface.tick(std::chrono::duration<double>(now - previous_time).count());
    frontend.planner.tick(std::chrono::duration<double>(now - previous_time).count());
    previous_time = now; tick_fraction = 0;
    auto const cursor{window.cursor_position()};
    menu.move(cursor ? static_cast<int>(cursor->x) : -1, cursor ? static_cast<int>(cursor->y) : -1);
    for(auto const &click : input.clicks) menu.button(click.down, click.button);
    input.clicks.clear();
    for(auto const &key : input.keys) {
      if(!key.down) continue;
      if(key.character) { frontend.text_input(static_cast<unsigned int>(key.value)); continue; }
      if((key.value == GLFW_KEY_ESCAPE || key.value == GLFW_KEY_ENTER || key.value == GLFW_KEY_BACKSPACE) && frontend.text_input(key.value == GLFW_KEY_ESCAPE ? 27 : key.value == GLFW_KEY_ENTER ? 10 : 8)) continue;
      if(key.value == GLFW_KEY_ESCAPE) frontend.back();
      else if(key.value == GLFW_KEY_F1) menu.show("_help_screen");
    }
    input.keys.clear();
    if(menu.current() == "_preferences_screen") {
      int count{0}, hats{0}; auto const *axes{glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count)}; auto const *hat{glfwGetJoystickHats(GLFW_JOYSTICK_1, &hats)};
      std::array<int32_t, 4> values{128, 128, 128, 128};
      for(size_t i{0}; i != values.size(); ++i) if(axes && i < static_cast<size_t>(count)) values[i] = static_cast<int32_t>(std::lround((std::clamp(axes[i], -1.0f, 1.0f) + 1.0f) * 127.5f));
      frontend.preferences.calibration(values, hat && hats ? *hat : 0u);
    }
    if(sound) sound->configure(frontend.settings);
    menu.render(frame);
    window.present(frame);
    if(auto launched{frontend.takeoff()}) {
      mission = std::move(launched); launched_from_menu = true; connect_audio();
      mission->step(10); mission->draw(); window.title("Apache C++ | " + mission->briefing.title);
      previous_time = std::chrono::steady_clock::now(); tick_fraction = 0;
    }
    if(frame_limit && ++frames == frame_limit) break;
  }
  frontend.logs.save(); frontend.settings.save();
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << "ERROR: " << error.what() << std::endl;
  return EXIT_FAILURE;
}
