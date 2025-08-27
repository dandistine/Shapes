#pragma once

#include "components.hpp"
#include "system.hpp"

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
	}
private:
	entt::entity player_entity;
};