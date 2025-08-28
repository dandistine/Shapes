#include "weapon.hpp"

#include "utilities/random.hpp"
#include "utilities/utility.hpp"

std::mt19937_64 weapon_rng {std::random_device{}()};

WeaponPrototype DefaultWeapon {
    .name{"Standard"},
    .LevelUpFunction {
        [](WeaponPrototype& weapon, int count){
            weapon.fire_cost *= std::powf(0.95f, count);
            weapon.damage *= std::powf(1.1f, count);
            weapon.aim_variance *= std::powf(0.99f, count);
            weapon.level += count;
        }
    }
};

WeaponPrototype PierceWeapon {
    .hit_count {3},
    .aim_variance {0.1f},
    .damage {30.0f},
    .fire_cost {1.0f},
    .initial_velocity {600.0f},
    .name{"Pierce"},
    .LevelUpFunction {
        [](WeaponPrototype& weapon, int count){
            weapon.damage *= std::powf(1.2f, count);
            weapon.aim_variance *= std::powf(0.99f, count);
            weapon.level += count;
            weapon.hit_count = 3 + weapon.level / 2;
        }     
    },
    .type {ShapePrototypes::Cursor}
};

WeaponPrototype BurstWeapon{
    .hit_count {1},
    .aim_variance {0.05},
    .damage {100.0f},
    .fire_cost {3.0f},
    .initial_velocity {150.0f},
    .name{"Burst"},
    .LevelUpFunction{
        [](WeaponPrototype& weapon, int count) {
            weapon.damage *= std::powf(1.3f, count);
            weapon.level += count;
            weapon.initial_velocity += 25.0f * count;
            weapon.fire_cost = std::max(1.5f, weapon.fire_cost * std::powf(.95f, count));
        }
    },
    .on_kill_func {
        [](entt::registry& reg, entt::dispatcher& dispatcher, olc::vf2d position) {
            utilities::random::uniform_real_distribution<float> dist{olc::utils::geom2d::pi, -olc::utils::geom2d::pi};
            // Spawn a bunch of projectiles
            for(int i = 0; i < 32; i++) {
                float r1 = dist(weapon_rng);
                float r2 = dist(weapon_rng);
                float r3 = dist(weapon_rng);

                SpawnBullet spawn;
                spawn.position = position;
                spawn.initial_velocity = olc::vf2d{200.0f + r1, r2}.cart();
                spawn.angular_velocity = r3;
                spawn.hit_count = 1;
                spawn.damage = 10.0f;
                spawn.duration = 3.0f;
                spawn.angular_velocity = r1 + r2;
                spawn.scale = 0.4f;
                spawn.shape = ShapePrototypes::Square;
                spawn.color = utilities::RandomRedColor();

                dispatcher.enqueue(spawn);
            }
        }
    },
    .type {ShapePrototypes::Pentagon}
};


std::array<WeaponPrototype*, 3> weapon_prototypes {
    &DefaultWeapon,
    &PierceWeapon,
    &BurstWeapon
};