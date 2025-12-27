#include "audio.h"
#include <SDL3/SDL_audio.h>
#include <iostream>
#include <algorithm>

static inline float Clamp01(float v) {
    return (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
}

SoundManager gSoundManager;

SoundManager::SoundManager() = default;

SoundManager::~SoundManager() {
    Shutdown();
}

bool SoundManager::Init() {
    // Open default playback device
    SDL_AudioSpec want{};
    want.freq = 44100;
    want.format = SDL_AUDIO_S16LE;
    want.channels = 2;

    deviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want);
    if (deviceId == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        return false;
    }

    // Get actual device format
    SDL_GetAudioDeviceFormat(deviceId, &deviceSpec, nullptr);

    SDL_ResumeAudioDevice(deviceId);
    return true;
}

void SoundManager::Shutdown() {
    sounds.clear();
    for (auto &kv : soundStreams) {
        if (kv.second) {
            SDL_DestroyAudioStream(kv.second);
        }
    }
    soundStreams.clear();
    for (auto *s : coinStreams) {
        if (s) SDL_DestroyAudioStream(s);
    }
    coinStreams.clear();
    if (musicStream) {
        SDL_DestroyAudioStream(musicStream);
        musicStream = nullptr;
    }
    musicData.clear();
    if (deviceId != 0) {
        SDL_CloseAudioDevice(deviceId);
        deviceId = 0;
    }
}

bool SoundManager::LoadSound(const std::string& name, const std::string& filename) {
    Uint8* buffer = nullptr;
    Uint32 length = 0;
    SDL_AudioSpec wavSpec;

    if (!SDL_LoadWAV(filename.c_str(), &wavSpec, &buffer, &length)) {
        std::cerr << "Failed to load WAV " << filename << ": " << SDL_GetError() << std::endl;
        return false;
    }

    // Convert WAV to device format using a temporary stream
    SDL_AudioStream* convertStream = SDL_CreateAudioStream(&wavSpec, &deviceSpec);
    if (!convertStream || SDL_PutAudioStreamData(convertStream, buffer, length) < 0) {
        std::cerr << "Audio conversion error: " << SDL_GetError() << std::endl;
        SDL_free(buffer);
        if (convertStream) SDL_DestroyAudioStream(convertStream);
        return false;
    }

    SDL_FlushAudioStream(convertStream);

    std::vector<Uint8> converted;
    int avail = SDL_GetAudioStreamAvailable(convertStream);
    if (avail > 0) {
        converted.resize(avail);
        SDL_GetAudioStreamData(convertStream, converted.data(), avail);
    }

    SDL_DestroyAudioStream(convertStream);
    SDL_free(buffer);

    sounds[name] = std::move(converted);
    // create a bound stream for this sound (one per sound so SDL can mix multiple at once)
    if (!soundStreams.count(name)) {
        soundStreams[name] = CreateBoundStream();
    }
    return true;
}

SDL_AudioStream* SoundManager::CreateBoundStream() {
    if (deviceId == 0) return nullptr;
    SDL_AudioStream *s = SDL_CreateAudioStream(&deviceSpec, &deviceSpec);
    if (!s) return nullptr;
    if (SDL_BindAudioStream(deviceId, s) < 0) {
        SDL_DestroyAudioStream(s);
        return nullptr;
    }
    return s;
}

void SoundManager::StopSound(const std::string& name) {
    auto itStream = soundStreams.find(name);
    if (itStream == soundStreams.end()) return;
    if (itStream->second) {
        SDL_DestroyAudioStream(itStream->second);
        itStream->second = nullptr;
    }
}

void SoundManager::CleanupCoinStreams() {
    coinStreams.erase(
        std::remove_if(coinStreams.begin(), coinStreams.end(), [](SDL_AudioStream* s){
            if (!s) return true;
            // If nothing left queued, consider it done
            int avail = SDL_GetAudioStreamAvailable(s);
            if (avail <= 0) {
                SDL_DestroyAudioStream(s);
                return true;
            }
            return false;
        }),
        coinStreams.end());
}

void SoundManager::Update() {
    CleanupCoinStreams();
    if (musicStream && !musicData.empty()) {
        int avail = SDL_GetAudioStreamAvailable(musicStream);
        if (avail < static_cast<int>(musicData.size() / 2)) {
            SDL_PutAudioStreamData(musicStream, musicData.data(), static_cast<int>(musicData.size()));
        }
    }
}

// Specialized handling for coin: allow overlapping by spinning up a temporary stream per play.
// The default PlaySound will be used for others.
void SoundManager::PlaySound(const std::string& name) {
    if (name == "coin") {
        auto it = sounds.find(name);
        if (it == sounds.end() || it->second.empty()) return;
        if (deviceId == 0) return;

        CleanupCoinStreams();

        SDL_AudioStream *s = CreateBoundStream();
        if (!s) return;
        SDL_SetAudioStreamGain(s, sfxVolume);
        if (SDL_PutAudioStreamData(s, it->second.data(), it->second.size()) < 0) {
            SDL_DestroyAudioStream(s);
            return;
        }
        coinStreams.push_back(s);
        return;
    }

    auto it = sounds.find(name);
    auto itStream = soundStreams.find(name);
    if (it == sounds.end() || it->second.empty()) return;

    if (itStream == soundStreams.end() || itStream->second == nullptr) {
        soundStreams[name] = CreateBoundStream();
        itStream = soundStreams.find(name);
    }
    SDL_AudioStream *s = itStream->second;
    if (!s) return;

    if (name == "jump") {
        SDL_DestroyAudioStream(s);
        soundStreams[name] = CreateBoundStream();
        s = soundStreams[name];
        if (!s) return;
    }

    SDL_SetAudioStreamGain(s, sfxVolume);
    SDL_PutAudioStreamData(s, it->second.data(), it->second.size());
}

void SoundManager::PlayMusic(const std::string& filename, float volume) {
    musicVolume = Clamp01(volume);
    if (deviceId == 0) return;

    Uint8* buffer = nullptr;
    Uint32 length = 0;
    SDL_AudioSpec wavSpec;

    if (!SDL_LoadWAV(filename.c_str(), &wavSpec, &buffer, &length)) {
        std::cerr << "Failed to load music WAV " << filename << ": " << SDL_GetError() << std::endl;
        return;
    }

    SDL_AudioStream* convertStream = SDL_CreateAudioStream(&wavSpec, &deviceSpec);
    if (!convertStream || SDL_PutAudioStreamData(convertStream, buffer, length) < 0) {
        std::cerr << "Music audio conversion error: " << SDL_GetError() << std::endl;
        SDL_free(buffer);
        if (convertStream) SDL_DestroyAudioStream(convertStream);
        return;
    }

    SDL_FlushAudioStream(convertStream);

    std::vector<Uint8> converted;
    int avail = SDL_GetAudioStreamAvailable(convertStream);
    if (avail > 0) {
        converted.resize(avail);
        SDL_GetAudioStreamData(convertStream, converted.data(), avail);
    }

    SDL_DestroyAudioStream(convertStream);
    SDL_free(buffer);

    if (converted.empty()) return;

    if (!musicStream) {
        musicStream = CreateBoundStream();
    } else {
        SDL_ClearAudioStream(musicStream);
    }
    if (!musicStream) return;

    SDL_SetAudioStreamGain(musicStream, musicVolume);
    musicData = std::move(converted);
    SDL_PutAudioStreamData(musicStream, musicData.data(), static_cast<int>(musicData.size()));
}

void SoundManager::StopMusic() {
    if (musicStream) {
        SDL_DestroyAudioStream(musicStream);
        musicStream = nullptr;
    }
    musicData.clear();
}

void SoundManager::SetMusicVolume(float v) {
    musicVolume = Clamp01(v);
    if (musicStream) {
        SDL_SetAudioStreamGain(musicStream, musicVolume);
    }
}

void SoundManager::SetSfxVolume(float v) {
    sfxVolume = Clamp01(v);
    for (auto &kv : soundStreams) {
        if (kv.second) {
            SDL_SetAudioStreamGain(kv.second, sfxVolume);
        }
    }
    for (auto *s : coinStreams) {
        if (s) {
            SDL_SetAudioStreamGain(s, sfxVolume);
        }
    }
}
