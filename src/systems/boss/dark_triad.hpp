#pragma once

#include "boss_factory.hpp"

#include "systems/system.hpp"


struct DarkTriadLeadInSystem : public System {
    DarkTriadLeadInSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

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

struct DarkTriadBossSystem : public System {
    DarkTriadBossSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

    ~DarkTriadBossSystem();

    void on_boss_main(const BeginBossMain& boss);

    void OnUserUpdate(float fElapsedTime) override;

    
private:
    int power;
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    entt::entity boss_entity{};
    bool boss_dead {false};

    float heal_timer {0.0f};
    float heal_cost {5.0f};
};

struct DarkTriadLeadOutSystem : public System {
    DarkTriadLeadOutSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

    void OnUserUpdate(float fElapsedTime) override ;

    
private:
    int power;
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    float lead_out_time {10.0f};
    float total_time {0.0f};
    bool did_trigger {false};
};

struct DarkTriadFactory : public BossFactory {
    DarkTriadFactory(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

    std::unique_ptr<System> GetLeadInSystem(int power) const override;

    std::unique_ptr<System> GetBossSystem(int power) const override;

    std::unique_ptr<System> GetLeadOutSystem(int power) const override;

private:
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    entt::registry& reg;
    olc::PixelGameEngine* pge;
};
