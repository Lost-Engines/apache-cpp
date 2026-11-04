#pragma once
#include "effect_renderer.h"
#include "model_renderer.h"
#include "simulation/craters.h"

namespace apache::render {
void submit_craters(draw_queue &queue, simulation::craters const &craters, math::pose const &camera, effect_visibility const &visibility);
model_instance crater_model(simulation::crater const &crater, math::trigonometry const &trig, int32_t distance);
} // namespace apache::render
