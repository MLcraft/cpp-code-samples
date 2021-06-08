// Header
#include "background.hpp"
#include "render.hpp"

entt::entity Background::createBackground(vec2 position, std::string name)
{
	auto entity = _ecs.create();
    

	// Create rendering primitives
	std::string key = name;
	ShadedMesh& resource = cache_resource(key);
	if (resource.effect.program.resource == 0)
		RenderSystem::createSprite(resource, textures_path(key + ".png"), "textured");

	// Store a reference to the potentially re-used mesh object (the value is
	// stored in the resource cache)
	_ecs.emplace<ShadedMeshRef>(entity, resource);

	// Initialize the motion
	auto& motion = _ecs.emplace<Motion>(entity);
	motion.angle = 0.f;
	motion.velocity = {0.f, 0.f}; // 200
	motion.position = position;
	motion.depth = 900;
	// Setting initial values
	motion.scale =
		vec2({1.f, 1.f}) * static_cast<vec2>(resource.texture.size);
	// Create and (empty) Turtle component to be able to refer to all turtles
	auto& background = _ecs.emplace<Background>(entity);

	background.type = name;
	return entity;
}
