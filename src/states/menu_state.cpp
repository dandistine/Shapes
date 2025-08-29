#include "shape.hpp"
#include "menu_state.hpp"

#include "utilities/random.hpp"
#include "utilities/utility.hpp"


// Create some random shapes that float by in the background
Shape RandomShape() {
    ShapePrototypes type = static_cast<ShapePrototypes>(rand() % prototypes.size());

    olc::Pixel color = utilities::RandomColor();
    color.a = 150 + (rand() % 32);
    float scale = 10 + (rand() % 30);

    Shape shape {prototypes[type]};
    shape.color = color;
    shape.scale = scale;

    return shape;
}

MenuState::MenuState(olc::PixelGameEngine* pge) : State(pge) {};

struct LerpComponent  {
    olc::vf2d start;
    olc::vf2d end;
    float total_time;
    float current_time{0.0f};
};

void MenuState::EnterState() {
    auto entity = reg.create();


}

GameState MenuState::OnUserUpdate(float fElapsedTime) {
    fTotalTime += fElapsedTime;
    spawn_time += fElapsedTime;
    pge->Clear(olc::VERY_DARK_GREY * 0.8);
    GameState next_state = GameState::Menu;

    
    // Draw some title text
    {
        pge->SetDrawTarget(static_cast<uint8_t>(0));
        auto draw = [this](const std::string& text, float s, olc::Pixel color, olc::vf2d offset = {}) {
            olc::vf2d scale {s + std::sinf(fTotalTime), s + std::cosf(fTotalTime)};
            const olc::vf2d size = pge->GetTextSize(text) * scale;
            const olc::vf2d pos = (pge->GetScreenSize() - size) / 2.0f + offset;
            pge->DrawStringDecal(pos, text, color, scale);
            
        };

        for(int i = 0; i < 256; i++) {
            olc::Pixel color = {static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i), 255};
            float f = 20.0 + i * 0.01f;
            float f2 = 5.0f + i * 0.001f;
            draw("SHAPES", f, color);
            draw("Click to Start", f2, color, {0.0f, 200.0f});
        }       
        pge->SetDrawTarget(static_cast<uint8_t>(1));
    }


    {
        // If the spawn time has elapsed, spawn a new background shape until we get to the max
        if(spawn_time > spawn_rate) {
            spawn_time -= spawn_rate;

            // The lerp time should ensure that no more than 10 ever exist, but check jsut to be safe
            if(reg.storage<LerpComponent>().size() < 15) {
                auto entity = reg.create();
                utilities::random::uniform_real_distribution<float> dist {0, static_cast<float>(olc::utils::geom2d::pi) * 2.0f};
			    const float start_angle = dist(rng);
                const float end_angle = start_angle + static_cast<float>(olc::utils::geom2d::pi) + (dist(rng) - static_cast<float>(olc::utils::geom2d::pi)) / 4.0f;
                
                LerpComponent l;
                l.start = olc::vf2d{pge->ScreenWidth(), start_angle}.cart() + pge->GetScreenSize() / 2.0f;
                l.end = olc::vf2d{pge->ScreenWidth(), end_angle}.cart() + pge->GetScreenSize() / 2.0f;
                l.total_time = 10.0f + dist(rng);
                
                Shape s{prototypes[static_cast<ShapePrototypes>(rand() % prototypes.size())]};
                s.MoveTo(l.start);
                s.scale = 20.0f + 3.0f * dist(rng);
                s.color = utilities::RandomColor();
                s.color.a = 128;
                
                reg.emplace<LerpComponent>(entity, l);
			    reg.emplace<Shape>(entity, s);
            }
        }
    }

    {
        // Move all the current shapes
        auto view = reg.view<LerpComponent, Shape>();
        for(auto entity : view) {
            auto [l, s] = view.get(entity);
            l.current_time += fElapsedTime;
            if(l.current_time > l.total_time) {
                reg.destroy(entity);
                continue;
            }

            auto pos = utilities::lerp(l.start, l.end, l.current_time / l.total_time);
            auto sign = (entt::to_integral(entity) & 0x1) ? -1 : 1;
            s.theta += fElapsedTime * (s.scale / 40.0f) * sign;
            s.MoveTo(pos);
        }
    }

    {
        // Draw all the shapes
        auto view = reg.view<Shape>();
        view.each([this](const Shape& s){s.Draw(pge);});
    }

    {
        olc::Pixel color = olc::VERY_DARK_GREY * 0.8;
        color.a = utilities::lerp(255, 0, std::min(1.0f, fTotalTime / 2.0f));
        pge->SetDrawTarget(static_cast<uint8_t>(0));
        pge->FillRectDecal({0.0f, 0.0f}, pge->GetScreenSize(), color);
        pge->SetDrawTarget(static_cast<uint8_t>(1));
    }

    if(pge->GetMouse(0).bPressed) {
        next_state = GameState::Gameplay;
    }

    return next_state;
}
