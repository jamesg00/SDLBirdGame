#include "utils.h"

void GetLogicalMouse(float &lx, float &ly, Uint32 &buttons) {
    float wx = 0.0f, wy = 0.0f;
    buttons = SDL_GetMouseState(&wx, &wy);
    if (!SDL_RenderCoordinatesFromWindow(renderer, wx, wy, &lx, &ly)) {
        lx = wx;
        ly = wy;
    }
}

void GetLogicalMouse(float &lx, float &ly) {
    Uint32 dummy = 0;
    GetLogicalMouse(lx, ly, dummy);
}