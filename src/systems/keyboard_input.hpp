#pragma once

#include "components.hpp"
#include "system.hpp"

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
