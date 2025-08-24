#include "utilities/olcUTIL_Geometry2D.h"
#include "olcPixelGameEngine.h"
#include "extensions/olcPGEX_TransformedView.h"
#include "utilities/olcUTIL_Camera2D.h"

#include "SimpleSerialization.hpp"

#include "utilities/quad_tree.hpp"
#include "utilities/random.hpp"
#include "utilities/sprite_sheet.hpp"

#include "utilities/entt.hpp"
#include "utilities/utility.hpp"

#include <vector>
#include <random>


using namespace entt::literals;

/// @brief Rotate a point p by an angle in the form {sin(theta), cos{theta}}
/// @param p 
/// @param sc 
/// @return 
olc::vf2d rotate(const olc::vf2d& p, const olc::vf2d& sc) {
	return olc::vf2d{p.x * sc.y - p.y * sc.x, p.x * sc.x + p.y * sc.y};
}

struct Prototype {
	using iterator = std::vector<olc::utils::geom2d::triangle<float>>::iterator;
	using const_iterator = std::vector<olc::utils::geom2d::triangle<float>>::const_iterator;
	
	iterator begin() {
		return tris.begin();
	}

	iterator end() {
		return tris.end();
	}

	const_iterator begin() const {
		return tris.cbegin();
	}

	const_iterator end() const {
		return tris.cend();
	}

	std::vector<olc::utils::geom2d::triangle<float>> tris;
	std::vector<olc::vf2d> weapon_points;
};

enum class ShapePrototypes {
	Triangle,
	Square,
	Cursor,
	Pentagon,
	Star5_2,
	Star6_2,
	Star7_3,
	Star8_2,
	Star9_3
};

// Players start as a triangle with 3 weapon slots and gain one slot per progression until 9
std::array shape_progression {ShapePrototypes::Triangle, ShapePrototypes::Square, ShapePrototypes::Pentagon, ShapePrototypes::Star6_2, ShapePrototypes::Star7_3, ShapePrototypes::Star8_2, ShapePrototypes::Star9_3};

olc::Pixel RandomColor()
{
	return olc::Pixel(rand() % 256, rand() % 256, rand() % 256);
}

std::map<ShapePrototypes, Prototype> prototypes;

struct Shape {
	using iterator = std::vector<olc::utils::geom2d::triangle<float>>::iterator;
	using const_iterator = std::vector<olc::utils::geom2d::triangle<float>>::const_iterator;

	Shape(const Prototype& other) : tris(other.tris), prototype(&other) { }
	
	// Shape(const Shape& other, const Prototype& new_proto) : tris(new_proto.tris), prototype(&new_proto) {
	// 	scale = other.scale;
	// 	theta = other.theta;
	// 	position = other.position;
	// 	color = other.color;
	// }

	// Shape(const Shape& other) : tris(other.tris), prototype(other.prototype) {
	// 	scale = other.scale;
	// 	theta = other.theta;
	// 	position = other.position;
	// 	color = other.color;
	// }

	iterator begin() {
		return tris.begin();
	}

	iterator end() {
		return tris.end();
	}

	const_iterator begin() const {
		return tris.cbegin();
	}

	const_iterator end() const {
		return tris.cend();
	}

	void SetPrototype(const Prototype& proto) {
		prototype = &proto;
	}

	void Draw(olc::PixelGameEngine* pge) const {
		for (const auto& t : tris) {
			pge->FillTriangleDecal(t.pos[0], t.pos[1], t.pos[2], color);
		}
    }

	// Scale, Rotate, and Translate a point from shape-space to world-space.  Rotation vector must be provided separately
	olc::vf2d Translate(olc::vf2d pos, const olc::vf2d& sc) const {
		return rotate(pos, sc) * scale + position;
	}

	void MoveTo(olc::vf2d new_position) {
		position = new_position;
		const auto sc = olc::vf2d{std::sinf(theta), std::cosf(theta)};
		//const float s = std::sin(theta);
		//const float c = std::cos(theta);
		tris.clear();

		for(const auto& t : *prototype) {
			tris.push_back(
				{
					Translate(t.pos[0], sc),
					Translate(t.pos[1], sc),
					Translate(t.pos[2], sc),
				}
			);
			// tris.push_back(
			// 	{
			// 		(rotate(t.pos[0], sc) * scale) + position,
			// 	 	(rotate(t.pos[1], sc) * scale) + position,
			// 	 	(rotate(t.pos[2], sc) * scale) + position
			// 	}
			// );
		}
	}

	bool intersects(const Shape& other) const {
		for(const auto& t : other) {
			for(const auto& self_t : tris) {
				if(olc::utils::geom2d::overlaps(t, self_t)) {
					return true;
				}
			}
		}

		return false;
	};

	const std::vector<olc::vf2d>& WeaponPoints() {
		const auto sc = olc::vf2d{std::sinf(theta), std::cosf(theta)};
		weapon_points.clear();
		for(auto p : prototype->weapon_points) {
			weapon_points.push_back(Translate(p, sc));
		}
		return weapon_points;
	}

	size_t WeaponPointCount() const {
		return prototype->weapon_points.size();
	}

	float scale {1.0f};
	float theta {0.0f};
	olc::vf2d position {0.0f, 0.0f};
	olc::Pixel color {olc::MAGENTA};

	virtual ~Shape() = default;
protected:
	std::vector<olc::utils::geom2d::triangle<float>> tris;
	std::vector<olc::vf2d> weapon_points;
	const Prototype* prototype;
};

struct Enemy : public Shape {
	Enemy(const Prototype& proto) : Shape(proto) {};

	float health {10.0f};
	olc::vf2d velocity {0.0f, 0.0f};
};


olc::vi2d print(olc::utils::geom2d::triangle<float>& tri, olc::vi2d pos, olc::PixelGameEngine* pge) {
	pge->DrawString(pos, tri.pos[0].str());
	pos.y += 10;
	pge->DrawString(pos, tri.pos[1].str());
	pos.y += 10;
	pge->DrawString(pos, tri.pos[2].str());
	pos.y += 10;

	return pos;
};

// Events
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

// Components
//using Velocity = olc::vf2d;

struct EnemyComponent {
	float health {10.0f};
	float damage {1.0f};
	float attack_timer {0.0f};
	float attack_cooldown {1.0f};
	olc::vf2d velocity {0.0f, 0.0f};
};


struct Weapon {
	Weapon(entt::registry& reg, entt::dispatcher& dispatcher) : reg(reg), dispatcher(dispatcher) {
		dispatcher.sink<PlayerInput>().connect<&Weapon::on_player_input>(this);
	}

	// Copy all of the non-reference attributes from another weapon
	void Clone(const Weapon& other) {
		projectile_count = other.projectile_count;
		level = other.level;
		aim_variance = other.aim_variance;
		damage = other.damage;
		bullet_shape = other.bullet_shape;
		accumulated_power = other.accumulated_power;
		fire_cost = other.fire_cost;
		aim_direction = other.aim_direction;
		position = other.position;
	}

	Weapon(const Weapon& other): reg(other.reg), dispatcher(other.dispatcher) {
		dispatcher.sink<PlayerInput>().connect<&Weapon::on_player_input>(this);
		Clone(other);
	}

	Weapon(Weapon&& other) : reg(other.reg), dispatcher(other.dispatcher) {
		dispatcher.sink<PlayerInput>().connect<&Weapon::on_player_input>(this);
		Clone(other);
	}

	~Weapon() {
		dispatcher.sink<PlayerInput>().disconnect(this);
	}

	void on_player_input(const PlayerInput& input) {
		aim_direction = input.aim_direction;
	}

	virtual void OnUserUpdate(float fElapsedTime) {
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
	virtual void LevelUp(int count) {
		fire_cost *= std::powf(0.99, count);
		projectile_count = static_cast<int>(std::ceilf((level + count) / 3.0f));
		damage *= std::powf(1.05, count);
	}

	void SetPosition(olc::vf2d pos) {
		position = pos;
	}

	ShapePrototypes Shape() const {
		return bullet_shape;
	}
	
private:
	int projectile_count {1};
	int level {1};
	float aim_variance {0.2};
	float damage {10.0f};
	ShapePrototypes bullet_shape {ShapePrototypes::Square};

	// Fires when the accumulated_power goes over the fire_cost
	float accumulated_power {0.0f};
	float fire_cost {0.1f};

	olc::vf2d aim_direction;
	olc::vf2d position;
	entt::registry& reg;
	entt::dispatcher& dispatcher;
	std::mt19937_64 rng{std::random_device{}()};
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

enum class GameState {
	Unknown,
	Menu,
	Gameplay
};

struct System {
	System(entt::registry& reg, olc::PixelGameEngine* pge) : reg(reg), pge(pge) {};

	// Run before any OnUserUpdate functions.  Possibly to reset per-frame data
	virtual void PreUpdate() {};

	// Run once per tick
	virtual void OnUserUpdate(float fElapsedTime) = 0;

	virtual ~System() {};
protected:
	entt::registry& reg;
	olc::PixelGameEngine* pge;
};

struct PhysicsSystem : public System {
	PhysicsSystem(entt::registry& reg, olc::PixelGameEngine* pge) : System(reg, pge) {};

	void PreUpdate() override {
		const auto& view = reg.view<PhysicsComponent>();

		for(auto entity : view) {
			auto& physics = view.get<PhysicsComponent>(entity);
			if(physics.force.mag2() > 0) {
				physics.acceleration *=  0.8f;
				physics.velocity *= physics.friction;
			} else {
				physics.acceleration *=  0.4f;
				physics.velocity *= physics.friction * physics.friction;
			}

			physics.force = {0.0f, 0.0f};
		}
	}

	void OnUserUpdate(float fElapsedTime) override {
		const auto& view = reg.view<PhysicsComponent, Shape>();

		for(auto entity : view) {
			auto& physics = view.get<PhysicsComponent>(entity);
			auto& shape = view.get<Shape>(entity);

			physics.acceleration += physics.force * fElapsedTime;
			physics.velocity += physics.acceleration * fElapsedTime;

			shape.MoveTo(shape.position + physics.velocity * fElapsedTime);
		}
	}
};

struct EnemyMovementSystem : public System {
	EnemyMovementSystem(entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : player_entity(player), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		const auto& view = reg.view<PhysicsComponent, Shape, EnemyComponent>();
		const auto& player_position = reg.get<Shape>(player_entity).position;

		for(auto entity : view) {
			auto& physics = view.get<PhysicsComponent>(entity);
			const auto& s = reg.get<Shape>(entity);
			const auto& dir = player_position - s.position;

			physics.force += dir.norm() * 4500.0f;
		}
	}
private:
	entt::entity player_entity;
};

struct EnemyAttackSystem : public System {
	EnemyAttackSystem(entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : player_entity(player), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		const auto& view = reg.view<PhysicsComponent, Shape, EnemyComponent>();
		const auto& player_shape = reg.get<Shape>(player_entity);

		for(auto entity : view) {
			const auto& s = view.get<Shape>(entity);
			auto& e = view.get<EnemyComponent>(entity);

			e.attack_timer += fElapsedTime;
			
			if((e.attack_timer > e.attack_cooldown) && s.intersects(player_shape)) {
				const auto& dir = player_shape.position - s.position;
				auto& physics = view.get<PhysicsComponent>(entity);
				physics.force += dir.norm() * -450000.0f;

				auto& player = reg.get<PlayerComponent>(player_entity);
				player.health -= e.damage;
				e.attack_timer = 0.0f;
			}
		}
	}
private:
	entt::entity player_entity;
};

struct ParticleSystem : public System {
	ParticleSystem(entt::registry& reg, olc::PixelGameEngine* pge) : System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		const auto& view = reg.view<ParticleComponent, Shape>();

		for(auto entity : view) {
			auto& p = view.get<ParticleComponent>(entity);
			auto& s = view.get<Shape>(entity);

			p.lifespan -= fElapsedTime;

			if(p.lifespan <= 0.0f) {
				reg.destroy(entity);
			} else if (p.lifespan <= p.fade_begin) {
				float alpha = 1.0f - ((p.fade_begin - p.lifespan) / p.fade_begin);
				s.color.a = static_cast<uint8_t>(255 * alpha);
			}
		}
	}
};

struct DrawSystem : public System {
	DrawSystem(entt::registry& reg, olc::PixelGameEngine* pge) : System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		const auto view = reg.view<Shape>();
		view.each([this](entt::entity e, const Shape& s){s.Draw(pge);});
	}
};

struct KeyboardInputSystem : public System {
	KeyboardInputSystem(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

	void PreUpdate() override {
		// Process Inputs and dispatch the PlayerInput message
			olc::vf2d input {};

			if(pge->GetKey(olc::Key::W).bHeld) {
				input.y += -1.0f;
			}
			if(pge->GetKey(olc::Key::S).bHeld) {
				input.y += 1.0f;
			}
			if(pge->GetKey(olc::Key::A).bHeld) {
				input.x += -1.0f;
			}
			if(pge->GetKey(olc::Key::D).bHeld) {
				input.x += 1.0f;
			}

			if(input.x != 0.0f || input.y != 0.0f) {
				input = input.norm();
			}

			const auto& s = reg.get<Shape>(player_entity);
			olc::vf2d ray = (pge->GetMousePos() - s.position).norm();
			PlayerInput player_input;
			player_input.move_direction = input;
			player_input.aim_direction = ray;

			if(pge->GetMouse(0).bPressed) {
				player_input.fire = true;
			}

			dispatcher.enqueue<PlayerInput>(player_input);
	}

	void OnUserUpdate(float fElapsedTime) override {};

private:
	entt::dispatcher& dispatcher;
	entt::entity player_entity;
};

struct EnemySpawnSystem : public System {
	EnemySpawnSystem(entt::dispatcher& dispatcher, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		total_time += fElapsedTime;
		spawn_timer += fElapsedTime;
		accumulated_power += fElapsedTime * std::powf(power_scale, total_time / power_time);

		// If its not time to check spawns, then return immediately
		if(spawn_timer < spawn_rate) {
			return;
		}

		while(accumulated_power > spawn_cost) {
			accumulated_power -= spawn_cost;

			SpawnEnemy spawn;
			spawn.color = olc::YELLOW;
			utilities::random::uniform_real_distribution<float> dist {0, olc::utils::geom2d::pi * 2.0f};
			const float angle = dist(rng);

			// Position the enemy to spawn outside the visiable map area
			spawn.position = olc::vf2d{pge->ScreenWidth() / 1.7f, angle}.cart() + pge->GetScreenSize() / 2.0f;
			dispatcher.enqueue<SpawnEnemy>(spawn);
		}
	}

private:
	entt::dispatcher& dispatcher;
	// Every power_time seconds the enemy power will scale by power_scale
	// This happens smoothly over the duration
	float power_scale {3.0f};
	float power_time {15.0f};
	float accumulated_power {0.0f};
	float spawn_cost {10.0f};
	float total_time {0.0f};

	// Every 5 seconds, attempt to spawn things
	float spawn_timer {0.0f};
	float spawn_rate {5.0f};

	std::mt19937_64 rng{std::random_device{}()};
};

struct BulletSystem : public System {
	BulletSystem(entt::dispatcher& dispatcher, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), System(reg, pge) {};

	void PreUpdate() override {
		// Check if any bullets are off-screen and cull them before processing this frame
		auto view = reg.view<BulletComponent, Shape>();

		for(const auto e : view) {
			const auto& s = reg.get<Shape>(e);
			if ((s.position.x < -10.0f) || (s.position.y < -10.0f) || (s.position.x > pge->ScreenWidth() + 10.0f) || (s.position.y > pge->ScreenHeight() + 10.0f)) {
				reg.destroy(e);
			}
		}
	}

	void OnUserUpdate(float fElapsedTime) override {
		// Check if any enemy is overlapping any bullet and then deal damage
		auto bullet_view = reg.view<BulletComponent, Shape>();
		auto enemy_view = reg.view<EnemyComponent, Shape>();

		for(auto b_entity : bullet_view) {
			auto [b, b_shape] = bullet_view.get(b_entity);

			for(auto e_entity : enemy_view) {
				auto [e, e_shape] = enemy_view.get(e_entity);

				// Check if this is a hit
				if(b_shape.intersects(e_shape)) {
					b.hit_count--;
					e.health -= b.damage;

					if(e.health <= 0) {
						dispatcher.enqueue<EnemyDeath>(e_shape.position);
						reg.destroy(e_entity);
					}

					if (b.hit_count <= 0) {
						reg.destroy(b_entity);
						break;
					}
				}
			}

		}
	}

private:
	entt::dispatcher& dispatcher;
};

struct PlayerWeaponSystem : public System {
	PlayerWeaponSystem(entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : player_entity(player), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		auto& p = reg.get<PlayerComponent>(player_entity);
		auto& s = reg.get<Shape>(player_entity);
		const auto& wp = s.WeaponPoints();

		for(int i = 0; i < p.weapons.size(); i++) {
			p.weapons[i].SetPosition(wp[i]);
			p.weapons[i].OnUserUpdate(fElapsedTime);
		}

		// for(auto& w : p.weapons) {
		// 	w.SetPosition(s.position);
		// 	w.OnUserUpdate(fElapsedTime);
		// }
	}
private:
	entt::entity player_entity;
};

struct ExperienceSystem : public System {
	ExperienceSystem(entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : player_entity(player), System(reg, pge) {};

	// Pickup experience touching the player
	// Pull experience towards the player
	void OnUserUpdate(float fElapsedTime) override {
		const auto& player_shape = reg.get<Shape>(player_entity);
		auto& player_component = reg.get<PlayerComponent>(player_entity);
		auto view = reg.view<ExperienceComponent, Shape, PhysicsComponent>();

		float xp_range2 = player_component.experience_range * player_component.experience_range;
		for(auto entity : view) {
			const auto& s = view.get<Shape>(entity);
			auto& e = view.get<ExperienceComponent>(entity);
			auto& p = view.get<PhysicsComponent>(entity);
			
			e.age += fElapsedTime;
			
			// If the player is touching an experience, pick it up
			if(s.intersects(player_shape)) {
				const auto& xp = view.get<ExperienceComponent>(entity);
				player_component.experience += xp.value;
				reg.destroy(entity);
				continue;
			}

			// If the player is somewhat close to an experience, pull it in
			olc::vf2d distance = s.position - player_shape.position;
			float mag2 = distance.mag2();
			if(mag2 < xp_range2) {
				p.force += distance.norm() * (mag2 - xp_range2);
			}
		}
	}

private:
	entt::entity player_entity;
};

struct LevelUpPickSystem : public System {
	LevelUpPickSystem(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), System(reg, pge) {
		dispatcher.sink<LevelUp>().connect<&LevelUpPickSystem::on_levelup>(this);
	};

	~LevelUpPickSystem() {
		dispatcher.sink<LevelUp>().disconnect(this);
	}

	// Pick the three choices that a player will get
	void on_levelup(const LevelUp& levelup) {
		// Options: improve weapon
		//          add a random weapon
		//          add a weapon slot (if all slots are full)
		//          Improve base in some way
		//          limited choice count
		const auto& p = reg.get<PlayerComponent>(player_entity);
		const auto& ps = reg.get<Shape>(player_entity);

		// Clear the old options from the system
		options.clear();

		// If the player has an open weapon slot, all three choices should be a random weapon
		if(p.weapons.size() < ps.WeaponPointCount()) {
			std::string description = "Add a weapon";
			auto functor = [&](entt::registry& reg, entt::entity e) {
				auto& p = reg.get<PlayerComponent>(e);
				p.weapons.emplace_back(reg, dispatcher);
			};

			for(int i = 0; i < choice_count; i++) {
				options.emplace_back(LevelUpOption{description, functor});
			}
			return;
		}

		if((ps.WeaponPointCount() - 2) < shape_progression.size()) {
			std::string description = "Upgrade Core";
			auto functor = [](entt::registry& reg, entt::entity e) {
				auto& p = reg.get<PlayerComponent>(e);
				auto& s = reg.get<Shape>(e);
				auto& next_shape = prototypes[shape_progression[s.WeaponPoints().size() - 2]];
				s.SetPrototype(next_shape);
				//reg.erase<Shape>(e);
				//reg.emplace<Shape>(e, s, next_shape);
				//p.max_weapon_count += 1;
			};

			options.emplace_back(LevelUpOption{description, functor});
		}

		// Fill the rest of the options with "improve weapon"
		for(int i = options.size(); i < choice_count; i++) {
			int weapon_slot = rand() % p.weapons.size();
			std::string description = "Improve weapon " + std::to_string(weapon_slot + 1);
			auto functor = [weapon_slot](entt::registry& reg, entt::entity e) {
				auto& p = reg.get<PlayerComponent>(e);
				p.weapons[weapon_slot].LevelUp(1);
			};
			options.emplace_back(LevelUpOption{description, functor});
		}
	}

	void OnUserUpdate(float fElapsedTime) override {
		olc::vf2d pos {100.0f, 300.0f};
		for(auto & o : options) {
			pge->DrawStringDecal(pos, o.description, olc::WHITE, {3.0f, 3.0f});
			pos.y += 30.0f;
		}

		if(pge->GetKey(olc::Key::K1).bPressed) {
			dispatcher.enqueue(options[0]);
		}

		if(pge->GetKey(olc::Key::K2).bPressed) {
			dispatcher.enqueue(options[1]);
		}

		if(pge->GetKey(olc::Key::K3).bPressed) {
			dispatcher.enqueue(options[2]);
		}
	}

private:
	entt::dispatcher& dispatcher;
	entt::entity player_entity;
	int choice_count {3};
	std::vector<LevelUpOption> options;
};

// Track player state and issue related events
struct PlayerStateSystem : public System {
	PlayerStateSystem(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		auto& p = reg.get<PlayerComponent>(player_entity)		;

		if(p.experience >= p.xp_to_next_level) {
			p.experience -= p.xp_to_next_level;
			p.xp_to_next_level *= 1.1f;

			// Indicate that the player has leveled up
			dispatcher.enqueue(LevelUp{});
		}
	}
private:
	entt::dispatcher& dispatcher;
	entt::entity player_entity;
};

struct State {
	olc::PixelGameEngine* pge;

	State(olc::PixelGameEngine* pge) : pge(pge) {};

	virtual ~State() = default;
	virtual void EnterState() {};
	virtual GameState OnUserUpdate(float fElapsedTime) = 0;
	virtual void ExitState() {};
};

struct MenuState : public State {
	MenuState(olc::PixelGameEngine* pge) : State(pge) {};
	GameState OnUserUpdate(float fElapsedTime) {
		GameState next_state = GameState::Menu;

		if(pge->GetMouse(0).bPressed) {
			next_state = GameState::Gameplay;
		}

		return next_state;
	}
};

struct GameplayState : public State {
	enum class SubState {
		Normal,
		LevelUpScreen
	};

	entt::entity player_entity;

	float fTotalTime {0.0f};
	float enemyTimer {0.0f};
	float player_health {10.0f};
	float enemy_speed {100.0f};
	float player_speed {120.0f};
	float enemy_threshold {10.0f};
	float enemy_cost {10.0f};
	float score {0.0f};

	entt::registry reg;
	entt::dispatcher dispatcher;

	std::mt19937_64 rng;

	struct enemy_death{};

	std::unique_ptr<System> physics_system;
	std::unique_ptr<System> enemy_movement_system;
	std::unique_ptr<System> draw_system;
	std::unique_ptr<System> input_system;
	std::unique_ptr<System> spawn_enemy_system;
	std::unique_ptr<System> bullet_system;
	std::unique_ptr<System> particle_system;
	std::unique_ptr<System> enemy_attack_system;
	std::unique_ptr<System> player_weapons_system;
	std::unique_ptr<System> experience_system;
	std::unique_ptr<System> player_state_system;
	std::unique_ptr<System> levelup_pick_system;

	SubState previous_state;
	SubState current_state;
	SubState next_state;

	explicit GameplayState(olc::PixelGameEngine* pge) : State(pge) {
		rng.seed(std::random_device{}());
	}

	void tickEnemyTimer() {
		auto entity = reg.create();
		auto& s = reg.emplace<Shape>(entity, prototypes[ShapePrototypes::Square]);
		s.color = olc::YELLOW;

		utilities::random::uniform_real_distribution<float> dist {0, olc::utils::geom2d::pi * 2.0f};
		const float angle = dist(rng);

		// Position the enemy to spawn outside the visiable map area
		olc::vf2d position = olc::vf2d{pge->ScreenWidth() / 1.7f, angle}.cart() + pge->GetScreenSize() / 2.0f;

		s.MoveTo(position);
		reg.emplace<EnemyComponent>(entity);
		reg.emplace<PhysicsComponent>(entity);
	}

	void spawnBullet(olc::vf2d pos, olc::vf2d vel, ShapePrototypes type = ShapePrototypes::Square) {
		SpawnBullet spawn;
		spawn.position = pos;
		spawn.damage = 10.0f;
		spawn.duration = 10.0f;
		spawn.hit_count = 1;
		spawn.initial_velocity = vel * player_speed * 1.6f;
		spawn.angular_velocity = 14.0f;
		spawn.scale = 0.4f;
		spawn.shape = type;
		//dispatcher.enqueue(spawn);
	}

	void spawnParticle(olc::vf2d pos, olc::vf2d vel, olc::Pixel color = olc::YELLOW, ShapePrototypes type = ShapePrototypes::Triangle) {
		auto entity = reg.create();
		auto& s = reg.emplace<Shape>(entity, prototypes[type]);
		s.MoveTo(pos);
		s.color = color;
		utilities::random::uniform_real_distribution<float> dist {0, olc::utils::geom2d::pi * 2.0f};
		s.theta = dist(rng);
		reg.emplace<ParticleComponent>(entity);
		auto& p = reg.emplace<PhysicsComponent>(entity);
		p.force = vel.norm() * 600000.0f;
	}

	// Event responding to an enemy death
	void on_enemy_death(const EnemyDeath& e) {
		score += 1.0f;

		for(int i = 0; i < 4; i++) {
			utilities::random::uniform_real_distribution<float> dist {0, olc::utils::geom2d::pi * 2.0f};
			const float angle = dist(rng);
			spawnParticle(e.position, olc::vf2d{1.0, angle}.cart(), RandomColor(), static_cast<ShapePrototypes>(rand() % 9));
		}

		// Spawn experience
		SpawnExperience spawn;
		spawn.position = e.position;
		spawn.value = 1.0f;
		spawn.age = 0.0f;
		dispatcher.enqueue(spawn);
	}

	// Event responding to certain player input
	void on_player_input(const PlayerInput& input) {
		const auto& s = reg.get<Shape>(player_entity);
		auto& p = reg.get<PhysicsComponent>(player_entity);

		p.force = input.move_direction * 5000.0f;

		// if(input.fire) {
		// 	spawnBullet(s.position, input.aim_direction);
		// }
	}

	void on_spawn_enemy(const SpawnEnemy& spawn)  {
		auto entity = reg.create();
		auto& s = reg.emplace<Shape>(entity, prototypes[ShapePrototypes::Square]);
		s.color = spawn.color;
		s.scale = 2.0f;

		s.MoveTo(spawn.position);
		reg.emplace<EnemyComponent>(entity);
		reg.emplace<PhysicsComponent>(entity);		
	}

	void on_bullet_spawn(const SpawnBullet& spawn) {
		auto entity = reg.create();
		auto& s = reg.emplace<Shape>(entity, prototypes[spawn.shape]);
		s.MoveTo(spawn.position);
		s.theta = spawn.initial_velocity.y;
		s.scale = spawn.scale;
		auto& b = reg.emplace<BulletComponent>(entity, spawn);
		b.damage = spawn.damage;
		
		auto& p = reg.emplace<PhysicsComponent>(entity);
		p.velocity = spawn.initial_velocity;
		p.angular_velocity = spawn.angular_velocity;
		p.friction = 1.0;
	}

	void on_experience_spawn(const SpawnExperience& spawn) {
		auto entity = reg.create();
		auto& s = reg.emplace<Shape>(entity, prototypes[ShapePrototypes::Triangle]);
		auto& p = reg.emplace<PhysicsComponent>(entity);
		s.MoveTo(spawn.position);
		s.theta = spawn.position.y;
		s.scale = 0.4f * (1.0f + 0.1f * spawn.value);

		// Generate a random green color
		s.color.g = 192 + (rand() % 64);
		s.color.b = rand() % 128;
		s.color.r = rand() % 128;

		reg.emplace<ParticleComponent>(entity, 30.0f, 20.0f);
		reg.emplace<ExperienceComponent>(entity, spawn.value, spawn.age);
	}

	void on_levelup(const LevelUp& levelup) {
		next_state = SubState::LevelUpScreen;
	}

	void on_levelup_option(const LevelUpOption& option) {
		option.functor(reg, player_entity);
		next_state = SubState::Normal;
	}

	void EnterState() override {
		// Link events to their handlers
		dispatcher.sink<EnemyDeath>().connect<&GameplayState::on_enemy_death>(this);
		dispatcher.sink<PlayerInput>().connect<&GameplayState::on_player_input>(this);
		dispatcher.sink<SpawnEnemy>().connect<&GameplayState::on_spawn_enemy>(this);
		dispatcher.sink<SpawnBullet>().connect<&GameplayState::on_bullet_spawn>(this);
		dispatcher.sink<SpawnExperience>().connect<&GameplayState::on_experience_spawn>(this);
		dispatcher.sink<LevelUp>().connect<&GameplayState::on_levelup>(this);
		dispatcher.sink<LevelUpOption>().connect<&GameplayState::on_levelup_option>(this);
		
		score = 0.0f;

		// Create the player
		player_entity = reg.create();
		auto& p = reg.emplace<PlayerComponent>(player_entity);
		p.weapons.emplace_back(reg, dispatcher);
		auto& s = reg.emplace<Shape>(player_entity, prototypes[ShapePrototypes::Triangle]);
		s.MoveTo(pge->GetScreenSize() / 2.0f);
		s.scale = 4.0f;
		reg.emplace<PhysicsComponent>(player_entity);
		
		// Create all the systems that will be run
		physics_system = std::make_unique<PhysicsSystem>(reg, pge);
		enemy_movement_system = std::make_unique<EnemyMovementSystem>(player_entity, reg, pge);
		draw_system = std::make_unique<DrawSystem>(reg, pge);
		input_system = std::make_unique<KeyboardInputSystem>(dispatcher, player_entity, reg, pge);
		spawn_enemy_system = std::make_unique<EnemySpawnSystem>(dispatcher, reg, pge);
		bullet_system = std::make_unique<BulletSystem>(dispatcher, reg, pge);
		particle_system = std::make_unique<ParticleSystem>(reg, pge);
		enemy_attack_system = std::make_unique<EnemyAttackSystem>(player_entity, reg, pge);
		player_weapons_system = std::make_unique<PlayerWeaponSystem>(player_entity, reg, pge);
		experience_system = std::make_unique<ExperienceSystem>(player_entity, reg, pge);
		player_state_system = std::make_unique<PlayerStateSystem>(dispatcher, player_entity, reg, pge);
		levelup_pick_system = std::make_unique<LevelUpPickSystem>(dispatcher, player_entity, reg, pge);
	}

	GameState OnUserUpdate(float fElapsedTime) override {
		if(fElapsedTime > 1.0f/60.0f) {
			fElapsedTime = 1.0f/60.0f;
		}
		if(next_state != current_state) {
			previous_state = current_state;
			current_state = next_state;
		}

		if(pge->GetKey(olc::Key::SPACE).bPressed) {
			if(next_state == SubState::Normal) {
				next_state = SubState::LevelUpScreen;
			} else {
				next_state = SubState::Normal;
			}
		}

		// PreUpdates can probably always be run regardless of state
		physics_system->PreUpdate();
		enemy_movement_system->PreUpdate();
		draw_system->PreUpdate();
		input_system->PreUpdate();
		spawn_enemy_system->PreUpdate();
		bullet_system->PreUpdate();
		particle_system->PreUpdate();
		enemy_attack_system->PreUpdate();
		player_weapons_system->PreUpdate();
		experience_system->PreUpdate();
		player_state_system->PreUpdate();
		levelup_pick_system->PreUpdate();

		dispatcher.update();

		if(current_state == SubState::Normal) {
			enemy_movement_system->OnUserUpdate(fElapsedTime);
			enemy_attack_system->OnUserUpdate(fElapsedTime);
			player_weapons_system->OnUserUpdate(fElapsedTime);
			experience_system->OnUserUpdate(fElapsedTime);
			physics_system->OnUserUpdate(fElapsedTime/2.0f);
			physics_system->OnUserUpdate(fElapsedTime/2.0f);
			input_system->OnUserUpdate(fElapsedTime);
			spawn_enemy_system->OnUserUpdate(fElapsedTime);
			bullet_system->OnUserUpdate(fElapsedTime);
			particle_system->OnUserUpdate(fElapsedTime);
			player_state_system->OnUserUpdate(fElapsedTime);
		}

		if(current_state == SubState::LevelUpScreen) {
			levelup_pick_system->OnUserUpdate(fElapsedTime);
		}

		draw_system->OnUserUpdate(fElapsedTime);

		fTotalTime += fElapsedTime;

		pge->Clear(olc::VERY_DARK_GREY);

		{
			pge->DrawStringDecal({10.0f, 10.0f}, std::format("Score : {}", score), olc::WHITE, olc::vf2d{3.0f, 3.0f});
			pge->DrawStringDecal({10.0f, 40.0f}, std::format("Timer : {}", fTotalTime), olc::WHITE, olc::vf2d{3.0f, 3.0f});
			pge->DrawStringDecal({10.0f, 70.0f}, std::format("Level : {}", reg.get<PlayerComponent>(player_entity).level), olc::WHITE, olc::vf2d{3.0f, 3.0f});
			pge->DrawStringDecal({10.0f, 100.0f}, std::format("Health: {}", reg.get<PlayerComponent>(player_entity).health), olc::WHITE, olc::vf2d{3.0f, 3.0f});
			pge->DrawStringDecal({10.0f, 130.0f}, std::format("Xp    : {}", reg.get<PlayerComponent>(player_entity).experience), olc::WHITE, olc::vf2d{3.0f, 3.0f});

			// pge->DrawStringDecal({10.0f, 70.0f}, std::format("Enemy: {}", enemyTimer), olc::WHITE, olc::vf2d{3.0f, 3.0f});
			// pge->DrawStringDecal({10.0f, 100.0f}, std::format("Power: {}", 1 + std::powf(2, fTotalTime / 40.0f)), olc::WHITE, olc::vf2d{3.0f, 3.0f});
		}
		return GameState::Gameplay;
	}
};



class Jam2025Shapes : public olc::PixelGameEngine
{
public:
	std::unique_ptr<Shape> player;
	std::unique_ptr<Shape> cursor;
	std::unique_ptr<Shape> test_shape;

	float fTotalTime {0.0f};
	float enemyTimer {0.0f};
	float player_speed {80.0f};

	entt::registry reg;

	GameplayState gameplay {this};

	Jam2025Shapes()
	{
		sAppName = "Shapes";

	}

	std::map<GameState, std::unique_ptr<State>> game_states;
	GameState previous_state = GameState::Menu;
	GameState current_state = GameState::Menu;
	GameState next_state = GameState::Menu;

public:
	bool OnUserCreate() override
	{
		game_states.insert(std::make_pair(GameState::Menu, std::make_unique<MenuState>(this)));
		game_states.insert(std::make_pair(GameState::Gameplay, std::make_unique<GameplayState>(this)));

		// Generate the prototypes
		// Prototype triangle_proto;
		// triangle_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{0, -8}, {8, 8}, {-8, 8}});
		// prototypes.insert({ShapePrototypes::Triangle, triangle_proto});

		// Prototype square_proto;
		// square_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{-8, -8}, {8, -8}, {-8, 8}});
		// square_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{8, -8}, {8, 8}, {-8, 8}});
		// prototypes.insert({ShapePrototypes::Square, square_proto});
		
		// Create the cursor shape
		Prototype cursor_proto;
		cursor_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{0, -8}, {8, 8}, {0, 0}});
		cursor_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{0, -8}, {0, 0}, {-8, 8}});
		// Cursor proto has no weapon points as it is only used for projectiles
		prototypes.insert({ShapePrototypes::Cursor, cursor_proto});

		// Create the star52 shape and the pentagon shape
		Prototype star52_proto;
		Prototype pentagon_proto;
		{
			std::array<olc::vf2d, 5> outer_points;

			for(int i = 0; i < 5; i++) {
				float outer_angle = ((90.0f - (72.0f * i)) * olc::utils::geom2d::pi) / 180.0f;
				outer_points[i] = olc::vf2d{-8.0f * std::cosf(outer_angle), -8.0f * std::sinf(outer_angle)};

				// Both the pentagon and star52 shapes have the same weapon mount points
				star52_proto.weapon_points.push_back(outer_points[i]);
				pentagon_proto.weapon_points.push_back(outer_points[i]);
			}

			star52_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{outer_points[0]}, {outer_points[2]}, {0.0f, 0.0f}});
			star52_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{outer_points[1]}, {outer_points[3]}, {0.0f, 0.0f}});
			star52_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{outer_points[2]}, {outer_points[4]}, {0.0f, 0.0f}});
			star52_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{outer_points[3]}, {outer_points[0]}, {0.0f, 0.0f}});
			star52_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{outer_points[4]}, {outer_points[1]}, {0.0f, 0.0f}});

			pentagon_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{outer_points[0]}, {outer_points[1]}, {0.0f, 0.0f}});
			pentagon_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{outer_points[1]}, {outer_points[2]}, {0.0f, 0.0f}});
			pentagon_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{outer_points[2]}, {outer_points[3]}, {0.0f, 0.0f}});
			pentagon_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{outer_points[3]}, {outer_points[4]}, {0.0f, 0.0f}});
			pentagon_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{outer_points[4]}, {outer_points[0]}, {0.0f, 0.0f}});

			prototypes.insert({ShapePrototypes::Star5_2, star52_proto});
			prototypes.insert({ShapePrototypes::Pentagon, pentagon_proto});
		}

		// Create the star62 shape and the triangle shape since they share some points
		Prototype star62_proto;
		Prototype triangle_proto;
		{
			std::array<olc::vf2d, 6> points;
			for(int i = 0; i < 6; i++) {
				float angle = ((90.0f - (60.0f * i)) * olc::utils::geom2d::pi) / 180.0f;
				points[i] = olc::vf2d{-8.0f * std::cosf(angle), -8.0f * std::sinf(angle)};

				// Add the weapon points, but only half to the triangle
				star62_proto.weapon_points.push_back(points[i]);
				if((i % 2) == 0) {
					triangle_proto.weapon_points.push_back(points[i]);
				}
			}

			star62_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[0], points[2], points[4]});
			star62_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[1], points[3], points[5]});
			
			triangle_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[0], points[2], points[4]});

			prototypes.insert({ShapePrototypes::Star6_2, star62_proto});
			prototypes.insert({ShapePrototypes::Triangle, triangle_proto});
		}

		// Create the star82 shape and the square since they share some points
		Prototype star82_proto;
		Prototype square_proto;
		{
			std::array<olc::vf2d, 8> points;
			for(int i = 0; i < 8; i++) {
				float angle = ((90.0f - (45.0f * i)) * olc::utils::geom2d::pi) / 180.0f;
				points[i] = olc::vf2d{-8.0f * std::cosf(angle), -8.0f * std::sinf(angle)};

				star82_proto.weapon_points.push_back(points[i]);
				if((i % 2) == 1) {
					square_proto.weapon_points.push_back(points[i]);
				}
			}

			star82_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[0], points[2], points[4]});
			star82_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[1], points[3], points[5]});
			star82_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[4], points[6], points[0]});
			star82_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[5], points[7], points[1]});

			square_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[1], points[3], points[5]});
			square_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[5], points[7], points[1]});
			
			prototypes.insert({ShapePrototypes::Star8_2, star82_proto});
			prototypes.insert({ShapePrototypes::Square, square_proto});
		}

		// Create the star93 shape
		Prototype star93_proto;
		{
			std::array<olc::vf2d, 9> points;
			for(int i = 0; i < 9; i++) {
				float angle = ((90.0f - (40.0f * i)) * olc::utils::geom2d::pi) / 180.0f;
				points[i] = olc::vf2d{-8.0f * std::cosf(angle), -8.0f * std::sinf(angle)};

				star93_proto.weapon_points.push_back(points[i]);
			}

			star93_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[0], points[3], points[6]});
			star93_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[1], points[4], points[7]});
			star93_proto.tris.push_back(olc::utils::geom2d::triangle<float>{points[2], points[5], points[8]});
			prototypes.insert({ShapePrototypes::Star9_3, star93_proto});
	
		}

		Prototype star73_proto;
		{
			std::array<olc::vf2d, 7> points;
			for(int i = 0; i < 7; i++) {
				float angle = ((90.0f - ((360.0f/7.0f) * i)) * olc::utils::geom2d::pi) / 180.0f;
				points[i] = olc::vf2d{-8.0f * std::cosf(angle), -8.0f * std::sinf(angle)};

				star73_proto.weapon_points.push_back(points[i]);
			}

			star73_proto.tris.push_back(olc::utils::geom2d::triangle<float>(points[0], points[3], {0.0f, 0.0f}));
			star73_proto.tris.push_back(olc::utils::geom2d::triangle<float>(points[1], points[4], {0.0f, 0.0f}));
			star73_proto.tris.push_back(olc::utils::geom2d::triangle<float>(points[2], points[5], {0.0f, 0.0f}));
			star73_proto.tris.push_back(olc::utils::geom2d::triangle<float>(points[3], points[6], {0.0f, 0.0f}));
			star73_proto.tris.push_back(olc::utils::geom2d::triangle<float>(points[4], points[0], {0.0f, 0.0f}));
			star73_proto.tris.push_back(olc::utils::geom2d::triangle<float>(points[5], points[1], {0.0f, 0.0f}));
			star73_proto.tris.push_back(olc::utils::geom2d::triangle<float>(points[6], points[2], {0.0f, 0.0f}));
			prototypes.insert({ShapePrototypes::Star7_3, star73_proto});
		}

		return true;
	}


	bool OnUserUpdate(float fElapsedTime) override
	{
		const auto& state = game_states.at(current_state);

		if (current_state != previous_state) {
			state->EnterState();
		}

		next_state = state->OnUserUpdate(fElapsedTime);

		if (next_state != current_state) {
			state->ExitState();
		}

		previous_state = current_state;
		current_state = next_state;


		return true;
	}
};


int main()
{
	Jam2025Shapes demo;
	if (demo.Construct(1280, 960, 1, 1, false, true))
		demo.Start();

	return 0;
}
