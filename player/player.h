#ifndef PLAYER_H
#define PLAYER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "../core/sprite_anim.h"
#include "../core/utils.h"
#include "../core/projectile.h"
#include <vector>

// Animations and projectile pool are owned in main.cpp.
extern SpriteAnim runningAnim;
extern SpriteAnim flyingAnim;
extern std::vector<Projectile> projectiles;
extern bool playerDead;
extern TTF_Font *font;
extern bool spreadActive;

enum class AnimState {
    Running,
    Flying
};

struct Player {
    void Init();
    void Reset();
    void Update(float dt);
    void Render(SDL_Renderer *r) const;
    void SetAutopilot(bool enabled);

    SDL_FRect Bounds() const;
    SDL_FPoint Center() const;

    bool dead = false;

private:
    SDL_FRect rect{300.0f, 50.0f, 40.0f, 40.0f};
    SDL_FPoint velocity{0.0f, 0.0f};
    AnimState animState = AnimState::Running;
    int flyingLoopsRemaining = 0;
    bool facingRight = true;
    bool wasSpaceDown = false;
    bool wasMouseDown = false;

    static constexpr float kGravity = 900.0f;
    static constexpr float kJumpImpulse = -350.0f;
    static constexpr float kFloorY = 480.0f;

    bool autopilot = false;
    float autoTimer = 0.0f;
};

#endif // PLAYER_H
