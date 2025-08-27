#pragma once

#include "weapon.hpp"

struct PierceWeapon : Weapon {
	PierceWeapon(entt::registry& reg, entt::dispatcher& dispatcher, const Prototype& prototype) : Weapon(reg, dispatcher, prototype) {
        damage = 5.0f;
        
    };

    void OnUserUpdate(float fElapsedTime) override {
        accumulated_power += fElapsedTime;

        while(accumulated_power > fire_cost) {
            float angle = aim_direction.polar().y;
            utilities::random::uniform_real_distribution<float> dist{-aim_variance, aim_variance};
            angle += dist(rng);

            SpawnBullet spawn;
            spawn.angular_velocity = 0.0f;
            spawn.damage = damage;
            spawn.duration = 10.0f;
            spawn.hit_count = 3;
            spawn.initial_velocity = olc::vf2d{1.0f, angle}.cart() * 450.0f;
            spawn.position = position;
            spawn.scale = 0.4f;
            spawn.shape = bullet_shape;

            //std::cout << position << std::endl;
            dispatcher.enqueue(spawn);
            accumulated_power -= fire_cost;
        }
    
    }

	// Improve the weapon count times
	void LevelUp(int count) override;
};
