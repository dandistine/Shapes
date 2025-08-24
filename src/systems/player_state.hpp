#pragma once

#include "components.hpp"
#include "system.hpp"

struct PlayerStateSystem : public System {
	PlayerStateSystem(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		auto& p = reg.get<PlayerComponent>(player_entity);

		if(p.experience >= p.xp_to_next_level) {
			p.experience -= p.xp_to_next_level;
			p.xp_to_next_level *= 1.1f;

			// Indicate that the player has leveled up
			dispatcher.enqueue(LevelUp{});
		}
	}
private:
	entt::dispatcher& dispatcher;
	entt::entity player_entity;
};
