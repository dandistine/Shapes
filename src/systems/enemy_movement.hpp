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

		for(auto itr = view.begin(); itr != view.end(); itr++) {
			auto& p1 = view.get<PhysicsComponent>(*itr);
			const auto& s1 = view.get<Shape>(*itr);

			// Move the enemy towards the player
			const auto& dir = player_position - s1.position;
			p1.force += dir.norm() * 450000.0f * fElapsedTime;
			
			// Break out if the next entity in the view would not be valid
			auto next_itr = itr;
			next_itr++;
			if (next_itr == view.end()) {
				break;
			}

			auto& p2 = view.get<PhysicsComponent>(*next_itr);
			const auto& s2 = view.get<Shape>(*next_itr);

			// Apply a smaller repulsive force between enemies to keep them from overlapping too much
			const auto dist = s2.position - s1.position;
			const float scalar = utilities::lerp(1.0f, 0.0f, std::min(1.0f, dist.mag2() / 3000.0f));
            p1.force += dist.norm() * -1500.0f * scalar;
            p2.force += dist.norm() * 1500.0f * scalar;
		}
		
		// for(auto entity : view) {
		// 	auto& physics = view.get<PhysicsComponent>(entity);
		// 	const auto& s = reg.get<Shape>(entity);
		// 	const auto& dir = player_position - s.position;

		// 	physics.force += dir.norm() * 450000.0f * fElapsedTime;
		// }
	}
private:
	entt::entity player_entity;
};