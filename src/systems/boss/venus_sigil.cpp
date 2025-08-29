
#include "components.hpp"

#include "venus_sigil.hpp"

#include "boss.hpp"
#include "events.hpp"

#include "systems/system.hpp"

#include "utilities/utility.hpp"


float rand_float() {
	return ((float)rand()) / RAND_MAX;
}

template<typename T>
olc::v_2d<T> MidPoint(olc::v_2d<T> start, olc::v_2d<T> end) {
    return (start + end) / 2;
}

const float split_chance = 0.3f;
const float split_alpha_mod = 0.5f;


VenusSigilLeadInSystem::VenusSigilLeadInSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : power(power), dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

void VenusSigilLeadInSystem::OnUserUpdate(float fElapsedTime) {
    // Remove all enemies, if they exist
    auto view = reg.view<EnemyComponent, Shape>();

    // We will actually tick one more time after spawning.  So only wipe the screen on the first tick.
    if(!one_time) {
        for(auto e : view) {
            const auto s = view.get<Shape>(e);
            //std::cout << "Removing " << entt::to_integral(e) << std::endl;

            dispatcher.enqueue(EnemyDeath{s.position, false});
            reg.destroy(e);
        }
        one_time = true;
    }

    total_time += fElapsedTime;

    olc::Pixel background_color = utilities::lerp(olc::VERY_DARK_GREY, olc::BLACK, utilities::Ease(total_time / lead_in_time));
    pge->Clear(background_color);

    if((total_time > lead_in_time) && !did_spawn) {
        auto entity = reg.create();
        //std::cout << "Made Boss " << entt::to_integral(entity) << std::endl;

        auto& e = reg.emplace<EnemyComponent>(entity);
        auto& p = reg.emplace<PhysicsComponent>(entity);
        auto& s = reg.emplace<Shape>(entity, prototypes[ShapePrototypes::Star7_3]);

        e.health = 5000.0f + 2500 * power;
        e.damage = 10.0f;
        p.angular_velocity = .30f;
        p.mass = 3.0f;
        s.scale = 20.0f;
        s.color = olc::BLUE;
        //s.position = ;
        s.MoveTo({pge->ScreenWidth() / 2.0f, -0.4f * pge->ScreenHeight()});

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

    std::string s = "Fade To Black";
    olc::vf2d str_size = pge->GetTextSize(s) * olc::vf2d{4.0f, 4.0f};
    olc::vf2d pos = pge->GetScreenSize() * 0.5f - str_size * 0.5f;
    pge->DrawStringDecal(pos, s, color, {4.0f, 4.0f});


    pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
}




void Bolt::Iterate() {
    std::vector<LineSegment> new_segments;

    for (const auto& s : segments) {
        olc::vf2d m = MidPoint(s.line.start, s.line.end);
        olc::vf2d sl = m - s.line.start;

        //sl will be perpensidcular to the segment (s, m)
        sl = { -sl.y, sl.x };

        //move m perpendicularly a little bit
        m = m + (rand_float() - 0.5f) * sl;

        //Randomize the color of new segments a little bit
        //Keeping the blue at full gives a nice appearance
        float r = 0.7f + rand_float() / 3.34f;
        float g = 0.8f + rand_float() / 5.34f;

        olc::Pixel c = olc::PixelF(r, g, 1.0f, s.color.a / 255.0f);

        new_segments.emplace_back(LineSegment{ {s.line.start, m}, c });
        new_segments.emplace_back(LineSegment{ {m, s.line.end}, c });

        //If we're going to split, make the split a reflection
        //over the (s, m) line and give it a little bit of alpha
        if (rand_float() < split_chance) {
            olc::vf2d x = m + (m - s.line.start);
            olc::vf2d ne = x + (x - s.line.end);
            c.a *= split_alpha_mod;
            new_segments.emplace_back(LineSegment{ {m, ne}, c});
        }
    }

    segments = new_segments;
}

VenusSigilBossSystem::VenusSigilBossSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : power(power), dispatcher(dispatcher), player_entity(player), System(reg, pge) {
    dispatcher.sink<BeginBossMain>().connect<&VenusSigilBossSystem::on_boss_main>(this);
    idle_threshold = std::max(1.0f, 3.0f - (power * 0.5f));
};

VenusSigilBossSystem::~VenusSigilBossSystem() {
    dispatcher.sink<BeginBossMain>().disconnect(this);
}


void VenusSigilBossSystem::IdleFunction(float fElapsedTime) {
    did_hit = false;
    if(state_timer > idle_threshold) {
        state_timer -= idle_threshold;

        mode = eMode::HINT;

        float x1 = rand_float() * pge->ScreenWidth();
        float x2 = rand_float() * pge->ScreenWidth();
        float m_x = rand_float() * 30.0f - 15.0f;
        float m_y = rand_float() * 30.0f - 15.0f;
        
        const auto& s = reg.get<Shape>(player_entity);
        olc::vf2d mp = { s.position.x + m_x, s.position.y + m_y };

        //bolt = Bolt({ x1, 0.0f }, { x2, (float)ScreenHeight() });
        bolt = Bolt({ x1, 0.0f }, { mp });
        bolt.segments.push_back({ {mp, { x2, (float)pge->ScreenHeight() }} , olc::WHITE });

        float r = rand_float();
        float limit = r * 6 + 4;

        //delay.delay = 0.1 + r * (0.9);
        //mixer.amplitude[2] = rand_float();
        //gain.gain = 1;
        //ls.SetLCount(1 + floor(r * 6));

        

        for (float i = 0; i < limit; i += 1) {
            bolt.Iterate();
        }
    }
}

void VenusSigilBossSystem::HintFunction(float fElapsedTime) {
    if (state_timer > hint_threshold) {
        state_timer -= hint_threshold;
        mode = eMode::TRIGGER;
        //adsr.Begin();
        //adsr2.Begin();
        //ls.Trigger();
    }

    const auto& p = reg.get<Shape>(player_entity).position;
    auto threshold = (25000 * state_timer / hint_threshold);

    for (const auto& s : bolt.segments) {
        if ((p - s.line.start).mag2() < threshold) {
            olc::Pixel c = { s.color.r, s.color.g, s.color.b, (uint8_t)(s.color.a * 0.25f) };
            pge->DrawLine(s.line.start, s.line.end, c);
        }
    }    
}

void VenusSigilBossSystem::TriggerFunction(float fElapsedTime) {
    CheckCollision();
    float threshold = pge->ScreenHeight() * (state_timer / trigger_threshold);
    if (state_timer > trigger_threshold) {
        state_timer -= trigger_threshold;
        mode = eMode::SHOW;
    }


    for (const auto& s : bolt.segments) {
        if (s.line.start.y < threshold) {
            pge->DrawLine(s.line.start, s.line.end, s.color);
        }
    }
}

//Show state function.  The bolt is fully visible.
void VenusSigilBossSystem::ShowFunction(float fElapsedTime) {
    CheckCollision();
    if (state_timer > show_threshold) {
        state_timer -= show_threshold;
        mode = eMode::FADEOUT;
        

    }
    for (const auto& s : bolt.segments) {
        pge->DrawLine(s.line.start, s.line.end, s.color);
    }
}

//Fadeout state function.  Draw segments with increasing alpha so it
//looks like the bolt is fading away.  Any forks or off-shoots will
//appear to fade before the main bolt.
void VenusSigilBossSystem::FadeoutFunction(float fElapsedTime) {
    CheckCollision();
    float a = std::max(0.0f, 1.0f - state_timer / fadeout_threshold);

    for (const auto& s : bolt.segments) {
        olc::Pixel c = { s.color.r, s.color.g, s.color.b, (uint8_t)(s.color.a * a)};
        pge->DrawLine(s.line.start, s.line.end, c);
    }

    if (state_timer > fadeout_threshold) {
        state_timer -= fadeout_threshold;
        //bolts_dodged += 1;
        mode = eMode::IDLE;
    }

}

void VenusSigilBossSystem::on_boss_main(const BeginBossMain& boss){
    boss_entity = boss.entity;
}

void VenusSigilBossSystem::CheckCollision() {
    auto& p_shape = reg.get<Shape>(player_entity);
    if(did_hit) {
        return;
    }

    for(const auto& s : bolt.segments) {
        
        for(const auto& t : p_shape) {
            if(olc::utils::geom2d::overlaps(t, s.line)) {
                did_hit = true;
                break;
            }
        }
        if(did_hit) {
            break;
        }
    }
    if(did_hit) {
        auto& p = reg.get<PlayerComponent>(player_entity);
        p.health -= 3.0f;
    }
}

void VenusSigilBossSystem::OnUserUpdate(float fElapsedTime) {
    pge->Clear(olc::BLACK);
    state_timer += fElapsedTime;


    float total_health = 5000.0f + 2500 * power;
    float health {0.0f};
    if(reg.valid(boss_entity)) {
        const auto& e = reg.get<EnemyComponent>(boss_entity);
        health = e.health;
        pge->DrawStringDecal({10.0f, 170.0f}, std::to_string(health), olc::WHITE, {3.0f, 3.0f});
        pge->DrawStringDecal({10.0f, 200.0f}, std::to_string(static_cast<int>(mode)), olc::WHITE, {3.0f, 3.0f});
    }

    switch (mode) {
        // case eMode::START:
        //     StartFunction(fElapsedTime);
        //     break;
        case eMode::IDLE:
            IdleFunction(fElapsedTime);
            break;
        case eMode::HINT:
            HintFunction(fElapsedTime);
            break;
        case eMode::TRIGGER:
            TriggerFunction(fElapsedTime);
            break;
        case eMode::SHOW:
            ShowFunction(fElapsedTime);
            break;
        case eMode::FADEOUT:
            FadeoutFunction(fElapsedTime);
            break;
        // case eMode::DIE:
        //     DieFunction(fElapsedTime);
        //     break;
    }

    // Check if the boss has been killed
    if(!boss_dead && !reg.valid(boss_entity)) {
        boss_dead = true;
        
        // Signal the death of the boss
        dispatcher.enqueue(BossKill{});
    }

    // Draw a health bar across the bottom of the screen
    pge->SetDrawTarget(static_cast<uint8_t>(0)); // select the UI layer
    pge->FillRectDecal({10.0f, pge->ScreenHeight() - 30.0f}, {pge->ScreenWidth() - 20.0f, 20.0f}, olc::RED);

    float width = utilities::lerp(0.0f, pge->ScreenWidth() - 20.0f, utilities::Ease(health / total_health));
    pge->FillRectDecal({10.0f, pge->ScreenHeight() - 30.0f}, {width, 20.0f}, olc::GREEN);
    pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
}

VenusSigilLeadOutSystem::VenusSigilLeadOutSystem(int power, entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : power(power), dispatcher(dispatcher), player_entity(player), System(reg, pge) {};

void VenusSigilLeadOutSystem::OnUserUpdate(float fElapsedTime) {
    total_time += fElapsedTime;

    if((total_time > lead_out_time) && !did_trigger) {
        dispatcher.enqueue(BossPhaseDone{});
    }

    olc::Pixel background_color = utilities::lerp(olc::BLACK, olc::VERY_DARK_GREY, utilities::Ease(total_time / lead_out_time));
    pge->Clear(background_color);


    // Draw a health bar across the bottom of the screen
    pge->SetDrawTarget(static_cast<uint8_t>(0)); // select the UI layer
    olc::Pixel color = olc::RED;
    color.a = 255 * utilities::lerp(1.0, 0.0, utilities::Ease(total_time / lead_out_time));
    pge->FillRectDecal({10.0f, pge->ScreenHeight() - 30.0f}, {pge->ScreenWidth() - 20.0f, 20.0f}, color);

    // And also some outro text, for fun
    color = olc::BLUE;
    olc::Pixel color_back = olc::VERY_DARK_BLUE;
    // go from 0 to 1 over the first 1 second
    float t1 = utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, total_time)));
    // go from 1 to 0 over the last 1 second
    float t2 = utilities::lerp(0.0f, 1.0f, utilities::Ease(std::min(1.0f, lead_out_time - total_time)));
    color.a = 255 * (t1 * t2);
    color_back.a = 255 * (t1 * t2);


    std::string s = "Obtained Venus Sigil!";
    olc::vf2d str_size = pge->GetTextSize(s) * olc::vf2d{4.0f, 4.0f};
    olc::vf2d str_size_back = pge->GetTextSize(s) * olc::vf2d{4.2f, 4.4f};
    olc::vf2d pos = pge->GetScreenSize() * 0.5f - str_size * 0.5f;
    olc::vf2d pos_back = pge->GetScreenSize() * 0.5f - str_size_back * 0.5f;
    pge->DrawStringDecal(pos_back, s, color_back, {4.2f, 4.4f});
    pge->DrawStringDecal(pos, s, color, {4.0f, 4.0f});

    pge->SetDrawTarget(static_cast<uint8_t>(1)); // select the Game layer again
}


VenusSigilFactory::VenusSigilFactory(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), reg(reg), pge(pge) {};

std::unique_ptr<System> VenusSigilFactory::GetLeadInSystem(int power) const {
    return std::make_unique<VenusSigilLeadInSystem>(power, dispatcher, player_entity, reg, pge);
};

std::unique_ptr<System> VenusSigilFactory::GetBossSystem(int power) const {
    return std::make_unique<VenusSigilBossSystem>(power, dispatcher, player_entity, reg, pge);
};

std::unique_ptr<System> VenusSigilFactory::GetLeadOutSystem(int power) const {
    return std::make_unique<VenusSigilLeadOutSystem>(power, dispatcher, player_entity, reg, pge);
};