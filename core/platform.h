#ifndef PLATFORM_H
#define PLATFORM_H

#include <SDL3/SDL.h>

struct Platform {
    SDL_FPoint pos;
    float initialY = 0.0f;  // store starting Y for sine wave
    bool alive = true;
    float speed = 0.0f;
    float amp = 0.0f;   // amplitude of sine wave
    float freq = 0.0f;  // frequency
    float phase = 0.0f;
    float bobTimer = 0.0f;

    static constexpr float width = 120.0f;
    static constexpr float height = 24.0f;

    Platform(float x, float y, float spd, float a, float f, float ph);

    void Update(float dt);

    void Render(SDL_Renderer *r);

    SDL_FRect Bounds() const;
};

#endif // PLATFORM_H