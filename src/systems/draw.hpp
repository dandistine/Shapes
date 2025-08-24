#pragma once

#include "components.hpp"
#include "system.hpp"

#include "utilities/entt.hpp"

struct DrawSystem : public System {
	DrawSystem(entt::registry& reg, olc::PixelGameEngine* pge) : System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
		const auto view = reg.view<Shape>();
		view.each([this](entt::entity e, const Shape& s){s.Draw(pge);});
	}
};