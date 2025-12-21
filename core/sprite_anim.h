#ifndef SPRITE_ANIM_H
#define SPRITE_ANIM_H

#include <SDL3/SDL.h>
#include <vector>
#include <string>

struct SpriteAnim {
    std::vector<SDL_Texture *> frames;
    float frameTime = 0.1f;
    float timer = 0.0f;
    int frame = 0;
    bool loop = true;

    void LoadFrames(const char *base, int count, SDL_Renderer *r);

    void DestroyFrames();

    void Init(float ft, int count = 0);  // sets frameTime, optionally reserves

    void Reset();

    bool Update(float dt);  // returns true if looped

    SDL_Texture *Current() const;

    void Render(SDL_Renderer *r, float x, float y, float w, float h, float angle = 0.0f, SDL_FPoint *center = nullptr, SDL_FlipMode flip = SDL_FLIP_NONE);
};

#endif // SPRITE_ANIM_H