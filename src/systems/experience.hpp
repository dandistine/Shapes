#pragma once

#include "components.hpp"
#include "system.hpp"

struct ExperienceSystem : public System {
	ExperienceSystem(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

	// Pickup experience touching the player
	// Pull experience towards the player
	void OnUserUpdate(float fElapsedTime) override {
		const auto& player_shape = reg.get<Shape>(player_entity);
		auto& player_component = reg.get<PlayerComponent>(player_entity);
		auto view = reg.view<ExperienceComponent, Shape, PhysicsComponent>();

		float xp_range2 = player_component.experience_range * player_component.experience_range;
		for(auto entity : view) {
			const auto& s = view.get<Shape>(entity);
			auto& e = view.get<ExperienceComponent>(entity);
			auto& p = view.get<PhysicsComponent>(entity);
			
			e.age += fElapsedTime;
			
			// If the player is touching an experience, pick it up
			if(s.intersects(player_shape)) {
				const auto& xp = view.get<ExperienceComponent>(entity);
				player_component.experience += xp.value;
				reg.destroy(entity);

				dispatcher.enqueue<PlayRandomEffect>({"experience"});
				continue;
			}

			// If the player is somewhat close to an experience, pull it in
			olc::vf2d distance = s.position - player_shape.position;
			float mag2 = distance.mag2();
			if(mag2 < xp_range2) {
				p.force += distance.norm() * (mag2 - xp_range2) * fElapsedTime * 100.0f;
			}
		}
	}

private:
	entt::dispatcher& dispatcher;
	entt::entity player_entity;
};