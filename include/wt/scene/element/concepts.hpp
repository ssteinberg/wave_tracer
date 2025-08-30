/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#include <concepts>

namespace wt::scene {

class scene_element_t;

/**
 * @brief Concept satisfied if and only if T is derived from scene_element_t
 */
template <typename T>
concept SceneElement = std::derived_from<T,scene::scene_element_t>;

}
