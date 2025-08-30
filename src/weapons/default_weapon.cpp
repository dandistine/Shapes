#include "weapon.hpp"

#include "utilities/random.hpp"
#include "utilities/global_rng.hpp"


// Copy all of the non-reference attributes from another weapon
void Weapon::Clone(const Weapon& other) {
    prototype = other.prototype;
    //projectile_count = other.projectile_count;
    level = other.level;
    //aim_variance = other.aim_variance;
    //damage = other.damage;
    //bullet_shape = other.bullet_shape;
    accumulated_power = other.accumulated_power;
    //fire_cost = other.fire_cost;
    aim_direction = other.aim_direction;
    position = other.position;
    shape = other.shape;
}

void Weapon::on_player_input(const PlayerInput& input) {
    aim_direction = input.aim_direction;
    shape.theta = input.aim_direction.polar().y + static_cast<float>(olc::utils::geom2d::pi) / 2.0f;
}

void Weapon::OnUserUpdate(float fElapsedTime) {
    accumulated_power += fElapsedTime;

    while(accumulated_power > prototype.fire_cost) {
        for(int i = 0; i < prototype.projectile_count; i++) {
            float angle = aim_direction.polar().y;
            utilities::random::uniform_real_distribution<float> dist{-prototype.aim_variance, prototype.aim_variance};
            angle += dist(rng);

            SpawnBullet spawn;
            spawn.angular_velocity = prototype.angular_velocity;
            spawn.damage = prototype.damage;
            spawn.duration = prototype.duration;
            spawn.hit_count = prototype.hit_count;
            spawn.initial_velocity = olc::vf2d{1.0f, angle}.cart() * prototype.initial_velocity;
            spawn.position = position;
            spawn.scale = prototype.scale;
            spawn.shape = prototype.type;
            spawn.on_hit_func = prototype.on_hit_func;
            spawn.on_kill_func = prototype.on_kill_func;
            spawn.color = prototype.color;

            //std::cout << position << std::endl;
            dispatcher.enqueue(spawn);
        }
        accumulated_power -= prototype.fire_cost;
    }

}

// Improve the weapon count times
void Weapon::LevelUp(int count) {
    prototype.LevelUpFunction(prototype, count);
    level += count;
    // prototype.fire_cost *= std::powf(0.99, count);
    // //prototype.projectile_count = static_cast<int>(std::ceilf((level + count) / 3.0f));
    // prototype.damage *= std::powf(1.05, count);
}

void Weapon::SetPosition(olc::vf2d pos) {
    position = pos;
    shape.MoveTo(pos);
}

void Weapon::Draw(olc::PixelGameEngine* pge) const {
    shape.Draw(pge);
}

ShapePrototypes Weapon::BulletShape() const {
    return prototype.type;
}

std::string Weapon::Name() const {
    return prototype.name;
}

int Weapon::Level() const {
    return level;
}

Weapon::Weapon(entt::registry& reg, entt::dispatcher& dispatcher, const WeaponPrototype& prototype) : reg(reg), dispatcher(dispatcher), shape(prototypes[prototype.type]), prototype(prototype) {
    shape.color = prototype.color;
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
