#ifndef BAT_H
#define BAT_H

#include <SDL3/SDL.h>

// Declare the global texture arrays here (defined in main.cpp)
extern SDL_Texture *batMoveFrames[16];
extern SDL_Texture *batDeathFrames[8];

enum class BatState { Moving, Dying };

struct Bat {
    SDL_FPoint pos;
    SDL_FPoint vel;
    BatState state = BatState::Moving;
    float animTimer = 0.0f;
    int frame = 0;
    float speed = 200.0f;
    bool alive = true;
    float deathTimer = 0.0f;
    float bobTimer = 0.0f;
    float hitFlash = 0.0f;
    float trackTimer = 0.0f;

    Bat(float x, float y, float spd) : pos{x, y}, vel{0.0f, 0.0f}, speed(spd) {}

    void Update(float dt, const SDL_FPoint &target);

    void Kill();

    void Render(SDL_Renderer *r);

    SDL_FRect Bounds() const;
};

#endif // BAT_H