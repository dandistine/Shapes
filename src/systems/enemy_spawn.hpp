#pragma once

#include "components.hpp"
#include "system.hpp"

#include "utilities/global_rng.hpp"

#include <random>

struct SpawnDescriptor {
    // Total cost of this spawn event
    float cost{10.0f};
    // Health of an individual enemy
    float health{10.0f};
    // Mass of an individual enemy
    float mass{1.0f};
    // Size of an individual enemy
    float scale{1.0f};
    // Number of enemies to spawn
    int count{0};
    // Color of the enemies
    olc::Pixel color{olc::YELLOW};
    // Shape of the enemies
    ShapePrototypes type{ShapePrototypes::Square};
};

struct SpawnFactory {
    virtual std::optional<SpawnDescriptor> GetSpawn(float current_power, int boss_count) = 0;
    virtual ~SpawnFactory() = default;
};

struct StandardEnemyFactory : public SpawnFactory{
    std::optional<SpawnDescriptor> GetSpawn(float current_power, int boss_count) {
        float base_cost = 5.0f + 10.0f * boss_count;

        // If there isn't enough power, return nullopt to indicate that
        if(current_power < base_cost) {
            return std::nullopt;
        }

        float could_spawn = std::floorf(current_power / base_cost);
        int spawn_count = std::min(5.0f, std::floorf(current_power / base_cost));
        float total_cost =  could_spawn * base_cost;
        
        float bonus_health = 0.0f;
        float bonus_scale = 0.0f;
        float bonus_mass = 0.0f;

        if(could_spawn > spawn_count) {
            bonus_health = (could_spawn - spawn_count) * base_cost / 5.0f;
            //std::cout << bonus_health << std::endl;
            // bonus_scale = std::sqrtf(bonus_health);
            // bonus_mass = std::sqrtf(bonus_scale);
        }

        

        SpawnDescriptor spawn;
        spawn.cost = total_cost;
        spawn.health = base_cost + bonus_health;
        spawn.mass = 1.0f + bonus_mass;
        spawn.scale = 2.0f + bonus_scale;
        spawn.count = spawn_count;
        spawn.color = olc::GREY;
        spawn.type = ShapePrototypes::Triangle;

        return spawn;
    }
};

struct HordeEnemyFactory : public SpawnFactory{
    std::optional<SpawnDescriptor> GetSpawn(float current_power, int boss_count) {
        float base_cost = 30.0f + 10.0f * boss_count;

        // If there isn't enough power, return nullopt to indicate that
        if(current_power < base_cost) {
            return std::nullopt;
        }

        int spawn_count = 6 + boss_count;
        float total_cost = base_cost;

        SpawnDescriptor spawn;
        spawn.cost = total_cost;
        spawn.health = 1.0f + boss_count;
        spawn.mass = .80f;
        spawn.scale = 1.0f;
        spawn.count = spawn_count;
        spawn.color = olc::DARK_CYAN;
        spawn.type = ShapePrototypes::Square;

        return spawn;
    }
};

struct ToughEnemyFactory : public SpawnFactory{
    std::optional<SpawnDescriptor> GetSpawn(float current_power, int boss_count) {
        float base_cost = 10.0f + 10.0f * boss_count;

        // If there isn't enough power, return nullopt to indicate that
        if(current_power < base_cost) {
            return std::nullopt;
        }

        int spawn_count = 1;
        float total_cost = std::floorf(current_power / base_cost) * base_cost;

        SpawnDescriptor spawn;
        spawn.cost = total_cost;
        spawn.health = (100 + total_cost) * (boss_count + 1);
        spawn.mass = std::sqrtf(total_cost);
        spawn.scale = std::sqrtf(total_cost);
        spawn.count = spawn_count;
        spawn.color = olc::VERY_DARK_BLUE;
        spawn.type = ShapePrototypes::Pentagon;

        return spawn;
    }
};


std::array<std::unique_ptr<SpawnFactory>, 3> spawn_factories = {
    std::make_unique<StandardEnemyFactory>(),
    std::make_unique<HordeEnemyFactory>(),
    std::make_unique<ToughEnemyFactory>()
};


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

        spawn_timer -= spawn_rate;

        //StandardEnemyFactory factory;
        // Pick a random factory to use
        const auto& factory = spawn_factories[random_factory(rng)];


        auto potential_spawn = factory->GetSpawn(accumulated_power, 0);

        if(potential_spawn) {
            auto spawn = potential_spawn.value();
            accumulated_power -= spawn.cost;
            dispatcher.enqueue<SpawnDescriptor>(spawn);
        }

		while(accumulated_power > spawn_cost) {
			accumulated_power -= spawn_cost;

			SpawnEnemy spawn;
			spawn.color = olc::YELLOW;
			utilities::random::uniform_real_distribution<float> dist {0, static_cast<float>(olc::utils::geom2d::pi) * 2.0f};
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
	float power_scale {2.5f};
	float power_time {30.0f};
	float accumulated_power {0.0f};
	float spawn_cost {10.0f};
	float total_time {0.0f};

	// Every 2 seconds, attempt to spawn things
	float spawn_timer {0.0f};
	float spawn_rate {2.0f};

	//std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> random_factory{0, spawn_factories.size() - 1};
};