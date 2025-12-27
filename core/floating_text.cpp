#include "floating_text.h"
#include <cmath>
#include <cstring>

extern SDL_Renderer *renderer;
extern TTF_Font *font;  // We need the global font from main.cpp

FloatingText::FloatingText(float x, float y, const std::string &txt, SDL_Color c)
    : pos{x, y}, text(txt), color(c), velocityY(-50.0f) {}

void FloatingText::Update(float dt) {
    if (!alive) return;

    timer += dt;
    blinkTimer += dt;
    pos.y += velocityY * dt;  // float upward

    if (timer >= lifetime) {
        alive = false;
    }
}

void FloatingText::Render(SDL_Renderer *r) {
    if (!alive || !font) return;

    // Arcade-style blinking
    Uint8 alpha = (fmodf(blinkTimer, 0.2f) < 0.1f) ? 255 : 180;

    SDL_Color renderColor = color;
    SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), 0, renderColor);
    if (!surface) {
        printf("TTF_RenderText_Solid failed: %s\n", SDL_GetError());
        return;
    }

    SDL_Surface *shadowSurface = TTF_RenderText_Blended(font, text.c_str(), 0, SDL_Color{0,0,0,255});

    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surface);
    SDL_Texture *shadowTex = shadowSurface ? SDL_CreateTextureFromSurface(r, shadowSurface) : nullptr;
    if (shadowTex) SDL_SetTextureAlphaMod(shadowTex, 180);
    if (tex) {
        float w = static_cast<float>(surface->w);
        float h = static_cast<float>(surface->h);

        SDL_FRect shadowDst{
            pos.x - w * 0.5f + 2.0f,
            pos.y - h * 0.5f + 2.0f,
            w, h
        };
        SDL_FRect dst{
            pos.x - w * 0.5f,   // centered horizontally
            pos.y - h * 0.5f,   // centered vertically
            w,
            h
        };

        if (shadowTex) {
            SDL_RenderTexture(r, shadowTex, nullptr, &shadowDst);
        }
        SDL_SetTextureAlphaMod(tex, alpha);
        SDL_RenderTexture(r, tex, nullptr, &dst);
    }

    if (tex) SDL_DestroyTexture(tex);
    if (shadowTex) SDL_DestroyTexture(shadowTex);
    SDL_DestroySurface(surface);  // SDL3: NOT SDL_FreeSurface
    if (shadowSurface) SDL_DestroySurface(shadowSurface);
}
