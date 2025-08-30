#pragma once

#include "olcPGEX_MiniAudio.h"

struct AudioManager {
	void Load(const std::string& filepath, olc::MiniAudio* audio);
	
	int RandomSound(const std::string& name) const;

    //int CurrentSong() const;

	const std::vector<int>& GetSoundBank(const std::string& name) const;
private:
    //int current_song {0};
    //int next_song {0};
    //float current_song_volume {1.0f};
    //float next_song
	std::map<std::string, std::vector<int>> sounds;
};

extern AudioManager audio_manager;
