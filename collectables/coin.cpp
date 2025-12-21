#include "coin.h"
#include <cmath>     // for sinf
#include <cstdlib>   // for rand()

Coin::Coin(float x, float y, float spd, float a, float f, float ph) 
    : pos{x, y}, speed(spd), amp(a), freq(f), phase(ph) {
    // Randomize starting frame for variety
    frame = rand() % 4;
}

void Coin::Update(float dt) {
    if (!alive) return;

    // Move left
    pos.x -= speed * dt;

    // Spin animation
    animTimer += dt;
    const float frameTime = 0.1f;
    while (animTimer >= frameTime) {
        animTimer -= frameTime;
        frame = (frame + 1) % 4;
    }

    // Bobbing up and down
    bobTimer += dt;

    // Kill if off left screen
    if (pos.x < -50.0f) {
        alive = false;
    }
}

void Coin::Render(SDL_Renderer *r) {
    if (!alive) return;

    SDL_Texture *tex = coinFrames[frame];
    if (!tex) return;

    const float w = 32.0f;
    const float h = 32.0f;
    float bobY = amp * SDL_sinf(freq * bobTimer + phase);

    SDL_FRect dst{ pos.x - w * 0.5f, pos.y - h * 0.5f + bobY, w, h };
    SDL_RenderTexture(r, tex, nullptr, &dst);
}

SDL_FRect Coin::Bounds() const {
    const float w = 28.0f;
    const float h = 28.0f;
    float bobY = amp * SDL_sinf(freq * bobTimer + phase);
    return SDL_FRect{ pos.x - w * 0.5f, pos.y - h * 0.5f + bobY, w, h };
}