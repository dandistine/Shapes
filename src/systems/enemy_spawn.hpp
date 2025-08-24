#pragma once

#include "components.hpp"
#include "system.hpp"

#include <random>

struct EnemySpawnSystem : public System {
	EnemySpawnSystem(entt::dispatcher& dispatcher, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		total_time += fElapsedTime;
		spawn_timer += fElapsedTime;
		accumulated_power += fElapsedTime * std::powf(power_scale, total_time / power_time);

		// If its not time to check spawns, then return immediately
		if(spawn_timer < spawn_rate) {
			return;
		}

		while(accumulated_power > spawn_cost) {
			accumulated_power -= spawn_cost;

			SpawnEnemy spawn;
			spawn.color = olc::YELLOW;
			utilities::random::uniform_real_distribution<float> dist {0, olc::utils::geom2d::pi * 2.0f};
			const float angle = dist(rng);

			// Position the enemy to spawn outside the visiable map area
			spawn.position = olc::vf2d{pge->ScreenWidth() / 1.7f, angle}.cart() + pge->GetScreenSize() / 2.0f;
			dispatcher.enqueue<SpawnEnemy>(spawn);
		}
	}

private:
	entt::dispatcher& dispatcher;
	// Every power_time seconds the enemy power will scale by power_scale
	// This happens smoothly over the duration
	float power_scale {3.0f};
	float power_time {15.0f};
	float accumulated_power {0.0f};
	float spawn_cost {10.0f};
	float total_time {0.0f};

	// Every 5 seconds, attempt to spawn things
	float spawn_timer {0.0f};
	float spawn_rate {5.0f};

	std::mt19937_64 rng{std::random_device{}()};
};