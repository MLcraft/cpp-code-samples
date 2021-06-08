// internal
#include "ai.hpp"
#include "tiny_ecs.hpp"
#include "salmon.hpp"
#include "fish.hpp"

void AISystem::step(float elapsed_ms, vec2 window_size_in_game_units)
{
	auto player_salmon = ECS::registry<Salmon>.entities[0];
	auto& motion_container = ECS::registry<Fish>;
	for (unsigned int i = 0; i < motion_container.components.size(); i++) {
		ECS::Entity fish_i = motion_container.entities[i];
		Motion& motion_i = ECS::registry<Motion>.get(fish_i);
		FishAI& ai_i = ECS::registry<FishAI>.get(fish_i);
		Motion& salmon_motion = ECS::registry<Motion>.get(player_salmon);
		ai_i.last_check += elapsed_ms;
		if (ai_i.last_check >= check_rate) {
			ai_i.last_check = 0.f;
			ai_i.distance = sqrtf(powf((motion_i.position.x - salmon_motion.position.x), 2) + powf((motion_i.position.y - salmon_motion.position.y), 2));
			// std::cout << ai_i.distance << '\n';
			if (ai_i.distance < check_e) {
				motion_i.velocity.x = 0.f;
				if ((motion_i.position.y - salmon_motion.position.y) > 0) {
					motion_i.velocity.y = 100.f;
				}
				else {
					motion_i.velocity.y = -100.f;
				}
				ai_i.avoiding = true;
			}
			else {
				if (ai_i.avoiding) {
					motion_i.velocity.y = 0.f;
					motion_i.velocity.x = -100.f;
					ai_i.avoiding = false;
				}
			}
		}
	}
}
