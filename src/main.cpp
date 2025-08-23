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
};

struct Shape {
	using iterator = std::vector<olc::utils::geom2d::triangle<float>>::iterator;
	using const_iterator = std::vector<olc::utils::geom2d::triangle<float>>::const_iterator;

	Shape(const Prototype& other) : tris(other.tris), prototype(other) {
		
	}

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

	void Draw(olc::PixelGameEngine* pge) const {
		for (const auto& t : tris) {
			pge->FillTriangleDecal(t.pos[0], t.pos[1], t.pos[2], color);
		}
    }

	void MoveTo(olc::vf2d new_position) {
		position = new_position;
		const auto sc = olc::vf2d{std::sinf(theta), std::cosf(theta)};
		const float s = std::sin(theta);
		const float c = std::cos(theta);
		tris.clear();

		for(const auto& t : prototype) {
			tris.push_back(
				{
					(rotate(t.pos[0], sc) * scale) + position,
				 	(rotate(t.pos[1], sc) * scale) + position,
				 	(rotate(t.pos[2], sc) * scale) + position
				}
			);
		}
	}

	bool intersects(const Shape& other) {
		for(const auto& t : other) {
			for(const auto& self_t : tris) {
				if(olc::utils::geom2d::overlaps(t, self_t)) {
					return true;
				}
			}
		}

		return false;
	};



	float scale {1.0f};
	float theta {0.0f};
	olc::vf2d position {0.0f, 0.0f};
	olc::Pixel color {olc::MAGENTA};

	virtual ~Shape() = default;
protected:
	std::vector<olc::utils::geom2d::triangle<float>> tris;
	const Prototype& prototype;
};

struct Enemy : public Shape {
	Enemy(const Prototype& proto) : Shape(proto) {};

	float health {10.0f};
	olc::vf2d velocity {0.0f, 0.0f};
};

// struct Bullet : public Shape {
// 	Bullet(const Prototype& proto) : Shape(proto) {};

// 	int hit_count {1};
// 	float damage {10.0f};
// 	olc::vf2d velocity {0.0f, 0.0f};
// };


olc::vi2d print(olc::utils::geom2d::triangle<float>& tri, olc::vi2d pos, olc::PixelGameEngine* pge) {
	pge->DrawString(pos, tri.pos[0].str());
	pos.y += 10;
	pge->DrawString(pos, tri.pos[1].str());
	pos.y += 10;
	pge->DrawString(pos, tri.pos[2].str());
	pos.y += 10;

	return pos;
};

using Velocity = olc::vf2d;

struct EnemyComponent {
	float health {10.0f};
	float damage {1.0f};
	olc::vf2d velocity {0.0f, 0.0f};
};

struct BulletComponent {
	olc::vf2d velocity {};
	float damage {10.0f};
	int hit_count {1};
	float duration {10.0f};
	float angular_velocity {14.0};
};

enum class GameState {
	Unknown,
	Menu,
	Gameplay
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
	std::unique_ptr<Shape> player;
	//std::unique_ptr<Shape> cursor;
	//std::unique_ptr<Shape> test_shape;
	Prototype square_proto;
	Prototype cursor_proto;

	float fTotalTime {0.0f};
	float enemyTimer {0.0f};
	float player_health {10.0f};
	float player_speed {80.0f};
	float enemy_threshold {10.0f};
	float enemy_cost {10.0f};
	float score {0.0f};

	entt::registry reg;
	entt::dispatcher dispatcher;

	std::mt19937_64 rng;

	struct enemy_death{};

	explicit GameplayState(olc::PixelGameEngine* pge) : State(pge) {
		// Create the square
		square_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{-8, -8}, {8, -8}, {-8, 8}});
		square_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{8, -8}, {8, 8}, {-8, 8}});
		
		// Create the cursor shape
		cursor_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{0, -8}, {8, 8}, {0, 0}});
		cursor_proto.tris.push_back(olc::utils::geom2d::triangle<float>{{0, -8}, {0, 0}, {-8, 8}});

		rng.seed(std::random_device{}());
	}

	void tickEnemyTimer() {
		auto entity = reg.create();
		auto& s = reg.emplace<Shape>(entity, square_proto);
		s.color = olc::YELLOW;

		utilities::random::uniform_real_distribution<float> dist {0, olc::utils::geom2d::pi * 2.0f};
		const float angle = dist(rng);

		// Position the enemy to spawn outside the visiable map area
		olc::vf2d position = olc::vf2d{pge->ScreenWidth() / 1.7f, angle}.cart() + pge->GetScreenSize() / 2.0f;

		s.MoveTo(position);
		reg.emplace<EnemyComponent>(entity);
	}

	void spawnBullet(olc::vf2d pos, olc::vf2d vel) {
		auto entity = reg.create();
		auto& s = reg.emplace<Shape>(entity, square_proto);
		s.MoveTo(pos);
		s.scale = 0.3f;
		auto& b = reg.emplace<BulletComponent>(entity, vel);

	}

	void on_enemy_death(const enemy_death& e) {
		score += 1.0f;
	}

	void EnterState() override {
		dispatcher.sink<enemy_death>().connect<&GameplayState::on_enemy_death>(this);
		
		score = 0.0f;
		player = std::make_unique<Shape>(square_proto);
		player->MoveTo(pge->GetScreenSize() / 2.0f);
	}

	GameState OnUserUpdate(float fElapsedTime) override {
		dispatcher.update();
		fTotalTime += fElapsedTime;
		enemyTimer += fElapsedTime * (1 + std::powf(2, fTotalTime / 30.0f));

		while (enemyTimer > enemy_threshold) {
			enemyTimer -= enemy_threshold;
			tickEnemyTimer();
		}

		pge->Clear(olc::VERY_DARK_GREY);
		//DebugDraw();

		//cursor->theta += fElapsedTime;
		//cursor->MoveTo(GetMousePos());

		// Perform player movement
		olc::vf2d input {};
		float speed = 40.0f;
		if(pge->GetKey(olc::Key::W).bHeld) {
			input.y += -player_speed;
		}
		if(pge->GetKey(olc::Key::S).bHeld) {
			input.y += player_speed;
		}
		if(pge->GetKey(olc::Key::A).bHeld) {
			input.x += -player_speed;
		}
		if(pge->GetKey(olc::Key::D).bHeld) {
			input.x += player_speed;
		}

		if(input.x != 0.0f && input.y != 0.0f) {
			input = input.norm() * player_speed;
		}
		
		player->theta += fElapsedTime;
		player->MoveTo(player->position + input * fElapsedTime);

		// Get the path from the player to the cursor and rotate the cursor
		olc::vf2d ray = (pge->GetMousePos() - player->position).norm();
		//cursor->theta = ray.polar().y + (olc::utils::geom2d::pi / 2.0f);


		// Shoot a projectile on Left Click
		if(pge->GetMouse(0).bPressed) {
			spawnBullet(player->position, ray*player_speed);
		}

		// if(cursor->intersects(*player)) {
		// 	cursor->color = olc::RED;
		// } else {
		// 	cursor->color = olc::GREEN;
		// }



		//cursor->Draw(this);
		//test_shape->Draw(this);
		player->Draw(pge);

		{
			// Move bullets around
			const auto view = reg.view<BulletComponent, Shape>();
			for(const auto e : view) {
				auto& s = reg.get<Shape>(e);
				auto& b = reg.get<BulletComponent>(e);
				s.theta += b.angular_velocity * fElapsedTime;
				s.MoveTo(s.position + b.velocity * fElapsedTime);
			}
		}

		{
			// Move Enemies
			const auto view = reg.view<EnemyComponent, Shape>();
			const auto& player_position = player->position;
			for(const auto entity : view) {
				auto [e, s] = view.get(entity);

				olc::vf2d direction = player_position - s.position;
				olc::vf2d velocity = direction.norm() * player_speed;
				s.MoveTo(s.position + velocity * fElapsedTime);
			}
		}

		{
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
							reg.destroy(e_entity);
							enemy_threshold *= 0.9f;
							dispatcher.enqueue<enemy_death>(enemy_death{});
						}

						if (b.hit_count <= 0) {
							reg.destroy(b_entity);
							break;
						}
					}
				}

			}
		}

		{
			// Check if any bullets are off-screen and cull them
			auto view = reg.view<BulletComponent, Shape>();
			//auto max_mag = (pge->GetScreenSize() / 1.7f).mag2();

			for(const auto e : view) {
				const auto& s = reg.get<Shape>(e);
				if ((s.position.x < -10.0f) || (s.position.y < -10.0f) || (s.position.x > pge->ScreenWidth() + 10.0f) || (s.position.y > pge->ScreenHeight() + 10.0f)) {
					reg.destroy(e);
				}
			}
		}

		{
			// Draw Enemies
			const auto view = reg.view<EnemyComponent, Shape>();
			for(const auto e : view) {
				const auto& s = reg.get<Shape>(e);
				s.Draw(pge);
			}
		}
		
		{
			// Draw bullets
			const auto view = reg.view<BulletComponent, Shape>();
			for(const auto e : view) {
				const auto& s = reg.get<Shape>(e);
				s.Draw(pge);
			}
		}

		{
			pge->DrawStringDecal({10.0f, 10.0f}, std::format("Score: {}", score), olc::WHITE, olc::vf2d{3.0f, 3.0f});
		}
		return GameState::Gameplay;
	}
};

class Jam2025Shapes : public olc::PixelGameEngine
{
public:
	//Square test_square;

	//std::vector<std::unique_ptr<Enemy>> enemies;
	// std::vector<std::unique_ptr<Bullet>> bullets;
	std::unique_ptr<Shape> player;
	std::unique_ptr<Shape> cursor;
	std::unique_ptr<Shape> test_shape;

	Prototype square_proto;
	Prototype cursor_proto;

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

		//gameplay.OnEnterState();
		return true;
	}

	// void DebugDraw() {
	// 	DrawString({0,0}, GetMousePos().str());
	// 	DrawString({0,10}, player->position.str());
	// 	DrawString({0,20}, std::to_string(cursor->theta));
	// 	olc::vi2d p {0, 30};
	// 	for(auto& t : *test_shape) {
	// 		p = print(t, p, this);
	// 	}
	// }

	// void tickEnemyTimer() {
	// 	auto entity = reg.create();
	// 	auto& s = reg.emplace<Shape>(entity, square_proto);
	// 	s.color = olc::YELLOW;

	// 	int x = rand() % ScreenWidth();
	// 	int y = rand() % ScreenHeight();

	// 	s.MoveTo({x, y});
	// 	reg.emplace<EnemyComponent>(entity);
	// }

	// void spawnBullet(olc::vf2d pos, olc::vf2d vel) {
	// 	auto entity = reg.create();
	// 	auto& s = reg.emplace<Shape>(entity, square_proto);
	// 	s.MoveTo(pos);
	// 	s.scale = 0.3f;
	// 	auto& b = reg.emplace<BulletComponent>(entity, vel);

	// }


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
	if (demo.Construct(1920, 1080, 1, 1, false, true))
		demo.Start();

	return 0;
}
