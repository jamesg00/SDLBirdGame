#ifndef COIN_H
#define COIN_H

#include <SDL3/SDL.h>

extern SDL_Texture *coinFrames[4];  // defined in main.cpp

struct Coin {
    SDL_FPoint pos;
    bool alive = true;
    float animTimer = 0.0f;
    int frame = 0;
    float speed = 0.0f;
    float amp = 0.0f;
    float freq = 0.0f;
    float phase = 0.0f;
    float bobTimer = 0.0f;

    Coin(float x, float y, float spd, float a, float f, float ph);

    void Update(float dt);

    void Render(SDL_Renderer *r);

    SDL_FRect Bounds() const;
};

#endif // COIN_H