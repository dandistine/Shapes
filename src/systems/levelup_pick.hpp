#pragma once

#include "components.hpp"
#include "system.hpp"

extern std::array<ShapePrototypes, 7> shape_progression;

// Map of the potential prototypes
extern std::map<ShapePrototypes, Prototype> prototypes;

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
			for(int i = 0; i < choice_count; i++) {
                // Pick a random weapon type
                auto weapon_index = std::uniform_int_distribution<int>{0, weapon_prototypes.size() - 1}(rng);

                std::string description = std::format("Gain a {} weapon", weapon_prototypes[weapon_index]->name);
                auto functor = [&, weapon_index](entt::registry& reg, entt::entity e) {
                    auto& p = reg.get<PlayerComponent>(e);
                    p.weapons.emplace_back(reg, dispatcher, *(weapon_prototypes[weapon_index]));
                };
				options.emplace_back(LevelUpOption{description, functor});
			}
			return;
		}

        // Core upgrades will now come from defeating bosses
		// if((ps.WeaponPointCount() - 2) < shape_progression.size()) {
		// 	std::string description = "Upgrade Core";
		// 	auto functor = [](entt::registry& reg, entt::entity e) {
		// 		auto& p = reg.get<PlayerComponent>(e);
		// 		auto& s = reg.get<Shape>(e);
		// 		auto& next_shape = prototypes[shape_progression[s.WeaponPoints().size() - 2]];
		// 		s.SetPrototype(next_shape);
		// 		//reg.erase<Shape>(e);
		// 		//reg.emplace<Shape>(e, s, next_shape);
		// 		//p.max_weapon_count += 1;
		// 	};

		// 	options.emplace_back(LevelUpOption{description, functor});
		// }

		// Fill the rest of the options with "improve weapon"
		for(int i = options.size(); i < choice_count; i++) {
			int weapon_slot = rand() % p.weapons.size();
			const auto& weapon = p.weapons[weapon_slot];
			std::string description = std::format("Improve {} weapon from level {} to {}", weapon.Name(), weapon.Level(), weapon.Level() + 1);
			//std::string description = "Improve weapon " + std::to_string(weapon_slot + 1);

			auto functor = [weapon_slot](entt::registry& reg, entt::entity e) {
				auto& p = reg.get<PlayerComponent>(e);
				p.weapons[weapon_slot].LevelUp(1);
			};
			options.emplace_back(LevelUpOption{description, functor});
		}
	}

	void OnUserUpdate(float fElapsedTime) override {
		olc::vf2d pos {100.0f, 300.0f};
        pge->SetDrawTarget(static_cast<uint8_t>(0));

		for(auto & o : options) {
			pge->DrawStringDecal(pos, o.description, olc::WHITE, {3.0f, 3.0f});
			pos.y += 30.0f;
		}
        pge->SetDrawTarget(static_cast<uint8_t>(1));


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
	std::mt19937_64 rng{std::random_device{}()};

};
