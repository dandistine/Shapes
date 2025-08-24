#pragma once

#include "components.hpp"
#include "system.hpp"

struct BulletSystem : public System {
	BulletSystem(entt::dispatcher& dispatcher, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), System(reg, pge) {};

	void PreUpdate() override {
		// Check if any bullets are off-screen and cull them before processing this frame
		auto view = reg.view<BulletComponent, Shape>();

		for(const auto e : view) {
			const auto& s = reg.get<Shape>(e);
			if ((s.position.x < -10.0f) || (s.position.y < -10.0f) || (s.position.x > pge->ScreenWidth() + 10.0f) || (s.position.y > pge->ScreenHeight() + 10.0f)) {
				reg.destroy(e);
			}
		}
	}

	void OnUserUpdate(float fElapsedTime) override {
		// Check if any enemy is overlapping any bullet and then deal damage
		auto bullet_view = reg.view<BulletComponent, Shape>();
		auto enemy_view = reg.view<EnemyComponent, Shape>();

		for(auto b_entity : bullet_view) {
			auto [b, b_shape] = bullet_view.get(b_entity);

			for(auto e_entity : enemy_view) {
				auto [e, e_shape] = enemy_view.get(e_entity);

				// Check if this is a hit
				if(b_shape.intersects(e_shape)) {
					b.hit_count--;
					e.health -= b.damage;

					if(e.health <= 0) {
						dispatcher.enqueue<EnemyDeath>(e_shape.position);
						reg.destroy(e_entity);
					}

					if (b.hit_count <= 0) {
						reg.destroy(b_entity);
						break;
					}
				}
			}

		}
	}

private:
	entt::dispatcher& dispatcher;
};
