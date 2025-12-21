#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <SDL3/SDL.h>

extern SDL_Texture *projectileFrames[30];  // defined in main.cpp

struct Projectile {
    SDL_FPoint pos;
    SDL_FPoint vel;
    float angle;
    float timer;
    int frame;
    bool alive;

    Projectile(float x, float y, float vx, float vy, float ang);

    void Update(float dt);

    void Render(SDL_Renderer *r);

    SDL_FRect Bounds() const;  // Added for modularity (matches hardcoded collision box)
};

#endif // PROJECTILE_H