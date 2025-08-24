#pragma once

#include "components.hpp"
#include "shape.hpp"

#include "utilities/entt.hpp"

struct EnemyAttackSystem : public System {
	EnemyAttackSystem(entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : player_entity(player), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		const auto& view = reg.view<PhysicsComponent, Shape, EnemyComponent>();
		const auto& player_shape = reg.get<Shape>(player_entity);

		for(auto entity : view) {
			const auto& s = view.get<Shape>(entity);
			auto& e = view.get<EnemyComponent>(entity);

			e.attack_timer += fElapsedTime;
			
			if((e.attack_timer > e.attack_cooldown) && s.intersects(player_shape)) {
				const auto& dir = player_shape.position - s.position;
				auto& physics = view.get<PhysicsComponent>(entity);
				physics.force += dir.norm() * -450000.0f;

				auto& player = reg.get<PlayerComponent>(player_entity);
				player.health -= e.damage;
				e.attack_timer = 0.0f;
			}
		}
	}
private:
	entt::entity player_entity;
};