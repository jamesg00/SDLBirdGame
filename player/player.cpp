#include "player.h"
#include <cmath>

void Player::Init() {
    Reset();
}

void Player::Reset() {
    rect = {kWinW * 0.5f - 20.0f, kWinH * 0.5f - 20.0f, 40.0f, 40.0f};
    velocity = {0.0f, 0.0f};
    animState = AnimState::Running;
    flyingLoopsRemaining = 0;
    facingRight = true;
    wasSpaceDown = false;
    wasMouseDown = false;
    dead = false;
    autopilot = false;
    autoTimer = 0.0f;
    runningAnim.Reset();
    flyingAnim.Reset();
}

void Player::Update(float dt) {
    if (dead) return;

    // Keyboard input
    const bool *keys = SDL_GetKeyboardState(nullptr);
    bool leftDown = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    bool rightDown = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    bool spaceDown = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_W];

    if (autopilot && spaceDown) {
        autopilot = false;
    }

    // Gentle hover before the game starts
    if (autopilot) {
        autoTimer += dt;
        velocity = {0.0f, 0.0f};
        const float baseY = kWinH * 0.5f - rect.h * 0.5f;
        const float amp = 12.0f;
        rect.y = baseY + SDL_sinf(autoTimer * 2.2f) * amp;
        animState = AnimState::Flying;
        flyingAnim.Update(dt);
        runningAnim.Reset();
        wasSpaceDown = spaceDown;
        // skip the rest; stay hovering
        return;
    }

    if (leftDown && !rightDown) {
        velocity.x = -200.0f;
        facingRight = false;
    } else if (rightDown && !leftDown) {
        velocity.x = 200.0f;
        facingRight = true;
    } else {
        velocity.x = 0.0f;
    }

    if (autopilot) {
        autoTimer += dt;
        if (autoTimer >= 0.8f) {
            autoTimer = 0.0f;
            velocity.y = kJumpImpulse;
            animState = AnimState::Flying;
            flyingAnim.Reset();
        }
    }

    if (spaceDown && !wasSpaceDown) {
        velocity.y = kJumpImpulse;
        flyingLoopsRemaining = 1;
        animState = AnimState::Flying;
        flyingAnim.Reset();
    }
    wasSpaceDown = spaceDown;

    // Gravity and integration
    velocity.y += kGravity * dt;
    rect.x += velocity.x * dt;
    rect.y += velocity.y * dt;

    // Bounds
    const float floor = kFloorY - rect.h;
    if (rect.y > floor) {
        rect.y = floor;
        velocity.y = 0.0f;
    }
    if (rect.x < 0.0f) {
        rect.x = 0.0f;
        velocity.x = 0.0f;
    } else if (rect.x + rect.w > 480.0f) {
        rect.x = 480.0f - rect.w;
        velocity.x = 0.0f;
    }
    if (rect.y < 0.0f) {
        rect.y = 0.0f;
        velocity.y = 0.0f;
    }

    // Animation update
    if (animState == AnimState::Flying) {
        bool looped = flyingAnim.Update(dt);
        if (looped && flyingLoopsRemaining > 0) {
            flyingLoopsRemaining--;
            if (flyingLoopsRemaining <= 0) {
                animState = AnimState::Running;
                runningAnim.Reset();
            }
        }
    } else {
        runningAnim.Update(dt);
        if (!spaceDown) {
            animState = AnimState::Running;
        }
    }


    

    // Shooting (left click) disabled during autopilot
    float mx = 0.0f, my = 0.0f;
    Uint32 mouseState = 0;
    GetLogicalMouse(mx, my, mouseState);
    bool mouseDown = (mouseState & SDL_BUTTON_LMASK) != 0;

    if (!autopilot) {
        if (mouseDown && !wasMouseDown) {
            float pcx = rect.x + rect.w * 0.5f;
            float pcy = rect.y + rect.h * 0.5f;

            float dx = mx - pcx;
            float dy = my - pcy;
            float len = SDL_sqrtf(dx*dx + dy*dy);
            if (len < 0.001f) len = 0.001f;
            dx /= len; dy /= len;

            const float speed = 400.0f;
            float vx = dx * speed;
            float vy = dy * speed;
            float angle = SDL_atan2(dy, dx) * (180.0f / SDL_PI_D);

            projectiles.emplace_back(pcx, pcy, vx, vy, angle);
        }
    }
    wasMouseDown = mouseDown;

    bool onPlatform = false;

    for (const auto &plat : platforms) {
        SDL_FRect playerBox = Bounds();
        if (AABBOverlap(playerBox, plat.Bounds()) && velocity.y >= 0.0f) {  // falling or standing
            rect.y = plat.Bounds().y - rect.h;  // snap to top of platform
            velocity.y = 0.0f;
            // You can set a flag for 2x score here
            onPlatform = true;
            break;
        }
    }
    this->onPlatform = onPlatform;
}

void Player::Render(SDL_Renderer *r) const {
    if (dead) return;

    SDL_Texture *playerTex = (animState == AnimState::Running) ? runningAnim.Current() : flyingAnim.Current();
    if (!playerTex) return;

    SDL_FlipMode flip = facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    SDL_RenderTextureRotated(r, playerTex, nullptr, &rect, 0.0, nullptr, flip);
}

SDL_FRect Player::Bounds() const {
    return rect;
}

SDL_FPoint Player::Center() const {
    return {rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f};
}

void Player::SetAutopilot(bool enabled) {
    autopilot = enabled;
    autoTimer = 0.0f;
}
