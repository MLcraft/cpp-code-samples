#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

enum mode {BASIC, ADVANCED};

struct Salmon
{
	mode current_mode = BASIC;
	float last_angle = 0;
	// Creates all the associated render resources and default transform
	static ECS::Entity createSalmon(vec2 pos);
};
