#pragma once

#include "components.hpp"
#include "events.hpp"

struct BossTimerSystem : public System {
	BossTimerSystem(entt::dispatcher& dispatcher, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), System(reg, pge) {};

	void OnUserUpdate(float fElapsedTime) override {
        total_time += fElapsedTime;

        if(total_time >= boss_time) {
            total_time = 0.0f;

            dispatcher.enqueue(SpawnBoss{"Boss 1", boss_count});
            boss_count++;
        }
	}
private:
    entt::dispatcher& dispatcher;

    float total_time {0.0f};
    float boss_time {60.0f};
    
    // How many bosses have been summoned
    int boss_count {0};
};