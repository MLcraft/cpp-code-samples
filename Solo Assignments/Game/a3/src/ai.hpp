#pragma once

#include <vector>

#include "common.hpp"
#include "tiny_ecs.hpp"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DON'T WORRY ABOUT THIS CLASS UNTIL ASSIGNMENT 3
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

const size_t check_e = 200;
const size_t check_rate = 500;

struct FishAI
{
	float distance = 0.f;
	float last_check = 0.f;
	bool avoiding = false;
};

class AISystem
{
public:
	void step(float elapsed_ms, vec2 window_size_in_game_units);
};
