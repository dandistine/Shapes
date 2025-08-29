#pragma once

#include "boss_factory.hpp"

#include "systems/system.hpp"


struct BigChungusLeadInSystem : public System {
    BigChungusLeadInSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

    void OnUserUpdate(float fElapsedTime) override;

    
private:
    int power;
    entt::dispatcher& dispatcher;
    entt::entity player_entity;

    float lead_in_time {10.0f};
    float total_time {0.0f};
    float did_spawn {false};
    float one_time {false};
};

struct BigChungusBossSystem : public System {
    BigChungusBossSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

    ~BigChungusBossSystem();

    void on_boss_main(const BeginBossMain& boss);

    void OnUserUpdate(float fElapsedTime) override;

    
private:
    int power;
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    entt::entity boss_entity{};
    bool boss_dead {false};
};

struct BigChungusLeadOutSystem : public System {
    BigChungusLeadOutSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

    void OnUserUpdate(float fElapsedTime) override ;

    
private:
    int power;
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    float lead_out_time {10.0f};
    float total_time {0.0f};
    bool did_trigger {false};
};

struct BigChungusFactory : public BossFactory {
    BigChungusFactory(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

    std::unique_ptr<System> GetLeadInSystem(int power) const override;

    std::unique_ptr<System> GetBossSystem(int power) const override;

    std::unique_ptr<System> GetLeadOutSystem(int power) const override;

private:
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    entt::registry& reg;
    olc::PixelGameEngine* pge;
};
