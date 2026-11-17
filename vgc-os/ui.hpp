#pragma once

#include <string>

#include "32blit.hpp"

#include "theme.hpp"

// buttons pressed this frame, with d-pad auto-repeat while held
uint32_t get_pressed(uint32_t time);

std::string format_size(uint32_t bytes);
