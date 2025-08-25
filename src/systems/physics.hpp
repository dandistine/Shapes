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
			if(physics.force.mag2() > 0) {
				physics.acceleration *=  0.8f;
				physics.velocity *= physics.friction;
			} else {
				physics.acceleration *=  0.4f;
				physics.velocity *= physics.friction * physics.friction;
			}

			physics.force = {0.0f, 0.0f};
		}
	}

	void OnUserUpdate(float fElapsedTime) override {
		const auto& view = reg.view<PhysicsComponent, Shape>();

		for(auto entity : view) {
			auto& physics = view.get<PhysicsComponent>(entity);
			auto& shape = view.get<Shape>(entity);

			physics.acceleration += (physics.force / physics.mass) * fElapsedTime;
			physics.velocity += physics.acceleration * fElapsedTime;

            shape.theta += physics.angular_velocity * fElapsedTime;

			shape.MoveTo(shape.position + physics.velocity * fElapsedTime);
		}
	}
};