#pragma once

#include "olcPixelGameEngine.h"

#include "events.hpp"
#include "weapon.hpp"

struct EnemyComponent {
	float health {10.0f};
	float damage {1.0f};
	float attack_timer {0.0f};
	float attack_cooldown {1.0f};
	olc::vf2d velocity {0.0f, 0.0f};
};

struct PlayerComponent {
	float health {10.0f};
	float max_health {10.0f};
	int level {1};
	float experience {0.0f};
	float xp_to_next_level {5.0f};
	float experience_range {120.0f};
	int max_weapon_count {1};
	std::vector<Weapon> weapons;
};

struct BulletComponent {
	BulletComponent() = default;
	BulletComponent(const SpawnBullet& spawn) : damage(spawn.damage), hit_count(spawn.hit_count), duration(spawn.duration), angular_velocity(spawn.angular_velocity) { }
	//olc::vf2d velocity {};
	float damage {10.0f};
	int hit_count {1};
	float duration {10.0f};
	float angular_velocity {14.0};
};

struct PhysicsComponent {
	olc::vf2d velocity {};
	olc::vf2d acceleration {};
	olc::vf2d force {};
	float mass {1.0f};
	float friction {0.95f};
	float angular_velocity {0.0f};
};

struct ParticleComponent {
	// How long in seconds that particle should stay around
	float lifespan {1.0f};

	// Particle fades to alpha when lifespan is below this
	float fade_begin {1.0f};
};

struct ExperienceComponent {
	float value {1.0f};
	float age {0.0f};
};
