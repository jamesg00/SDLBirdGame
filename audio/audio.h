#ifndef AUDIO_H
#define AUDIO_H

#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>

class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    bool Init();
    void Shutdown();

    bool LoadSound(const std::string& name, const std::string& filename);

    void Update(); // cleanup and keep streaming music fed

    void PlaySound(const std::string& name);
    void StopSound(const std::string& name);  // stop a looping/held sound (used for jump)
    void PlayMusic(const std::string& filename, float volume = 1.0f);
    void StopMusic();
    void SetMusicVolume(float v);
    void SetSfxVolume(float v);
    float GetMusicVolume() const { return musicVolume; }
    float GetSfxVolume() const { return sfxVolume; }

private:
    SDL_AudioStream* CreateBoundStream();
    void CleanupCoinStreams();

    SDL_AudioDeviceID deviceId = 0;
    SDL_AudioSpec deviceSpec{};
    std::unordered_map<std::string, std::vector<Uint8>> sounds;  // converted PCM data
    std::unordered_map<std::string, SDL_AudioStream*> soundStreams; // one stream per sound for mixing/restart
    std::vector<SDL_AudioStream*> coinStreams; // allow overlapping coin sounds
    std::vector<Uint8> musicData; // converted PCM for background music
    SDL_AudioStream *musicStream = nullptr;
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
};

extern SoundManager gSoundManager;

#endif // AUDIO_H
