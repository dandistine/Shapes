#pragma once

#include "olcPixelGameEngine.h"

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
