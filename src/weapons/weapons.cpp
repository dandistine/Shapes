#include "weapon.hpp"

WeaponPrototype DefaultWeapon {};

WeaponPrototype PierceWeapon {
    .hit_count {3},
    .aim_variance {0.1f},
    .damage {30.0f},
    .fire_cost {1.0f},
    .initial_velocity {600.0f},
    .name{"Pierce"},
    .type {ShapePrototypes::Cursor}
};

std::array<WeaponPrototype*, 2> weapon_prototypes {
    &DefaultWeapon,
    &PierceWeapon
};