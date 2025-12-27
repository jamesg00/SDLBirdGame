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
    SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), (int)strlen(text.c_str()), renderColor);
    if (!surface) {
        printf("TTF_RenderText_Solid failed: %s\n", SDL_GetError());
        return;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surface);
    if (tex) {
        float w = static_cast<float>(surface->w);
        float h = static_cast<float>(surface->h);

        SDL_FRect dst{
            pos.x - w * 0.5f,   // centered horizontally
            pos.y - h * 0.5f,   // centered vertically
            w,
            h
        };

        SDL_SetTextureAlphaMod(tex, alpha);
        SDL_RenderTexture(r, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }

    SDL_DestroySurface(surface);  // SDL3: NOT SDL_FreeSurface
}
