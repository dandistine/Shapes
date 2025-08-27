#pragma once

#include "events.hpp"
#include "shape.hpp"

#include "olcPixelGameEngine.h"

#include "utilities/entt.hpp"

#include <random>

struct Weapon {
	Weapon(entt::registry& reg, entt::dispatcher& dispatcher, const Prototype& prototype);

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
	int projectile_count {1};
	int level {1};
	float aim_variance {0.2};
	float damage {10.0f};
	ShapePrototypes bullet_shape {ShapePrototypes::Square};
    Shape shape;

	// Fires when the accumulated_power goes over the fire_cost
	float accumulated_power {0.0f};
	float fire_cost {0.1f};

	olc::vf2d aim_direction;
	olc::vf2d position;
	entt::registry& reg;
	entt::dispatcher& dispatcher;
	std::mt19937_64 rng{std::random_device{}()};
};
