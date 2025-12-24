#include "platform.h"
#include <cmath>  // sinf

Platform::Platform(float x, float y, float spd, float a, float f, float ph)
    : pos{x, y}, initialY(y), speed(spd), amp(a), freq(f), phase(ph) {}

void Platform::Update(float dt) {
    if (!alive) return;

    // Move left like coins
    pos.x -= speed * dt;

    // Sine wave vertical motion
    bobTimer += dt;
    pos.y = initialY + amp * SDL_sinf(freq * bobTimer + phase);

    // Kill if off-screen left
    if (pos.x < -width - 50.0f) {
        alive = false;
    }
}

void Platform::Render(SDL_Renderer *r) {
    if (!alive) return;

    // Simple brown platform (you can replace with a texture later)
    SDL_SetRenderDrawColor(r, 139, 69, 19, 255);  // brown
    SDL_FRect dst{ pos.x - width * 0.5f, pos.y - height * 0.5f, width, height };
    SDL_RenderFillRect(r, &dst);

    // Optional border
    SDL_SetRenderDrawColor(r, 101, 67, 33, 255);  // darker brown
    SDL_RenderRect(r, &dst);
}

SDL_FRect Platform::Bounds() const {
    return SDL_FRect{ pos.x - width * 0.5f, pos.y - height * 0.5f, width, height };
}