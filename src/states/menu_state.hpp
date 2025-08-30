#pragma once

#include "shape.hpp"
#include "state.hpp"

#include "utilities/entt.hpp"

#include <random>
#include <vector>

struct MenuState : public State {
	MenuState(olc::PixelGameEngine* pge);
    void EnterState() override;
	GameState OnUserUpdate(float fElapsedTime) override;

private:
    float fTotalTime {0.0f};
    float spawn_time {0.0f};
    float spawn_rate {1.0f};
    std::vector<Shape> shapes;
    entt::registry reg;
	//std::mt19937_64 rng{std::random_device{}()};

};
