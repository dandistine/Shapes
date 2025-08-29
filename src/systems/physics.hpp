#pragma once

#include "components.hpp"
#include "system.hpp"

#include "utilities/entt.hpp"

struct PhysicsSystem : public System {
	PhysicsSystem(entt::registry& reg, olc::PixelGameEngine* pge) : System(reg, pge) {};

	void PreUpdate() override {
		const auto& view = reg.view<PhysicsComponent>();

		for(auto entity : view) {
			auto& physics = view.get<PhysicsComponent>(entity);
			// if(physics.force.mag2() > 0) {
			// 	physics.acceleration *=  0.8f;
			// 	physics.velocity *= physics.friction;
			// } else {
			// 	physics.acceleration *=  0.4f;
			// 	physics.velocity *= physics.friction * physics.friction;
			// }

			//physics.force = {0.0f, 0.0f};
		}
	}

	void OnUserUpdate(float fElapsedTime) override {
		const auto& view = reg.view<PhysicsComponent, Shape>();
		const float dt = 1.0f / 60.0f;
		total_time += fElapsedTime;

		while(total_time > dt) {
			total_time -= dt;
			for(auto entity : view) {
				auto& physics = view.get<PhysicsComponent>(entity);
				auto& shape = view.get<Shape>(entity);

				if(physics.force.mag2() > 0) {
					physics.acceleration *=  0.8f;
					physics.velocity *= physics.friction;
				} else {
					physics.acceleration *=  0.4f;
					physics.velocity *= physics.friction * physics.friction;
				}
	// 			if(physics.force.mag2() > 0) {
	// 				physics.acceleration -=  (24.0f * physics.acceleration * fElapsedTime);
	// 				physics.velocity = physics.velocity - 30.0f * physics.velocity * (1.0f - physics.friction) * fElapsedTime;
	// 				//physics.velocity *= physics.friction;
	// 			} else {
	// 				physics.acceleration -=  (12.0f * physics.acceleration * fElapsedTime);
	// 				physics.velocity -= 30.0f * physics.velocity * (1.0f - physics.friction * physics.friction) * fElapsedTime;
	// //				physics.velocity *= (physics.friction * physics.velocity);
	// 			}
	
				physics.acceleration += (physics.force / physics.mass) * dt;
				physics.velocity += physics.acceleration * dt;
	
				shape.theta += physics.angular_velocity * dt;
	
				shape.MoveTo(shape.position + physics.velocity * dt);
				physics.force = {0.0f, 0.0f};

			}
		}


	}

	float total_time {0.0f};
};