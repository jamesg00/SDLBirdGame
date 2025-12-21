#include "bat.h"

void Bat::Update(float dt, const SDL_FPoint &target) {
    if (!alive) return;
    if (hitFlash > 0.0f) {
        hitFlash -= dt;
        if (hitFlash < 0.0f) hitFlash = 0.0f;
    }
    if (state == BatState::Moving) {
        // simple seek
        float dx = target.x - pos.x;
        float dy = target.y - pos.y;
        float len = SDL_sqrtf(dx*dx + dy*dy);
        SDL_FPoint desired{0.0f, 0.0f};
        if (len > 0.001f) {
            desired.x = dx / len;
            desired.y = dy / len;
        }
        // limited tracking window so player can dodge past
        trackTimer += dt;
        if (trackTimer < 1.2f) {
            float steer = 0.6f; // partial follow
            vel.x = vel.x * (1.0f - steer) + desired.x * speed * steer;
            vel.y = vel.y * (1.0f - steer) + desired.y * speed * steer;
        }
        // normalize velocity to maintain speed
        float vlen = SDL_sqrtf(vel.x*vel.x + vel.y*vel.y);
        if (vlen > 0.001f) {
            vel.x = vel.x / vlen * speed;
            vel.y = vel.y / vlen * speed;
        }
        // bob adds variability
        vel.y += SDL_sinf(bobTimer * 4.0f) * 30.0f;
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;

        // animate move
        animTimer += dt;
        const float ft = 0.06f;
        while (animTimer >= ft) {
            animTimer -= ft;
            frame = (frame + 1) % 15;
        }
        bobTimer += dt;
    } else {
        // death animation advance
        deathTimer += dt;
        const float ft = 0.08f;
        animTimer += dt;
        while (animTimer >= ft) {
            animTimer -= ft;
            if (frame < 6) frame++;
        }
        if (frame >= 6 && deathTimer > 0.6f) {
            alive = false;
        }
    }
}

void Bat::Kill() {
    if (state == BatState::Moving) {
        state = BatState::Dying;
        frame = 0;
        animTimer = 0.0f;
        deathTimer = 0.0f;
        hitFlash = 0.2f;
    }
}

void Bat::Render(SDL_Renderer *r) {
    if (!alive) return;
    SDL_Texture *tex = nullptr;
    if (state == BatState::Moving) {
        tex = batMoveFrames[frame];
    } else {
        tex = batDeathFrames[frame];
    }
    if (!tex) return;
    float w = 72.0f, h = 54.0f;
    SDL_FRect dst{ pos.x - w * 0.5f, pos.y - h * 0.5f, w, h };
    bool blink = (state == BatState::Dying) && ((SDL_GetTicks() / 80) % 2 == 0);
    if (blink) {
        SDL_SetTextureAlphaMod(tex, 140);
    }
    if (hitFlash > 0.0f) {
        SDL_SetTextureColorMod(tex, 255, 220, 220);
    }
    SDL_RenderTexture(r, tex, nullptr, &dst);
    if (hitFlash > 0.0f) {
        SDL_SetTextureColorMod(tex, 255, 255, 255);
    }
    if (blink) {
        SDL_SetTextureAlphaMod(tex, 255);
    }
}

SDL_FRect Bat::Bounds() const {
    float w = 64.0f, h = 46.0f;
    return SDL_FRect{ pos.x - w * 0.5f, pos.y - h * 0.5f, w, h };
}