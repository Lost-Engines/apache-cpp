#include "inspect_menu.h"
#include "frontend/mission_frontend.h"
#include <iomanip>
#include <istream>
#include <ostream>
#include <stdexcept>

void inspect_menu(apache::resources::data_store const &data, std::istream &input, std::ostream &output) {
  apache::frontend::menu menu{data};
  apache::render::framebuffer frame{640, 480};
  std::string command;
  while(input >> command) {
    std::string name; input >> std::quoted(name);
    if(command == "screen") menu.show(name);
    else if(command == "draw") {
      menu.render(frame);
      if(name != "-") apache::resources::write_ppm(name, frame.image);
      output << apache::resources::fingerprint(frame.image.pixels) << '\n';
    } else {
      auto &object{menu.named(name)};
      if(command == "text") input >> std::quoted(object.text);
      else if(command == "range") { for(auto &value : object.range) input >> value; }
      else if(command == "on") input >> object.on;
      else if(command == "picture") input >> std::quoted(object.picture);
      else if(command == "selection") input >> object.selected_line >> object.start_line;
      else if(command == "visible") { bool visible; input >> visible; object.flags = (object.flags & ~1u) | static_cast<unsigned int>(visible); }
      else throw std::runtime_error{"Unknown menu inspection command"};
    }
    if(!input) throw std::runtime_error{"Incomplete menu inspection command"};
  }
}

void inspect_frontend(apache::resources::data_store const &data, std::istream &input, std::ostream &output) {
  auto owner{std::make_unique<apache::frontend::mission_frontend>(data)};
  auto &app{*owner}; auto &menu{app.interface};
  apache::render::framebuffer frame{640, 480};
  std::unique_ptr<apache::frontend::mission_session> mission;
  menu.render(frame);
  std::string command;
  while(input >> command) {
    if(command == "click") {
      int x, y; input >> x >> y; menu.move(x, y); menu.button(true); menu.button(false); menu.move(-1, -1);
    } else if(command == "back") app.back();
    else if(command == "tick") {
      unsigned int ticks; input >> ticks;
      if(!mission) throw std::runtime_error{"No mission running"};
      mission->step(ticks); mission->draw();
    } else if(command == "end") {
      if(!mission) throw std::runtime_error{"No mission running"};
      mission->post(0x2001); mission->step(1); app.complete(std::move(mission));
    } else if(command == "draw") {
      std::string file; input >> std::quoted(file);
      auto const &image{mission ? mission->frame.image : frame.image};
      if(file != "-") apache::resources::write_ppm(file, image);
      output << "frame " << apache::resources::fingerprint(image.pixels) << '\n';
    } else if(command == "state") {
      output << std::quoted(menu.current()) << ' ' << (mission ? mission->briefing.id : 0) << ' ' << app.pilot.total_time << ' ' << app.pilot.missions.size();
      for(auto const &entry : app.pilot.missions) output << ' ' << entry.mission << ' ' << static_cast<unsigned int>(entry.result);
      output << '\n';
    } else throw std::runtime_error{"Unknown frontend inspection command"};
    if(!input) throw std::runtime_error{"Incomplete frontend inspection command"};
    menu.render(frame);
    if(auto launched{app.takeoff()}) { mission = std::move(launched); mission->step(10); mission->draw(); }
  }
}
