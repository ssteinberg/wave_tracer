/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#pragma once

#ifdef GLM_VERSION_MAJOR
#error "include <wt/math/glm.hpp> before GLM headers"
#endif

#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_SIZE_T_LENGTH
#define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/reciprocal.hpp>
#include <glm/gtc/ulp.hpp>
