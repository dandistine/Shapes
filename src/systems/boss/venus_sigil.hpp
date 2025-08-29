#pragma once

#include "boss_factory.hpp"

#include "systems/system.hpp"


struct VenusSigilLeadInSystem : public System {
    VenusSigilLeadInSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

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

struct LineSegment {
    olc::utils::geom2d::line<float> line;
    olc::Pixel color;  
};

struct Bolt {
    Bolt() {};
	Bolt(olc::vf2d start, olc::vf2d end) { segments.push_back({ {start , end} , olc::WHITE }); };
    void Iterate();

    std::vector<LineSegment> segments;
};

struct VenusSigilBossSystem : public System {
    enum class eMode {
        START, //The beginning of the game
        IDLE, //Nothing is shown on screen
        HINT, //Show the upcoming lightning bolt around a point
        TRIGGER, //Show the full lightning bolt
        SHOW,
        FADEOUT,
        DIE
    };
    
    VenusSigilBossSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

    ~VenusSigilBossSystem();

    void on_boss_main(const BeginBossMain& boss);

    void CheckCollision();

    //void StartFunction(float fElapsedTime);
    void IdleFunction(float fElapsedTime);
    void HintFunction(float fElapsedTime);
    void TriggerFunction(float fElapsedTime);
    void ShowFunction(float fElapsedTime);
    void FadeoutFunction(float fElapsedTime);
    //void DieFunction(float fElapsedTime);

    void OnUserUpdate(float fElapsedTime) override;

    
private:
    int power;
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    entt::entity boss_entity{};
    bool boss_dead {false};
    eMode mode {eMode::IDLE};

    float state_timer {0.0f};
    float idle_threshold {3.0f};
    float hint_threshold {1.0f};
    float trigger_threshold {0.5f};
    float show_threshold {0.1f};
    float fadeout_threshold {0.3f};

    bool did_hit {false};

    Bolt bolt;
};

struct VenusSigilLeadOutSystem : public System {
    VenusSigilLeadOutSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

    void OnUserUpdate(float fElapsedTime) override;

    
private:
    int power;
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    float lead_out_time {10.0f};
    float total_time {0.0f};
    bool did_trigger {false};
};

struct VenusSigilFactory : public BossFactory {
    VenusSigilFactory(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge);

    std::unique_ptr<System> GetLeadInSystem(int power) const override;

    std::unique_ptr<System> GetBossSystem(int power) const override;

    std::unique_ptr<System> GetLeadOutSystem(int power) const override;

private:
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    entt::registry& reg;
    olc::PixelGameEngine* pge;
};
