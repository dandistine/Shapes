#pragma once

#include "components.hpp"
#include "system.hpp"

struct BulletSystem : public System {
	BulletSystem(entt::dispatcher& dispatcher, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), System(reg, pge) {};

	void PreUpdate() override {
		// Check if any bullets are off-screen and cull them before processing this frame
		// or have expired
		auto view = reg.view<BulletComponent, Shape>();

		for(const auto e : view) {
			auto [b, s] = view.get(e);
			if ((s.position.x < -10.0f) || (s.position.y < -10.0f) || (s.position.x > pge->ScreenWidth() + 10.0f) || (s.position.y > pge->ScreenHeight() + 10.0f) || (b.duration < 0.0f)) {
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

			b.duration -= fElapsedTime;

			for(auto e_entity : enemy_view) {
                // Don't allow bullets to hit the same enemy twice in a row
                if(e_entity == b.last_hit) {
                    continue;
                }

				auto [e, e_shape] = enemy_view.get(e_entity);

                // If the bullet isn't remotely close, skip the intersection check
                if((b_shape.position - e_shape.position).mag2() > (e_shape.scale*e_shape.scale*64)) {
                    continue;
                }

				// Check if this is a hit
				if(b_shape.intersects(e_shape)) {
					b.last_hit = e_entity;
                    b.hit_count--;
					e.health -= b.damage;

					b.on_hit_func(reg, dispatcher, b_shape.position);

					// Need to handle the Dark Triad boss specially
					if(e.health <= 0.0f && !reg.storage<DarkTriadMember>().contains(e_entity)) {
                        //std::cout << "Killing " << entt::to_integral(e_entity) << std::endl;
                        b.on_kill_func(reg, dispatcher, e_shape.position);
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
