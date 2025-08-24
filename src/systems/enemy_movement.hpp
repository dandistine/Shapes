#pragma once

#include "components.hpp"
#include "shape.hpp"
#include "system.hpp"

#include "utilities/entt.hpp"

struct EnemyMovementSystem : public System {
	EnemyMovementSystem(entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : player_entity(player), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		const auto& view = reg.view<PhysicsComponent, Shape, EnemyComponent>();
		const auto& player_position = reg.get<Shape>(player_entity).position;

		for(auto entity : view) {
			auto& physics = view.get<PhysicsComponent>(entity);
			const auto& s = reg.get<Shape>(entity);
			const auto& dir = player_position - s.position;

			physics.force += dir.norm() * 4500.0f;
		}
	}
private:
	entt::entity player_entity;
};