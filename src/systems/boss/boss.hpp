#pragma once

#include "systems/system.hpp"

struct BossFactory {   
    virtual std::unique_ptr<System> GetLeadInSystem() const = 0;
    virtual std::unique_ptr<System> GetBossSystem() const = 0;
    virtual std::unique_ptr<System> GetLeadOutSystem() const = 0;

    virtual ~BossFactory() = default;
};

struct BigChungusLeadInSystem : public System {
    BigChungusLeadInSystem(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

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

            e.health = 50000.0f;
            e.damage = 10.0f;
            p.angular_velocity = .30f;
            p.mass = 10.0f;
            s.scale = 80.0f;
            s.color = olc::GREY;
            //s.position = ;
            s.MoveTo({pge->ScreenWidth() / 2.0f, -1.0f * pge->ScreenHeight()});

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
        pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
    }

    
private:
    entt::dispatcher& dispatcher;
    entt::entity player_entity;

    float lead_in_time {10.0f};
    float total_time {0.0f};
    float did_spawn {false};
    float one_time {false};
};

struct BigChungusBossSystem : public System {
    BigChungusBossSystem(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), System(reg, pge) {
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

        float width = utilities::lerp(0.0f, pge->ScreenWidth() - 20.0f, utilities::Ease(health / 50000.0f));
        pge->FillRectDecal({10.0f, pge->ScreenHeight() - 30.0f}, {width, 20.0f}, olc::GREEN);
        pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
    }

    
private:
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    entt::entity boss_entity{};
    bool boss_dead {false};
};

struct BigChungusLeadOutSystem : public System {
    BigChungusLeadOutSystem(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

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

        pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
    }

    
private:
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    float lead_out_time {10.0f};
    float total_time {0.0f};
    bool did_trigger {false};
};




struct BigChungusFactory : public BossFactory {
    BigChungusFactory(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), reg(reg), pge(pge) {};

    std::unique_ptr<System> GetLeadInSystem() const override {
        return std::make_unique<BigChungusLeadInSystem>(dispatcher, player_entity, reg, pge);
    };

    std::unique_ptr<System> GetBossSystem() const override {
        return std::make_unique<BigChungusBossSystem>(dispatcher, player_entity, reg, pge);
    };

    std::unique_ptr<System> GetLeadOutSystem() const override {
        return std::make_unique<BigChungusLeadOutSystem>(dispatcher, player_entity, reg, pge);
    };

private:
    entt::dispatcher& dispatcher;
    entt::entity player_entity;
    entt::registry& reg;
    olc::PixelGameEngine* pge;
};
