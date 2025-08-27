#pragma once

#include "systems/system.hpp"

struct BossFactory {   
    virtual std::unique_ptr<System> GetLeadInSystem(int power) const = 0;
    virtual std::unique_ptr<System> GetBossSystem(int power) const = 0;
    virtual std::unique_ptr<System> GetLeadOutSystem(int power) const = 0;

    virtual ~BossFactory() = default;
};

struct BigChungusLeadInSystem : public System {
    BigChungusLeadInSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : power(power), dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

    void OnUserUpdate(float fElapsedTime) {
        // Remove all enemies, if they exist
        auto view = reg.view<EnemyComponent, Shape>();

        // We will actually tick one more time after spawning.  So only wipe the screen on the first tick.
        if(!one_time) {
            for(auto e : view) {
                const auto s = view.get<Shape>(e);
                std::cout << "Removing " << entt::to_integral(e) << std::endl;

                dispatcher.enqueue(EnemyDeath{s.position, false});
                reg.destroy(e);
            }
            one_time = true;
        }

        total_time += fElapsedTime;

        if((total_time > lead_in_time) && !did_spawn) {
            auto entity = reg.create();
            std::cout << "Made Boss " << entt::to_integral(entity) << std::endl;

            auto& e = reg.emplace<EnemyComponent>(entity);
            auto& p = reg.emplace<PhysicsComponent>(entity);
            auto& s = reg.emplace<Shape>(entity, prototypes[ShapePrototypes::Star9_3]);

            e.health = 10000.0f * (power + 1);
            e.damage = 10.0f;
            p.angular_velocity = .30f;
            p.mass = 10.0f;
            s.scale = 80.0f;
            s.color = olc::GREY;
            //s.position = ;
            s.MoveTo({pge->ScreenWidth() / 2.0f, -0.7f * pge->ScreenHeight()});

            did_spawn = true;

            dispatcher.enqueue(BeginBossMain{entity});
        }

        // Draw a health bar loading across the bottom of the screen
        pge->SetDrawTarget(static_cast<uint8_t>(0)); // select the UI layer

        olc::Pixel color = olc::RED;
        color.a = 255 * utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, total_time)));
        pge->FillRectDecal({10.0f, pge->ScreenHeight() - 30.0f}, {pge->ScreenWidth() - 20.0f, 20.0f}, color);

        float t = std::max((total_time - 1.0f), 0.0f) / (lead_in_time - 1.0f);
        float width = utilities::lerp(0.0f, pge->ScreenWidth() - 20.0f, utilities::Ease(t));
        pge->FillRectDecal({10.0f, pge->ScreenHeight() - 30.0f}, {width, 20.0f}, olc::GREEN);

        // And also some intro text, for fun
        color = olc::WHITE;
        // go from 0 to 1 over the first 1 second
        float t1 = utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, total_time)));
        // go from 1 to 0 over the last 1 second
        float t2 = utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, lead_in_time - total_time)));
        color.a = 255 * (t1 * t2);

        std::string s = "The Chungus Amongus";
        olc::vf2d str_size = pge->GetTextSize(s) * olc::vf2d{4.0f, 4.0f};
        olc::vf2d pos = pge->GetScreenSize() * 0.5f - str_size * 0.5f;
        pge->DrawStringDecal(pos, s, color, {4.0f, 4.0f});


        pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
    }

    
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
    BigChungusBossSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : power(power), dispatcher(dispatcher), player_entity(player), System(reg, pge) {
        dispatcher.sink<BeginBossMain>().connect<&BigChungusBossSystem::on_boss_main>(this);
    };

    ~BigChungusBossSystem() {
        dispatcher.sink<BeginBossMain>().disconnect(this);
    }

    void on_boss_main(const BeginBossMain& boss) {
        //std::cout << "Got Boss " << entt::to_integral(boss.entity) << std::endl;
        boss_entity = boss.entity;
    }

    void OnUserUpdate(float fElapsedTime) {
        float total_health = 10000.0f * (power + 1);
        float health {0.0f};
        if(reg.valid(boss_entity)) {
            const auto& e = reg.get<EnemyComponent>(boss_entity);
            health = e.health;
            pge->DrawStringDecal({10.0f, 170.0f}, std::to_string(health), olc::WHITE, {3.0f, 3.0f});
        }

        // Check if Big Chungus has been killed
        if(!boss_dead && !reg.valid(boss_entity)) {
            boss_dead = true;
            
            // Signal the death of big chungus. :`(
            dispatcher.enqueue(BossKill{});
        }

        // Draw a health bar across the bottom of the screen
        pge->SetDrawTarget(static_cast<uint8_t>(0)); // select the UI layer
        pge->FillRectDecal({10.0f, pge->ScreenHeight() - 30.0f}, {pge->ScreenWidth() - 20.0f, 20.0f}, olc::RED);

        float width = utilities::lerp(0.0f, pge->ScreenWidth() - 20.0f, utilities::Ease(health / total_health));
        pge->FillRectDecal({10.0f, pge->ScreenHeight() - 30.0f}, {width, 20.0f}, olc::GREEN);
        pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
    }

    
private:
    int power;
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    entt::entity boss_entity{};
    bool boss_dead {false};
};

struct BigChungusLeadOutSystem : public System {
    BigChungusLeadOutSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : power(power), dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

    void OnUserUpdate(float fElapsedTime) {
        total_time += fElapsedTime;

        if((total_time > lead_out_time) && !did_trigger) {
            dispatcher.enqueue(BossPhaseDone{});
        }


        // Draw a health bar across the bottom of the screen
        pge->SetDrawTarget(static_cast<uint8_t>(0)); // select the UI layer
        olc::Pixel color = olc::RED;
        color.a = 255 * utilities::lerp(1.0, 0.0, utilities::Ease(total_time / lead_out_time));
        pge->FillRectDecal({10.0f, pge->ScreenHeight() - 30.0f}, {pge->ScreenWidth() - 20.0f, 20.0f}, color);

        // And also some intro text, for fun
        color = olc::YELLOW;
        olc::Pixel color_back = olc::VERY_DARK_YELLOW;
        // go from 0 to 1 over the first 1 second
        float t1 = utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, total_time)));
        // go from 1 to 0 over the last 1 second
        float t2 = utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, lead_out_time - total_time)));
        color.a = 255 * (t1 * t2);
        color_back.a = 255 * (t1 * t2);


        std::string s = "Great Enemy Felled";
        olc::vf2d str_size = pge->GetTextSize(s) * olc::vf2d{4.0f, 4.0f};
        olc::vf2d str_size_back = pge->GetTextSize(s) * olc::vf2d{4.2f, 4.4f};
        olc::vf2d pos = pge->GetScreenSize() * 0.5f - str_size * 0.5f;
        olc::vf2d pos_back = pge->GetScreenSize() * 0.5f - str_size_back * 0.5f;
        pge->DrawStringDecal(pos_back, s, color_back, {4.2f, 4.4f});
        pge->DrawStringDecal(pos, s, color, {4.0f, 4.0f});

        pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
    }

    
private:
    int power;
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    float lead_out_time {10.0f};
    float total_time {0.0f};
    bool did_trigger {false};
};




struct BigChungusFactory : public BossFactory {
    BigChungusFactory(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), reg(reg), pge(pge) {};

    std::unique_ptr<System> GetLeadInSystem(int power) const override {
        return std::make_unique<BigChungusLeadInSystem>(power, dispatcher, player_entity, reg, pge);
    };

    std::unique_ptr<System> GetBossSystem(int power) const override {
        return std::make_unique<BigChungusBossSystem>(power, dispatcher, player_entity, reg, pge);
    };

    std::unique_ptr<System> GetLeadOutSystem(int power) const override {
        return std::make_unique<BigChungusLeadOutSystem>(power, dispatcher, player_entity, reg, pge);
    };

private:
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    entt::registry& reg;
    olc::PixelGameEngine* pge;
};
