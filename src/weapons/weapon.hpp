#pragma once

#include "events.hpp"
#include "shape.hpp"

#include "olcPixelGameEngine.h"

#include "utilities/entt.hpp"

#include <random>


extern std::map<ShapePrototypes, Prototype> prototypes;

struct WeaponPrototype {
	int projectile_count {1};
	int hit_count {1};
	float aim_variance {0.2f};
	float damage {10.0f};
	float fire_cost {.35f};
	float angular_velocity {0.0f};
	float duration {10.0f};
	float initial_velocity {250.0f};
	float scale {0.4f};
	std::string name{"Standard"};

	olc::Pixel color {olc::WHITE};
	// Function that can be called when an enemy is killed by this weapon
	std::function<void(void)> on_kill_func {[](){}};
	ShapePrototypes type {ShapePrototypes::Triangle};
};

extern std::array<WeaponPrototype*, 2> weapon_prototypes;
extern WeaponPrototype DefaultWeapon;
extern WeaponPrototype PierceWeapon;

struct Weapon {
	Weapon(entt::registry& reg, entt::dispatcher& dispatcher, const WeaponPrototype& prototype);

	// Copy all of the non-reference attributes from another weapon
	void Clone(const Weapon& other);

	Weapon(const Weapon& other);

	Weapon(Weapon&& other);

	~Weapon();

	void on_player_input(const PlayerInput& input);

	virtual void OnUserUpdate(float fElapsedTime);

	// Improve the weapon count times
	virtual void LevelUp(int count);

	void SetPosition(olc::vf2d pos);

    void Draw(olc::PixelGameEngine* pge) const;

	ShapePrototypes BulletShape() const;
	
private:
	//int projectile_count {1};
	int level {1};
	//float aim_variance {0.2};
	//float damage {10.0f};
	//ShapePrototypes bullet_shape {ShapePrototypes::Square};
    Shape shape;

	// Fires when the accumulated_power goes over the fire_cost
	float accumulated_power {0.0f};
	//float fire_cost {0.1f};

	olc::vf2d aim_direction;
	olc::vf2d position;
	entt::registry& reg;
	entt::dispatcher& dispatcher;
	WeaponPrototype prototype;
	std::mt19937_64 rng{std::random_device{}()};
};
