#ifndef UTILS_H
#define UTILS_H

#include <SDL3/SDL.h>

static const int kWinW = 480;
static const int kWinH = 480;

// Forward declare renderer if needed elsewhere later
extern SDL_Renderer *renderer;

void GetLogicalMouse(float &lx, float &ly, Uint32 &buttons);
void GetLogicalMouse(float &lx, float &ly);

inline float ClampF(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

inline bool AABBOverlap(const SDL_FRect &a, const SDL_FRect &b) {
    return (a.x < b.x + b.w) && (a.x + a.w > b.x) &&
           (a.y < b.y + b.h) && (a.y + a.h > b.y);
}

#endif // UTILS_H