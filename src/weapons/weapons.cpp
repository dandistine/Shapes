#include "weapon.hpp"

#include "utilities/random.hpp"
#include "utilities/utility.hpp"
#include "utilities/global_rng.hpp"

//std::mt19937_64 weapon_rng {std::random_device{}()};

WeaponPrototype DefaultWeapon {
    .name{"Standard"},
    .LevelUpFunction {
        [](WeaponPrototype& weapon, int count){
            weapon.fire_cost *= std::powf(0.95f, count);
            weapon.damage *= std::powf(1.1f, count);
            weapon.aim_variance *= std::powf(0.99f, count);
            weapon.level += count;
            weapon.projectile_count = 1 + weapon.level / 3;
        }
    }
};

WeaponPrototype SpreadWeapon {
    .projectile_count {17},
    .damage {8.0f},
    .fire_cost {1.5f},
    .duration {0.7f},
    .initial_velocity {350.0f},
    .name{"Spread"},
    .color {utilities::RandomGreenColor()},
    .LevelUpFunction {
        [](WeaponPrototype& weapon, int count){
            weapon.fire_cost *= std::powf(0.95f, count);
            weapon.damage *= std::powf(1.1f, count);
            weapon.duration *= std::powf(0.99f, count);
            weapon.initial_velocity *= std::powf(0.99f, count);
            weapon.level += count;
            weapon.projectile_count = 16 + weapon.level;
        }
    },
    .type {ShapePrototypes::Star7_3}
};

WeaponPrototype Nova {
    .projectile_count {64},
    .aim_variance {static_cast<float>(olc::utils::geom2d::pi)},
    .damage {20.0f},
    .fire_cost {5.0f},
    .duration {3.0f},
    .initial_velocity {150.0f},
    .name{"Nova"},
    .color {olc::DARK_YELLOW},
    .LevelUpFunction {
        [](WeaponPrototype& weapon, int count){
            weapon.fire_cost *= std::powf(0.95f, count);
            weapon.damage *= std::powf(1.1f, count);
            weapon.level += count;
            weapon.duration += static_cast<float>(count) * 0.3f;
        }
    },
    .type {ShapePrototypes::Star8_2}
};

WeaponPrototype PierceWeapon {
    .hit_count {3},
    .aim_variance {0.07f},
    .damage {34.0f},
    .fire_cost {1.0f},
    .initial_velocity {600.0f},
    .name{"Pierce"},
    .color {utilities::RandomColor()},
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
    .color{utilities::RandomRedColor()},
    .LevelUpFunction{
        [](WeaponPrototype& weapon, int count) {
            weapon.damage *= std::powf(1.3f, count);
            weapon.level += count;
            weapon.initial_velocity += 25.0f * count;
            weapon.fire_cost = std::max(1.5f, weapon.fire_cost * std::powf(.95f, count));
        }
    },
    .on_hit_func {
        [](entt::registry& reg, entt::dispatcher& dispatcher, olc::vf2d position) {
            utilities::random::uniform_real_distribution<float> dist{static_cast<float>(olc::utils::geom2d::pi), -static_cast<float>(olc::utils::geom2d::pi)};
            // Spawn a bunch of projectiles
            for(int i = 0; i < 32; i++) {
                float r1 = dist(rng);
                float r2 = dist(rng);
                float r3 = dist(rng);

                SpawnBullet spawn;
                spawn.position = position;
                spawn.initial_velocity = olc::vf2d{200.0f + r1, r2}.cart();
                spawn.angular_velocity = r3;
                spawn.hit_count = 1;
                spawn.damage = 3.0f;
                spawn.duration = 3.0f;
                spawn.angular_velocity = r1 + r2;
                spawn.scale = 0.4f;
                spawn.shape = ShapePrototypes::Square;
                spawn.color = utilities::RandomRedColor();

                dispatcher.enqueue(spawn);
                dispatcher.enqueue<PlayRandomEffect>({"explode"});
            }
        }
    },
    .type {ShapePrototypes::Pentagon}
};

WeaponPrototype RapidFireWeapon {
    .damage {3.0f},
    .fire_cost {0.1f},
    .angular_velocity {2.0f},
    .duration {5.0f},
    .scale {0.2f},
    .name {"Rapid Fire"},
    .color {utilities::RandomBlueColor()},
    .LevelUpFunction {
        [](WeaponPrototype& weapon, int count){
            weapon.fire_cost *= std::powf(0.9f, count);
            weapon.damage += count;
            weapon.aim_variance *= std::powf(0.99f, count);
            weapon.level += count;
            weapon.projectile_count = 1 + weapon.level / 7;
        }
    },
    .type {ShapePrototypes::Star6_2}
};

WeaponPrototype MineLayerWeapon {
    .damage {50.0f},
    .fire_cost {3.0f},
    .angular_velocity {-0.5},
    .initial_velocity {0.0f},
    .scale {1.0f},
    .name{"Mine Layer"},
    .color {olc::DARK_RED},
    .LevelUpFunction {
        [](WeaponPrototype& weapon, int count){
            weapon.damage *= std::powf(1.5f, count);
            weapon.angular_velocity = -0.5 * (count +1);
            weapon.duration *= std::powf(1.1, count);
            weapon.level += count;
        }
    },
    .on_hit_func {
        [](entt::registry& reg, entt::dispatcher& dispatcher, olc::vf2d position) {
            utilities::random::uniform_real_distribution<float> dist{static_cast<float>(olc::utils::geom2d::pi), -static_cast<float>(olc::utils::geom2d::pi)};
            // Spawn a bunch of projectiles
            for(int i = 0; i < 32; i++) {
                float r1 = dist(rng);
                float r2 = dist(rng);
                float r3 = dist(rng);

                SpawnBullet spawn;
                spawn.position = position;
                spawn.initial_velocity = olc::vf2d{200.0f + r1, r2}.cart();
                spawn.angular_velocity = r3;
                spawn.hit_count = 5;
                spawn.damage = 10.0f;
                spawn.duration = .5f;
                spawn.angular_velocity = r1 + r2;
                spawn.scale = 0.4f;
                spawn.shape = ShapePrototypes::Star6_2;
                spawn.color = utilities::RandomRedColor();

                dispatcher.enqueue(spawn);
                dispatcher.enqueue<PlayRandomEffect>({"explode"});
            }
        }
    },
    .type {ShapePrototypes::Cross}
};


std::array<WeaponPrototype*, 7> weapon_prototypes {
    &DefaultWeapon,
    &PierceWeapon,
    &BurstWeapon,
    &RapidFireWeapon,
    &MineLayerWeapon,
    &SpreadWeapon,
    &Nova
};