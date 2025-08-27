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

#include "components.hpp"
#include "events.hpp"
#include "shape.hpp"
#include "state.hpp"

#include "systems/system.hpp"
#include "systems/physics.hpp"
#include "systems/enemy_movement.hpp"
#include "systems/enemy_attack.hpp"
#include "systems/particle.hpp"
#include "systems/draw.hpp"
#include "systems/keyboard_input.hpp"
#include "systems/enemy_spawn.hpp"
#include "systems/bullet.hpp"
#include "systems/experience.hpp"
#include "systems/player_weapon.hpp"
#include "systems/levelup_pick.hpp"
#include "systems/player_state.hpp"
#include "systems/boss_timer.hpp"
#include "systems/boss/boss.hpp"

#include "weapons/weapon.hpp"

#include <vector>
#include <random>


using namespace entt::literals;


// Players start as a triangle with 3 weapon slots and gain one slot per progression until 9
std::array<ShapePrototypes, 7> shape_progression {ShapePrototypes::Triangle, ShapePrototypes::Square, ShapePrototypes::Pentagon, ShapePrototypes::Star6_2, ShapePrototypes::Star7_3, ShapePrototypes::Star8_2, ShapePrototypes::Star9_3};

// Map of the potential prototypes
std::map<ShapePrototypes, Prototype> prototypes;

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
		LevelUpScreen,
		BossLeadIn, // A state where the boss is "entering" the fight
		Boss, // Boss battle state
		BossLeadOut,
		Pause
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
	std::unique_ptr<System> boss_timer_system;

	std::unique_ptr<System> boss_lead_in_system;
	std::unique_ptr<System> boss_system;
	std::unique_ptr<System> boss_lead_out_system;

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
        //std::cout << "Kill at " << e.position.str() << std::endl;
		
		score += 1.0f;

		for(int i = 0; i < 4; i++) {
			utilities::random::uniform_real_distribution<float> dist {0, olc::utils::geom2d::pi * 2.0f};
			const float angle = dist(rng);
			spawnParticle(e.position, olc::vf2d{1.0, angle}.cart(), utilities::RandomColor(), static_cast<ShapePrototypes>(rand() % 9));
		}

		// Spawn experience
		if(e.gives_xp) {
			SpawnExperience spawn;
			spawn.position = e.position;
			spawn.value = 1.0f;
			spawn.age = 0.0f;
			dispatcher.enqueue(spawn);
		}
	}

	// Event responding to certain player input
	void on_player_input(const PlayerInput& input) {
		const auto& s = reg.get<Shape>(player_entity);
		auto& p = reg.get<PhysicsComponent>(player_entity);

		p.force = input.move_direction * 5000.0f;
	}

	void on_spawn_enemy(const SpawnDescriptor& spawn)  {
		// Find a random position to spawn enemies near
		utilities::random::uniform_real_distribution<float> dist {0, olc::utils::geom2d::pi * 2.0f};
		utilities::random::uniform_real_distribution<float> dist_a {0.0f, 50.0f};
		
		const float angle = dist(rng);
		const olc::vf2d main_position = olc::vf2d{pge->ScreenWidth() / 1.7f, angle}.cart() + pge->GetScreenSize() / 2.0f;

		for(int i = 0; i < spawn.count; i++) {
			auto entity = reg.create();
			auto& s = reg.emplace<Shape>(entity, prototypes[spawn.type]);
			s.color = spawn.color;
			s.scale = spawn.scale;

			olc::vf2d jitter {dist_a(rng), dist_a(rng)};
			s.MoveTo(main_position + jitter);

			auto& e = reg.emplace<EnemyComponent>(entity);
			e.health = spawn.health;
			auto& p = reg.emplace<PhysicsComponent>(entity);
			p.mass = spawn.mass;

		}
	}

	void on_bullet_spawn(const SpawnBullet& spawn) {
		auto entity = reg.create();
		auto& s = reg.emplace<Shape>(entity, prototypes[spawn.shape]);
		s.MoveTo(spawn.position);
		s.theta = spawn.initial_velocity.y;
		s.scale = spawn.scale;
		s.color = spawn.color;
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
		next_state = previous_state;
	}

	void on_spawn_boss(const SpawnBoss& spawn) {
		// Once the boss spawn is triggered, proceed to the boss lead in and choose a boss to spawn
		next_state = SubState::BossLeadIn;

		const auto& factory = BigChungusFactory(dispatcher, player_entity, reg, pge);
		boss_lead_in_system = factory.GetLeadInSystem(spawn.power);
		boss_system = factory.GetBossSystem(spawn.power);
		boss_lead_out_system = factory.GetLeadOutSystem(spawn.power);

		std::cout << "Boss Time" << std::endl;
	}

	void on_boss_main(const BeginBossMain& boss) {
		next_state = SubState::Boss;
		std::cout << "Begin Fight" << std::endl;
	}

	void on_boss_kill(const BossKill& kill) {
		next_state = SubState::BossLeadOut;
		std::cout << "Boss Dead" << std::endl;

		auto& s = reg.get<Shape>(player_entity);

		// If the player is eligible for an upgrade, grant it
		if(s.WeaponPointCount() - 2 < shape_progression.size()) {
			auto& next_shape = prototypes[shape_progression[s.WeaponPoints().size() - 2]];
			s.SetPrototype(next_shape);
		}
	}

	void on_boss_phase_done(const BossPhaseDone& phase) {
		next_state = SubState::Normal;
		std::cout << "return to Normal" << std::endl;

	}

	void EnterState() override {
		// Link events to their handlers
		dispatcher.sink<EnemyDeath>().connect<&GameplayState::on_enemy_death>(this);
		dispatcher.sink<PlayerInput>().connect<&GameplayState::on_player_input>(this);
		dispatcher.sink<SpawnDescriptor>().connect<&GameplayState::on_spawn_enemy>(this);
		dispatcher.sink<SpawnBullet>().connect<&GameplayState::on_bullet_spawn>(this);
		dispatcher.sink<SpawnExperience>().connect<&GameplayState::on_experience_spawn>(this);
		dispatcher.sink<LevelUp>().connect<&GameplayState::on_levelup>(this);
		dispatcher.sink<LevelUpOption>().connect<&GameplayState::on_levelup_option>(this);
		
		// Boss related events
		dispatcher.sink<SpawnBoss>().connect<&GameplayState::on_spawn_boss>(this);
		dispatcher.sink<BeginBossMain>().connect<&GameplayState::on_boss_main>(this);
		dispatcher.sink<BossKill>().connect<&GameplayState::on_boss_kill>(this);
		dispatcher.sink<BossPhaseDone>().connect<&GameplayState::on_boss_phase_done>(this);
		
		score = 0.0f;

		// Create the player
		player_entity = reg.create();
		auto& p = reg.emplace<PlayerComponent>(player_entity);
		p.weapons.emplace_back(reg, dispatcher, PierceWeapon);
		auto& s = reg.emplace<Shape>(player_entity, prototypes[ShapePrototypes::Triangle]);
		s.MoveTo(pge->GetScreenSize() / 2.0f);
		s.scale = 4.0f;
		reg.emplace<PhysicsComponent>(player_entity);
		
		// Create all the systems that will be run
		physics_system = std::make_unique<PhysicsSystem>(reg, pge);
		enemy_movement_system = std::make_unique<EnemyMovementSystem>(player_entity, reg, pge);
		draw_system = std::make_unique<DrawSystem>(player_entity, reg, pge);
		input_system = std::make_unique<KeyboardInputSystem>(dispatcher, player_entity, reg, pge);
		spawn_enemy_system = std::make_unique<EnemySpawnSystem>(dispatcher, reg, pge);
		bullet_system = std::make_unique<BulletSystem>(dispatcher, reg, pge);
		particle_system = std::make_unique<ParticleSystem>(reg, pge);
		enemy_attack_system = std::make_unique<EnemyAttackSystem>(player_entity, reg, pge);
		player_weapons_system = std::make_unique<PlayerWeaponSystem>(player_entity, reg, pge);
		experience_system = std::make_unique<ExperienceSystem>(player_entity, reg, pge);
		player_state_system = std::make_unique<PlayerStateSystem>(dispatcher, player_entity, reg, pge);
		levelup_pick_system = std::make_unique<LevelUpPickSystem>(dispatcher, player_entity, reg, pge);
		boss_timer_system = std::make_unique<BossTimerSystem>(dispatcher, reg, pge);
	}

	GameState OnUserUpdate(float fElapsedTime) override {
		pge->Clear(olc::VERY_DARK_GREY);

		if(fElapsedTime > 1.0f/60.0f) {
			fElapsedTime = 1.0f/60.0f;
		}

		if(next_state != current_state) {
			previous_state = current_state;
			current_state = next_state;
		}

		if(pge->GetKey(olc::Key::SPACE).bPressed) {
			if(next_state != SubState::Pause) {
				next_state = SubState::Pause;
			} else {
				next_state = previous_state;
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
		boss_timer_system->PreUpdate();

		// If we're in a boss state, perform the expected pre-updates


		dispatcher.update();

		if(current_state != SubState::LevelUpScreen) {
			enemy_movement_system->OnUserUpdate(fElapsedTime);
			enemy_attack_system->OnUserUpdate(fElapsedTime);
			experience_system->OnUserUpdate(fElapsedTime);
			physics_system->OnUserUpdate(fElapsedTime/2.0f);
			physics_system->OnUserUpdate(fElapsedTime/2.0f);
			player_weapons_system->OnUserUpdate(fElapsedTime);
			input_system->OnUserUpdate(fElapsedTime);
			// Only spawn enemies if we're not in a boss state, other the boss mechanics will take care of this
			if(current_state == SubState::Normal) {
				spawn_enemy_system->OnUserUpdate(fElapsedTime);
				boss_timer_system->OnUserUpdate(fElapsedTime);
			}

			if(current_state == SubState::BossLeadIn) {
				boss_lead_in_system->OnUserUpdate(fElapsedTime);
			}

			if(current_state == SubState::Boss) {
				boss_system->OnUserUpdate(fElapsedTime);
			}

			if(current_state == SubState::BossLeadOut) {
				boss_lead_out_system->OnUserUpdate(fElapsedTime);
			}

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
		int game_layer = CreateLayer();
		EnableLayer(game_layer, true);
		SetDrawTarget(static_cast<uint8_t>(game_layer));

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
		cursor_proto.type = ShapePrototypes::Cursor; 
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


			star52_proto.type = ShapePrototypes::Star5_2;
			pentagon_proto.type = ShapePrototypes::Pentagon;
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

			star62_proto.type = ShapePrototypes::Star6_2;
			triangle_proto.type = ShapePrototypes::Triangle; 
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
			
			star82_proto.type = ShapePrototypes::Star8_2;
			square_proto.type = ShapePrototypes::Square;
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

			star93_proto.type = ShapePrototypes::Star9_3;
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

			star73_proto.type = ShapePrototypes::Star7_3;
			prototypes.insert({ShapePrototypes::Star7_3, star73_proto});
		}

		return true;
	}


	bool OnUserUpdate(float fElapsedTime) override
	{
		//auto* target = GetDrawTarget();
		SetDrawTarget(static_cast<uint8_t>(0));
		Clear(olc::BLANK);
		SetDrawTarget(static_cast<uint8_t>(1));
		Clear(olc::BLANK);

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
