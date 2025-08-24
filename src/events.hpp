#pragma once

#include "shape.hpp"

#include "utilities/entt.hpp"

#include "olcPixelGameEngine.h"

#include <functional>
#include <string>

// Events messages which are sent through the entt::dispatcher

struct PlayerInput {
	olc::vf2d move_direction {};
	olc::vf2d aim_direction {};
	bool fire {false};
};

struct SpawnEnemy {
	olc::vf2d position;
	olc::Pixel color;
};

struct EnemyDeath {
	// Where the enemy died
	olc::vf2d position;
};

struct SpawnBullet {
	// Where the bullet spawns
	olc::vf2d position;
	// Initial velocity
	olc::vf2d initial_velocity;
	// How many enemies the bullet can hit before being removed
	int hit_count;
	// base damage dealt
	float damage;
	// how long the bullet survives before being removed
	float duration;
	// how quickly the bullet spins
	float angular_velocity;
	//how big the bullet is
	float scale;
	// bullet shape
	ShapePrototypes shape;
};

struct SpawnExperience {
	// Where the experience spawns
	olc::vf2d position;
	// How much this experience is worth
	float value {1.0f};
	// How long this experience has been on the ground
	float age {0.0f};
};

struct LevelUp {

};

struct LevelUpOption {
	// Description to the player of what this option does
	std::string description;
	// Functor that actually applies this option to the player
	std::function<void(entt::registry& reg, entt::entity)> functor;
};