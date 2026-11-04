#pragma once
#include "effect_renderer.h"
#include "model_renderer.h"
#include "simulation/fragments.h"

namespace apache::render {
void fracture_model(model_instance &object, simulation::fragments &fragments, uint32_t &random_seed);
void submit_fragments(draw_queue &queue, simulation::fragments const &fragments, math::pose const &camera, effect_visibility const &visibility);
model_instance fragment_model(simulation::fragment const &fragment, resources::model_archive const &models, int32_t distance);
} // namespace apache::render
