#include "projectile.h"
#include "utils.h"  // for kWinW, kWinH
#include <cmath>

Projectile::Projectile(float x, float y, float vx, float vy, float ang)
    : pos{x, y}, vel{vx, vy}, angle(ang), timer(0.0f), frame(0), alive(true) {}

void Projectile::Update(float dt) {
    if (!alive) return;
    
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    // Animate through frames
    timer += dt;
    const float frameTime = 0.03f; // fast animation
    while (timer >= frameTime) {
        timer -= frameTime;
        frame++;
        if (frame >= 30) {
            frame = 0; // loop animation
        }
    }

    // Remove if off-screen
    if (pos.x < -100 || pos.x > kWinW + 100 || pos.y < -100 || pos.y > kWinH + 100) {
        alive = false;
    }
}

void Projectile::Render(SDL_Renderer *r) {
    if (!alive || !projectileFrames[frame]) return;
    
    float w = 128.0f, h = 128.0f; // projectile size
    SDL_FRect dst{pos.x - w * 0.5f, pos.y - h * 0.5f, w, h};
    SDL_RenderTextureRotated(r, projectileFrames[frame], nullptr, &dst, angle, nullptr, SDL_FLIP_NONE);
}

SDL_FRect Projectile::Bounds() const {
    return SDL_FRect{ pos.x - 10.0f, pos.y - 10.0f, 20.0f, 20.0f };
}
