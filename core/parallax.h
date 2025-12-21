#ifndef PARALLAX_H
#define PARALLAX_H

#include <SDL3/SDL.h>

struct ParallaxLayer {
    SDL_Texture *tex = nullptr;
    float speed = 0.0f;
    float y = 0.0f;
    float x = 0.0f;
    float texW = 0.0f;
    float texH = 0.0f;

    void SetTexture(SDL_Texture *t) {
        tex = t;
        float w = 0.0f, h = 0.0f;
        if (tex && SDL_GetTextureSize(tex, &w, &h)) {
            texW = w;
            texH = h;
        }
    }

    void Update(float dt) {
        if (!tex) return;
        x -= speed * dt;
        while (x <= -texW) x += texW;
        while (x >= texW) x -= texW;
    }

    void SetSpeed(float s) { speed = s; }

    void Render(SDL_Renderer *r, float viewportW) {
        if (!tex) return;

        // start one tile to the left of the screen
        float start = x;
        while (start > -texW) start -= texW;

        // draw until we've covered the viewport plus one extra tile
        for (float px = start; px < viewportW + texW; px += texW) {
            SDL_FRect dst{ px, y, texW, texH };
            SDL_RenderTexture(r, tex, nullptr, &dst);
        }
    }
};

#endif // PARALLAX_H