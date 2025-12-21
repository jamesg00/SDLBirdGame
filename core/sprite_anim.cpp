#include "sprite_anim.h"
#include <SDL3_image/SDL_image.h>  // Required for both IMG_LoadTexture and IMG_GetError
#include <cstdio>

void SpriteAnim::LoadFrames(const char *base, int count, SDL_Renderer *r) {
    DestroyFrames();
    frames.reserve(count);
    for (int i = 0; i < count; ++i) {
        char path[128];
        snprintf(path, sizeof(path), "%s%d.png", base, i);  // "Running0.png", "Flying0.png" etc.
        printf("Trying to load sprite: %s\n", path);
        SDL_Texture *f = IMG_LoadTexture(r, path);
        if (f) {
            frames.push_back(f);
            printf("Loaded %s successfully\n", path);
        } else {
            printf("Failed to load %s: %s\n", path, SDL_GetError());  // <-- CHANGED HERE
        }
    }
}

void SpriteAnim::DestroyFrames() {
    for (auto f : frames) {
        if (f) SDL_DestroyTexture(f);
    }
    frames.clear();
}

void SpriteAnim::Init(float ft, int count) {
    frameTime = ft;
    if (count > 0) frames.reserve(count);
    Reset();
}

void SpriteAnim::Reset() {
    timer = 0.0f;
    frame = 0;
}

bool SpriteAnim::Update(float dt) {
    if (frames.empty()) return false;

    timer += dt;
    bool didLoop = false;
    while (timer >= frameTime) {
        timer -= frameTime;
        frame++;
        if (frame >= static_cast<int>(frames.size())) {
            frame = loop ? 0 : frames.size() - 1;
            didLoop = true;
        }
    }
    return didLoop;
}

SDL_Texture *SpriteAnim::Current() const {
    if (frames.empty() || frame < 0 || frame >= static_cast<int>(frames.size())) return nullptr;
    return frames[frame];
}

void SpriteAnim::Render(SDL_Renderer *r, float x, float y, float w, float h, float angle, SDL_FPoint *center, SDL_FlipMode flip) {
    SDL_Texture *tex = Current();
    if (!tex) {
        printf("No texture for frame %d - skipping render\n", frame);
        return;
    }
    SDL_FRect dst{x - w * 0.5f, y - h * 0.5f, w, h};
    SDL_RenderTextureRotated(r, tex, nullptr, &dst, angle, center, flip);
}