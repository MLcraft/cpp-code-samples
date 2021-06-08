// internal
#include "physics.hpp"
#include "tiny_ecs.hpp"
#include "debug.hpp"
#include "ai.hpp"
#include "salmon.hpp"
#include "fish.hpp"
#include "pebbles.hpp"
#include "turtle.hpp"

#include <chrono>
#include <iostream>

using Clock = std::chrono::high_resolution_clock;

// Returns the local bounding coordinates scaled by the current size of the entity 
vec2 get_bounding_box(const Motion& motion)
{
	// fabs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You don't
// need to try to use this technique.
bool collides(const Motion& motion1, const Motion& motion2)
{
	auto dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	float other_r = std::sqrt(std::pow(get_bounding_box(motion1).x/2.0f, 2.f) + std::pow(get_bounding_box(motion1).y/2.0f, 2.f));
	float my_r = std::sqrt(std::pow(get_bounding_box(motion2).x/2.0f, 2.f) + std::pow(get_bounding_box(motion2).y/2.0f, 2.f));
	float r = max(other_r, my_r);
	if (dist_squared < r * r)
		return true;
	return false;
}

void PhysicsSystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	// Move entities based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.

	for (auto& motion : ECS::registry<Motion>.components)
	{
		float step_seconds = 1.0f * (elapsed_ms / 1000.f);
		
		// !!! TODO A1: uncomment block and update motion.position based on step_seconds and motion.velocity
		// add step_seconds * motion.velocity to position in x and y directions
		motion.position += motion.velocity * step_seconds;

		motion.velocity += motion.deceleration * step_seconds;
		motion.velocity += motion.acceleration * step_seconds;

		if (abs(motion.deceleration.x) > abs(motion.velocity.x)) {
			motion.deceleration.x = 0;
			motion.velocity.x = 0;
		}

		if (abs(motion.deceleration.y) > abs(motion.velocity.y)) {
			motion.deceleration.y = 0;
			motion.velocity.y = 0;
		}
	}
	(void)window_size_in_game_units;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE UPDATES HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	for (auto& pebble : ECS::registry<Pebble>.entities) {
		auto& motion = ECS::registry<Motion>.get(pebble);
		motion.acceleration.y = 9.8 * motion.mass;
	}

	// Visualization for debugging the position and scale of objects
	if (DebugSystem::in_debug_mode)
	{
		for (auto& motion : ECS::registry<Motion>.components)
		{
			DebugSystem::createBox(motion.position, motion.scale);
		}
		auto player_salmon = ECS::registry<Salmon>.entities[0];
		auto& salmon_motion = ECS::registry<Motion>.get(player_salmon);
		auto& motion_container = ECS::registry<Fish>;
		for (unsigned int i = 0; i < motion_container.components.size(); i++) {
			ECS::Entity fish_i = motion_container.entities[i];
			Motion& motion_i = ECS::registry<Motion>.get(fish_i);
			FishAI& ai_i = ECS::registry<FishAI>.get(fish_i);
			if (ai_i.avoiding) {
				if (motion_i.velocity.y < 0.f) {
					DebugSystem::createLine(vec2(motion_i.position.x, motion_i.position.y - (check_e / 2)), vec2(5.f, check_e));
				}
				else {
					DebugSystem::createLine(vec2(motion_i.position.x, motion_i.position.y + (check_e / 2)), vec2(5.f, check_e));
				}
				DebugSystem::createBox(salmon_motion.position, vec2(2 * check_e, 2 * check_e));
				auto t = Clock::now();
				float freeze_milliseconds = 0;
				while (freeze_milliseconds < 2) {
					auto now = Clock::now();
					freeze_milliseconds = static_cast<float>((std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count()) / 1000.f;
					std::cout << freeze_milliseconds << '\n';
					t = now;
				}
			}
			
		}
	}

	// Check for collisions between all moving entities
	auto& motion_container = ECS::registry<Motion>;
	// for (auto [i, motion_i] : enumerate(motion_container.components)) // in c++ 17 we will be able to do this instead of the next three lines
	for (unsigned int i=0; i<motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		ECS::Entity entity_i = motion_container.entities[i];
		for (unsigned int j=i+1; j<motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			ECS::Entity entity_j = motion_container.entities[j];

			if (collides(motion_i, motion_j))
			{
				// Create a collision event
				// Note, we are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity, hence, emplace_with_duplicates
				ECS::registry<Collision>.emplace_with_duplicates(entity_i, entity_j);
				ECS::registry<Collision>.emplace_with_duplicates(entity_j, entity_i);

				if (ECS::registry<Bounce>.has(entity_i) && (ECS::registry<Bounce>.has(entity_j))) {
					motion_i.velocity.x = -motion_i.velocity.x;
					motion_j.velocity.y = -motion_i.velocity.y;
				}
			}
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE SALMON - WALL COLLISIONS HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A3: HANDLE PEBBLE COLLISIONS HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 3
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

PhysicsSystem::Collision::Collision(ECS::Entity& other)
{
	this->other = other;
}
