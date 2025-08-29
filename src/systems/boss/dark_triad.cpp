#include "components.hpp"

#include "dark_triad.hpp"

#include "boss.hpp"
#include "events.hpp"

#include "systems/system.hpp"

#include "utilities/utility.hpp"

DarkTriadLeadInSystem::DarkTriadLeadInSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : power(power), dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

void DarkTriadLeadInSystem::OnUserUpdate(float fElapsedTime) {
    // Remove all enemies, if they exist
    auto view = reg.view<EnemyComponent, Shape>();

    // We will actually tick one more time after spawning.  So only wipe the screen on the first tick.
    if(!one_time) {
        for(auto e : view) {
            const auto s = view.get<Shape>(e);

            dispatcher.enqueue(EnemyDeath{s.position, false});
            reg.destroy(e);
        }
        one_time = true;
    }

    total_time += fElapsedTime;

    if((total_time > lead_in_time) && !did_spawn) {
        // Spawn 3 enemies, but we also need to hold onto their entities
        auto entity1 = reg.create();
        auto entity2 = reg.create();
        auto entity3 = reg.create();
        // Create a special entity to track the 3 bosses
        auto entity4 = reg.create();

        {
            auto& e = reg.emplace<EnemyComponent>(entity1);
            auto& p = reg.emplace<PhysicsComponent>(entity1);
            auto& s = reg.emplace<Shape>(entity1, prototypes[ShapePrototypes::Triangle]);
            reg.emplace<DarkTriadMember>(entity1);

            e.health = 1000.0f * (power + 1);
            e.damage = 10.0f;
            p.angular_velocity = -1.30f;
            p.mass = 3.0f;
            s.scale = 10.0f;
            s.color = utilities::RandomDarkColor();
            s.MoveTo({pge->ScreenWidth() / 2.0f, -0.4f * pge->ScreenHeight()});
        }

                {
            auto& e = reg.emplace<EnemyComponent>(entity2);
            auto& p = reg.emplace<PhysicsComponent>(entity2);
            auto& s = reg.emplace<Shape>(entity2, prototypes[ShapePrototypes::Triangle]);
            reg.emplace<DarkTriadMember>(entity2);

            e.health = 1000.0f * (power + 1);
            e.damage = 10.0f;
            p.angular_velocity = -1.30f;
            p.mass = 3.0f;
            s.scale = 10.0f;
            s.color = utilities::RandomDarkColor();
            s.MoveTo({static_cast<float>(pge->ScreenWidth()) * -0.2f, 0.5f * static_cast<float>(pge->ScreenHeight())});
        }

                {
            auto& e = reg.emplace<EnemyComponent>(entity3);
            auto& p = reg.emplace<PhysicsComponent>(entity3);
            auto& s = reg.emplace<Shape>(entity3, prototypes[ShapePrototypes::Triangle]);
            reg.emplace<DarkTriadMember>(entity3);

            e.health = 1000.0f * (power + 1);
            e.damage = 10.0f;
            p.angular_velocity = -1.30f;
            p.mass = 3.0f;
            s.scale = 10.0f;
            s.color = utilities::RandomDarkColor();
            s.MoveTo({static_cast<float>(pge->ScreenWidth()) * 1.2f, 0.5f * static_cast<float>(pge->ScreenHeight())});
        }

        did_spawn = true;

        reg.emplace<DarkTriadComponent>(entity4, entity1, entity2, entity3);

        dispatcher.enqueue(BeginBossMain{entity4});
    }

    // Create 3 health bars across the bottom of the screen
    pge->SetDrawTarget(static_cast<uint8_t>(0)); // select the UI layer

    olc::vf2d size = {(pge->ScreenWidth() - 40.0f) / 3.0f, 20.0f};
    olc::vf2d pos1 = {10.0f, pge->ScreenHeight() - 30.0f};
    olc::vf2d pos2 = {20.0f + size.x, pge->ScreenHeight() - 30.0f};
    olc::vf2d pos3 = {30.0f + 2.0f * size.x, pge->ScreenHeight() - 30.0f};

    olc::Pixel color = olc::RED;
    color.a = 255 * utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, total_time)));
    pge->FillRectDecal(pos1, size, color);
    pge->FillRectDecal(pos2, size, color);
    pge->FillRectDecal(pos3, size, color);

    float t = std::max((total_time - 1.0f), 0.0f) / (lead_in_time - 1.0f);
    float width = utilities::lerp(0.0f, size.x, utilities::Ease(t));
    
    pge->FillRectDecal(pos1, {width, 20.0f}, olc::GREEN);
    pge->FillRectDecal(pos2, {width, 20.0f}, olc::GREEN);
    pge->FillRectDecal(pos3, {width, 20.0f}, olc::GREEN);

    // And also some intro text, for fun
    color = olc::WHITE;
    // go from 0 to 1 over the first 1 second
    float t1 = utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, total_time)));
    // go from 1 to 0 over the last 1 second
    float t2 = utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, lead_in_time - total_time)));
    color.a = 255 * (t1 * t2);

    std::string s = "Dr. Paulhus will see you now";
    olc::vf2d str_size = pge->GetTextSize(s) * olc::vf2d{4.0f, 4.0f};
    olc::vf2d pos = pge->GetScreenSize() * 0.5f - str_size * 0.5f;
    pge->DrawStringDecal(pos, s, color, {4.0f, 4.0f});


    pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
}



DarkTriadBossSystem::DarkTriadBossSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : power(power), dispatcher(dispatcher), player_entity(player), System(reg, pge) {
    dispatcher.sink<BeginBossMain>().connect<&DarkTriadBossSystem::on_boss_main>(this);
    heal_cost = std::max(3.0f, 5.0f * std::powf(0.9, power));
};

DarkTriadBossSystem::~DarkTriadBossSystem() {
    dispatcher.sink<BeginBossMain>().disconnect(this);
}

void DarkTriadBossSystem::on_boss_main(const BeginBossMain& boss) {
    //std::cout << "Got Boss " << entt::to_integral(boss.entity) << std::endl;
    boss_entity = boss.entity;
}

void DarkTriadBossSystem::OnUserUpdate(float fElapsedTime) {
    if(boss_dead) {
        return;
    }
    heal_timer += fElapsedTime;
    float total_health = 1000.0f * (power + 1);
    float boss_1_health {0.0f};
    float boss_2_health {0.0f};
    float boss_3_health {0.0f};

    // This boss is actually 3 bosses
    if(reg.valid(boss_entity)) {
        const auto& e = reg.get<DarkTriadComponent>(boss_entity);
        boss_1_health = reg.get<EnemyComponent>(e.boss1).health;
        boss_2_health = reg.get<EnemyComponent>(e.boss2).health;
        boss_3_health = reg.get<EnemyComponent>(e.boss3).health;
    }

    //  Apply a small force between boss that makes them spread out a bit
    {
        auto view1 = reg.view<DarkTriadMember, PhysicsComponent, Shape>();
        auto view2 = reg.view<DarkTriadMember, Shape>();

        for(auto e1 : view1) {
            auto& p = view1.get<PhysicsComponent>(e1);
            auto& s = view1.get<Shape>(e1);
            for(auto e2 : view2) {
                if(e1 == e2) {
                    continue;
                }
                auto s2 = view2.get<Shape>(e2);
                auto dist = s2.position - s.position;
                float scalar = utilities::lerp(1.0f, 0.0f, std::min(1.0f, dist.mag2() / 62000));
                p.force += dist.norm() * -4500.0f * scalar;
            }
        }
    }
    
    // Every N seconds, level out the boss health
    if(heal_timer >= heal_cost) {
        heal_timer -= heal_cost;

        float new_health = std::max(boss_1_health, std::max(boss_2_health, boss_3_health));
        const auto& e = reg.get<DarkTriadComponent>(boss_entity);
        reg.get<EnemyComponent>(e.boss1).health = new_health;
        reg.get<EnemyComponent>(e.boss2).health = new_health;
        reg.get<EnemyComponent>(e.boss3).health = new_health;
    }

    // If a boss is at 0 health, don't move it
    {
        auto view1 = reg.view<DarkTriadMember, PhysicsComponent, EnemyComponent>();
        for(auto entity : view1) {
            auto& p = view1.get<PhysicsComponent>(entity);
            const auto& e = view1.get<EnemyComponent>(entity);

            if(e.health <= 0.0f) {
                p.force = {0.0f, 0.0f};
            }
        }
    }

    {
        // If all bosses are dead
        if((boss_1_health <= 0.0f) && (boss_2_health <= 0.0f) && (boss_3_health <= 0.0f)) {
            const auto& e = reg.get<DarkTriadComponent>(boss_entity);
            reg.destroy(e.boss1);
            reg.destroy(e.boss2);
            reg.destroy(e.boss3);
            reg.destroy(boss_entity);
        }
    }


    // Check if All bosses have been killed
    if(!boss_dead && !reg.valid(boss_entity)) {
        boss_dead = true;
        
        // Signal the death of big chungus. :`(
        dispatcher.enqueue(BossKill{});
    }

    // Draw a health bar across the bottom of the screen
    pge->SetDrawTarget(static_cast<uint8_t>(0)); // select the UI layer

    olc::vf2d size = {(pge->ScreenWidth() - 40.0f) / 3.0f, 20.0f};
    olc::vf2d pos1 = {10.0f, pge->ScreenHeight() - 30.0f};
    olc::vf2d pos2 = {20.0f + size.x, pge->ScreenHeight() - 30.0f};
    olc::vf2d pos3 = {30.0f + 2.0f * size.x, pge->ScreenHeight() - 30.0f};

    olc::Pixel color = olc::RED;
    pge->FillRectDecal(pos1, size, color);
    pge->FillRectDecal(pos2, size, color);
    pge->FillRectDecal(pos3, size, color);

    //float t = std::max((total_time - 1.0f), 0.0f) / (lead_in_time - 1.0f);
    //float width = utilities::lerp(0.0f, size.x, utilities::Ease(t));
    
    float width1 = utilities::lerp(0.0f, size.x, utilities::Ease(boss_1_health / total_health));
    float width2 = utilities::lerp(0.0f, size.x, utilities::Ease(boss_2_health / total_health));
    float width3 = utilities::lerp(0.0f, size.x, utilities::Ease(boss_3_health / total_health));

    pge->FillRectDecal(pos1, {width1, 20.0f}, olc::GREEN);
    pge->FillRectDecal(pos2, {width2, 20.0f}, olc::GREEN);
    pge->FillRectDecal(pos3, {width3, 20.0f}, olc::GREEN);

    {
        // Draw the boss name
        std::string boss_name = "The Dark Triad";
        olc::vf2d name_size = pge->GetTextSize(boss_name) * olc::vf2d{4.0f, 4.0f};
        olc::vf2d name_pos = olc::vf2d{static_cast<float>(pge->ScreenWidth()) * 0.5f, static_cast<float>(pge->ScreenHeight()) - 60} - name_size * 0.5f;
        pge->DrawStringDecal(name_pos, boss_name, olc::WHITE, {4.0f, 4.0f});

        //pge->DrawStringDecal()
    }

    pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
}


DarkTriadLeadOutSystem::DarkTriadLeadOutSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : power(power), dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

void DarkTriadLeadOutSystem::OnUserUpdate(float fElapsedTime) {
    total_time += fElapsedTime;

    if((total_time > lead_out_time) && !did_trigger) {
        dispatcher.enqueue(BossPhaseDone{});
    }


    // Draw a health bar across the bottom of the screen
    pge->SetDrawTarget(static_cast<uint8_t>(0)); // select the UI layer
    olc::Pixel color = olc::RED;
    color.a = 255 * utilities::lerp(1.0, 0.0, utilities::Ease(total_time / lead_out_time));
    olc::vf2d size = {(pge->ScreenWidth() - 40.0f) / 3.0f, 20.0f};
    olc::vf2d pos1 = {10.0f, pge->ScreenHeight() - 30.0f};
    olc::vf2d pos2 = {20.0f + size.x, pge->ScreenHeight() - 30.0f};
    olc::vf2d pos3 = {30.0f + 2.0f * size.x, pge->ScreenHeight() - 30.0f};

    //color = olc::RED;
    pge->FillRectDecal(pos1, size, color);
    pge->FillRectDecal(pos2, size, color);
    pge->FillRectDecal(pos3, size, color);

    // And also some outro text, for fun
    color = olc::WHITE;
    olc::Pixel color_back = olc::VERY_DARK_CYAN;
    // go from 0 to 1 over the first 1 second
    float t1 = utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, total_time)));
    // go from 1 to 0 over the last 1 second
    float t2 = utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, lead_out_time - total_time)));
    color.a = 255 * (t1 * t2);
    color_back.a = 255 * (t1 * t2);


    std::string s = "Balance Restored";
    olc::vf2d str_size = pge->GetTextSize(s) * olc::vf2d{4.0f, 4.0f};
    olc::vf2d str_size_back = pge->GetTextSize(s) * olc::vf2d{4.2f, 4.4f};
    olc::vf2d pos = pge->GetScreenSize() * 0.5f - str_size * 0.5f;
    olc::vf2d pos_back = pge->GetScreenSize() * 0.5f - str_size_back * 0.5f;
    pge->DrawStringDecal(pos_back, s, color_back, {4.2f, 4.4f});
    pge->DrawStringDecal(pos, s, color, {4.0f, 4.0f});

    pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
}


DarkTriadFactory::DarkTriadFactory(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), reg(reg), pge(pge) {};

std::unique_ptr<System> DarkTriadFactory::GetLeadInSystem(int power) const {
    return std::make_unique<DarkTriadLeadInSystem>(power, dispatcher, player_entity, reg, pge);
};

std::unique_ptr<System> DarkTriadFactory::GetBossSystem(int power) const {
    return std::make_unique<DarkTriadBossSystem>(power, dispatcher, player_entity, reg, pge);
};

std::unique_ptr<System> DarkTriadFactory::GetLeadOutSystem(int power) const {
    return std::make_unique<DarkTriadLeadOutSystem>(power, dispatcher, player_entity, reg, pge);
};