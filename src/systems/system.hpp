#pragma once

#include "olcPixelGameEngine.h"
#include "utilities/entt.hpp"

struct System {
	System(entt::registry& reg, olc::PixelGameEngine* pge) : reg(reg), pge(pge) {};

	// Run before any OnUserUpdate functions.  Possibly to reset per-frame data
	virtual void PreUpdate() {};

	// Run once per tick
	virtual void OnUserUpdate(float fElapsedTime) = 0;

	virtual ~System() {};
protected:
	entt::registry& reg;
	olc::PixelGameEngine* pge;
};