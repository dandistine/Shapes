#pragma once

#include "components.hpp"
#include "system.hpp"

#include "utilities/entt.hpp"

struct DrawSystem : public System {
	DrawSystem(entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : player_entity(player), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		const auto view = reg.view<Shape>();
		view.each([this](entt::entity e, const Shape& s){s.Draw(pge);});

        // Draw the player weapons
        const auto& p = reg.get<PlayerComponent>(player_entity);

        for(const auto& w : p.weapons) {
            w.Draw(pge);
        }
	}
private:
    entt::entity player_entity;
};