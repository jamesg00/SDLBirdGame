#ifndef FLOATING_TEXT_H
#define FLOATING_TEXT_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>
#include "../core/platform.h"

extern TTF_Font *font;  // from main.cpp
extern std::vector<Platform> platforms;

struct FloatingText {
    SDL_FPoint pos;
    std::string text;
    SDL_Color color{255, 255, 255, 255};
    float timer = 0.0f;
    float lifetime = 1.0f;
    bool alive = true;
    float blinkTimer = 0.0f;
    float velocityY = -50.0f;  // float up

    FloatingText(float x, float y, const std::string &txt, SDL_Color c = {255, 255, 255, 255});

    void Update(float dt);

    void Render(SDL_Renderer *r);
};

#endif // FLOATING_TEXT_H