#pragma once

#include "components.hpp"
#include "system.hpp"

#include "utilities/entt.hpp"

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