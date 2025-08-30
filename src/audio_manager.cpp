#include "audio_manager.hpp"

#include "utilities/global_rng.hpp"

void AudioManager::Load(const std::string& filepath, olc::MiniAudio* audio) {
    std::ifstream in{filepath};

    std::string name;
    std::string path;

    while(in >> name >> path) {
        auto id = audio->LoadSound(path);
        //std::cout << id << " " << name << " " << path << std::endl;

        sounds[name].push_back(id);
    }
}

int AudioManager::RandomSound(const std::string& name) const {
    const auto& sound_bank = sounds.at(name);

    std::uniform_int_distribution<size_t> dist {0, sound_bank.size() - 1};
    return sound_bank.at(dist(rng));
}

const std::vector<int>& AudioManager::GetSoundBank(const std::string& name) const {
    return sounds.at(name);
}

std::map<std::string, std::vector<int>> sounds;
