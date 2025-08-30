#pragma once

#include "olcPGEX_MiniAudio.h"

#include "components.hpp"
#include "system.hpp"

extern olc::MiniAudio* audio;


struct MusicSystem : public System {
	MusicSystem(entt::dispatcher& dispatcher, entt::entity player, entt::registry& reg, olc::PixelGameEngine* pge) : dispatcher(dispatcher), player_entity(player), System(reg, pge) {
		dispatcher.sink<PlayMusic>().connect<&MusicSystem::on_music_play>(this);
	};

	~MusicSystem() {
    	dispatcher.sink<PlayMusic>().disconnect(this);
		audio->Stop(current_music);
		//std::cout << "dtor" << std::endl;
	}

	void on_music_play(const PlayMusic& music) {
		//std::cout << music.sound_id << " " << music.fade_rate << std::endl;
		next_music = music.sound_id;
		fade_rate = music.fade_rate;
		is_fading = true;
		if(next_music != 0) {
			audio->SetVolume(next_music, 0.0f);
			audio->Play(next_music, true);
		}
	}

	void OnUserUpdate(float fElapsedTime) override {
		if(is_fading) {
			bool done = false;
			primary_volume -= fElapsedTime * fade_rate;
			
			//std::cout << primary_volume << std::endl;

			if(primary_volume <= 0.0f) {
				done = true;
			}
			
			primary_volume = std::max(0.0f, std::min(primary_volume, 1.0f));

			if(current_music != 0) {
				audio->SetVolume(current_music, utilities::lerp(0.0f, max_volume, utilities::Ease(primary_volume)));
			}

			if(next_music != 0) {
				audio->SetVolume(next_music, utilities::lerp(max_volume, 0.0f, utilities::Ease(primary_volume)));
			}

			// If we're done fading to the next track, stop the current (now muted) song and reset everything
			// to work on the next song
			if(done) {
				audio->Stop(current_music);
				current_music = next_music;
				next_music = 0;
				primary_volume = 1.0f;
				is_fading = false;
			}
		}
	}
private:
	int current_music {0};
	int next_music {0};
	float fade_rate {1.0f};
	float primary_volume {1.0f};
	float max_volume {0.7f};
	bool is_fading {false};
	entt::dispatcher& dispatcher;
	entt::entity player_entity;
};
