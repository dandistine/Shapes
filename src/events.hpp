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
	float rotate {0.0f};
	bool fire {false};
};

struct SpawnEnemy {
	olc::vf2d position;
	olc::Pixel color;
};

struct EnemyDeath {
	// Where the enemy died
	olc::vf2d position;

    // awards experience
    bool gives_xp {true};
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
    olc::Pixel color;

	// Functor called whgen an enemy is hit
	std::function<void(entt::registry&, entt::dispatcher&, olc::vf2d)> on_hit_func {[](entt::registry&, entt::dispatcher&, olc::vf2d){}};
    // Functor called when an enemy is killed
   	std::function<void(entt::registry&, entt::dispatcher&, olc::vf2d)> on_kill_func {[](entt::registry&, entt::dispatcher&, olc::vf2d){}};

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

// Inform that a specific boss should be spawned and begin the boss lead in
struct SpawnBoss {
    // Name of the boss
    std::string name;

    //Boss power level
    int power {0};
};

// Boss lead in is completed, continue to main boss phase
struct BeginBossMain {
    // boss entity id, in case its needed elsewhere
    entt::entity entity;
};

// Boss has been killed, begin boss lead out
struct BossKill {

};

// Boss lead out is done, return to normal play
struct BossPhaseDone {

};

struct SetBackgroundColor {
	olc::Pixel color;
};

struct PlayerDied {
	
};

// Play a random sound from the specified soundbank
struct PlayRandomEffect {
	std::string soundbank;
};

struct PlayMusic {
	int sound_id{0};
	float fade_rate{1.0f};
};