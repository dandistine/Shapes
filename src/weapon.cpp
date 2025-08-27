#include "weapon.hpp"

#include "utilities/random.hpp"

// Copy all of the non-reference attributes from another weapon
void Weapon::Clone(const Weapon& other) {
    projectile_count = other.projectile_count;
    level = other.level;
    aim_variance = other.aim_variance;
    damage = other.damage;
    bullet_shape = other.bullet_shape;
    accumulated_power = other.accumulated_power;
    fire_cost = other.fire_cost;
    aim_direction = other.aim_direction;
    position = other.position;
    shape = other.shape;
}

void Weapon::on_player_input(const PlayerInput& input) {
    aim_direction = input.aim_direction;
}

void Weapon::OnUserUpdate(float fElapsedTime) {
    accumulated_power += fElapsedTime;

    while(accumulated_power > fire_cost) {
        float angle = aim_direction.polar().y;
        utilities::random::uniform_real_distribution<float> dist{-aim_variance, aim_variance};
        angle += dist(rng);

        SpawnBullet spawn;
        spawn.angular_velocity = 14.0f;
        spawn.damage = damage;
        spawn.duration = 10.0f;
        spawn.hit_count = 1;
        spawn.initial_velocity = olc::vf2d{1.0f, angle}.cart() * 250.0f;
        spawn.position = position;
        spawn.scale = 0.4f;
        spawn.shape = bullet_shape;

        //std::cout << position << std::endl;
        dispatcher.enqueue(spawn);
        accumulated_power -= fire_cost;
    }

}

// Improve the weapon count times
void Weapon::LevelUp(int count) {
    fire_cost *= std::powf(0.99, count);
    projectile_count = static_cast<int>(std::ceilf((level + count) / 3.0f));
    damage *= std::powf(1.05, count);
}

void Weapon::SetPosition(olc::vf2d pos) {
    position = pos;
    shape.MoveTo(pos);
}

void Weapon::Draw(olc::PixelGameEngine* pge) const {
    shape.Draw(pge);
}

ShapePrototypes Weapon::BulletShape() const {
    return bullet_shape;
}


Weapon::Weapon(entt::registry& reg, entt::dispatcher& dispatcher, const Prototype& prototype) : reg(reg), dispatcher(dispatcher), shape(prototype) {
    dispatcher.sink<PlayerInput>().connect<&Weapon::on_player_input>(this);
}

Weapon::Weapon(const Weapon& other): reg(other.reg), dispatcher(other.dispatcher), shape(other.shape) {
    dispatcher.sink<PlayerInput>().connect<&Weapon::on_player_input>(this);
    Clone(other);
}

Weapon::Weapon(Weapon&& other) : reg(other.reg), dispatcher(other.dispatcher), shape(other.shape) {
    dispatcher.sink<PlayerInput>().connect<&Weapon::on_player_input>(this);
    Clone(other);
}

Weapon::~Weapon() {
    dispatcher.sink<PlayerInput>().disconnect(this);
}
