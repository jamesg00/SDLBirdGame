//Bird Game Concept 
#define SDL_MAIN_USE_CALLBACKS 1 
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include "core/parallax.h"
#include "core/utils.h"
#include "core/projectile.h"
#include "player/bat.h"
#include "player/player.h"
#include "collectables/coin.h"
#include "core/sprite_anim.h"

static SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
static TTF_Font *font = nullptr;
static SDL_Texture *textTex = nullptr;
static SDL_Texture *bgTexture =  NULL;
static SDL_Texture *psTextureL1 = NULL;
static SDL_Texture *psTextureL2 = NULL;
static SDL_Texture *psTextureL3 = NULL;
static SDL_Texture *psTextureL4 = NULL;
static SDL_Texture *psTextureL5 = NULL;
static SDL_Texture *arrowTex = nullptr;
static SDL_Texture *cursorTex = nullptr;
SDL_Texture *coinFrames[4] = {nullptr};
SDL_Texture *batMoveFrames[16] = {nullptr};
SDL_Texture *batDeathFrames[8] = {nullptr};
static SDL_Texture *warningFrames[9] = {nullptr};
static int coinCount = 0;
static float gameTimer = 0.0f; // seconds
bool playerDead = false;

struct PausedLetter {
    SDL_Texture *tex = nullptr;
    float w = 0.0f;
    float h = 0.0f;
};
static PausedLetter pausedLetters[6]; // "Paused"
static float pausedWaveAmp = 12.0f;
static float pausedWaveFreq = 3.0f;  // Hz
static float pausedWavePhaseStep = 0.6f;
static bool wasEscapeDown = false;
static const SDL_FColor kcolor = {1.0f, 0.0f, 1.0f, 1.0f};
static const SDL_FColor koutline = {0.0f, 0.0f, 0.0f, 1.0f};

SDL_Texture *projectileFrames[30] = {nullptr}; // 1_0.png to 1_29.png
static bool wasMouseDown = false;



struct CachedText {
    SDL_Texture *tex = nullptr;
    float w = 0.0f;
    float h = 0.0f;
    std::string text;
};

static void DestroyCached(CachedText &ct) {
    if (ct.tex) { SDL_DestroyTexture(ct.tex); ct.tex = nullptr; }
    ct.w = ct.h = 0.0f;
    ct.text.clear();
}

static void UpdateCachedText(CachedText &ct, const std::string &s, SDL_Color col) {
    if (s == ct.text && ct.tex) return;
    DestroyCached(ct);
    if (!font) return;
    SDL_Surface *surf = TTF_RenderText_Blended(font, s.c_str(), (int)s.size(), col);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex && SDL_GetTextureSize(tex, &ct.w, &ct.h)) {
        ct.tex = tex;
        ct.text = s;
    } else {
        if (tex) SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(surf);
}


static void RenderLabel(const std::string &text, float x, float y) {
    if (!font || text.empty()) return;
    SDL_Color white{255, 255, 255, 255};
    SDL_Color shadow{0, 0, 0, 160};

    auto makeTex = [&](SDL_Color col) -> SDL_Texture* {
        SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), (int)text.size(), col);
        if (!surf) return nullptr;
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
        return tex;
    };

    SDL_Texture *texShadow = makeTex(shadow);
    SDL_Texture *texMain = makeTex(white);
    float w = 0.0f, h = 0.0f;
    if (texShadow && SDL_GetTextureSize(texShadow, &w, &h)) {
        SDL_FRect dst1{ x + 2.0f, y + 2.0f, w, h };
        SDL_FRect dst2{ x + 4.0f, y + 4.0f, w, h };
        SDL_RenderTexture(renderer, texShadow, nullptr, &dst1);
        SDL_RenderTexture(renderer, texShadow, nullptr, &dst2);
    }
    if (texMain && SDL_GetTextureSize(texMain, &w, &h)) {
        SDL_FRect dst{ x, y, w, h };
        SDL_RenderTexture(renderer, texMain, nullptr, &dst);
    }
    if (texShadow) SDL_DestroyTexture(texShadow);
    if (texMain) SDL_DestroyTexture(texMain);
}

static bool LoadPausedLetters(SDL_Renderer *r, TTF_Font *f) {
    const char *word = "Paused";
    SDL_Color white{255, 255, 255, 255};
    for (int i = 0; i < 6; ++i) {
        char cstr[2] = {word[i], '\0'};
        SDL_Surface *surf = TTF_RenderText_Blended(f, cstr, 1, white);
        if (!surf) return false;
        pausedLetters[i].tex = SDL_CreateTextureFromSurface(r, surf);
        pausedLetters[i].w = (float)surf->w;
        pausedLetters[i].h = (float)surf->h;
        SDL_DestroySurface(surf);
        if (!pausedLetters[i].tex) return false;
    }
    return true;
}



//keylistner
struct InputState {
    const bool* state = nullptr;
    int count = 0;
    void Update() { state = SDL_GetKeyboardState(&count); }
    bool Down(SDL_Scancode sc) const { return state && state[sc]; }
};

struct DemoProjectile {
    SDL_FPoint pos;
    SDL_FPoint vel;
    float angle = 0.0f;
    float timer = 0.0f;
    int frame = 0;
    bool alive = true;
    DemoProjectile(float x, float y, float vx, float vy, float ang) : pos{x,y}, vel{vx,vy}, angle(ang) {}
    void Update(float dt) {
        if (!alive) return;
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        timer += dt;
        const float frameTime = 0.03f;
        while (timer >= frameTime) {
            timer -= frameTime;
            frame = (frame + 1) % 30;
        }
        if (pos.x < -120 || pos.x > kWinW + 120 || pos.y < -120 || pos.y > kWinH + 120) {
            alive = false;
        }
    }
    void Render(SDL_Renderer *r) {
        if (!alive || !projectileFrames[frame]) return;
        float w = 96.0f, h = 96.0f;
        SDL_FRect dst{pos.x - w * 0.5f, pos.y - h * 0.5f, w, h};
        SDL_RenderTextureRotated(r, projectileFrames[frame], nullptr, &dst, angle, nullptr, SDL_FLIP_NONE);
    }
};



static ParallaxLayer farClouds{ nullptr, 30.0f, 100.0f, 0.0f };   // slower, higher
static ParallaxLayer nearClouds{ nullptr, 50.0f, 125.0f, 0.0f };  // faster, lower
static ParallaxLayer farMountains {nullptr, 90.0f, 155.0f, 0.0f};
static ParallaxLayer nearMountains {nullptr, 120.0f, 210.0f, 0.0f};
static ParallaxLayer nearTrees {nullptr, 150.0f, 240.0f, 0.0f}; 


static InputState input;
std::vector<Projectile> projectiles;
static std::vector<Coin> coins;
static float coinSpawnTimer = 0.0f;
static float coinSpawnInterval = 1.5f; // seconds
static std::vector<Bat> bats;
static float batSpawnTimer = 0.0f;
static float batSpawnInterval = 3.5f;
static int warningFrame = 0;
static float warningTimer = 0.0f;
static std::mt19937 rng;
static uint64_t startTicks = 0;
enum class GameState { Menu, Playing, Paused, Options, GameOver };
static GameState gameState = GameState::Menu;
static int bestCoins = 0;
static float bestTime = 0.0f;


static bool newRecord = false;
static bool soundMuted = false;
SpriteAnim runningAnim;
SpriteAnim flyingAnim;
Player player;
static bool showFlyHint = true;
static bool waitingForStart = true;
inline bool IsPlayingActive() { return gameState == GameState::Playing && !playerDead && !player.dead; }


static CachedText hudCoinText;
static CachedText hudTimeText;


static int hudLastCoins = -1;
static int hudLastTime = -1;


static CachedText menuTitleText;
static CachedText menuStartText;
static CachedText menuOptionsText;
static CachedText optionsStateText;
static CachedText overTitleText;
static CachedText overCoinsText;
static CachedText overTimeText;
static CachedText newRecordText;
struct MenuButton {
    std::string label;
    SDL_FRect bounds{0,0,0,0};
    bool hover = false;
    float phase = 0.0f;
};
static MenuButton btnStart{ "START", {0,0,0,0}, false, 0.0f };
static MenuButton btnOptions{ "OPTIONS", {0,0,0,0}, false, 0.4f };
struct Demo {
    SDL_FPoint pos{240.0f, 260.0f};
    SDL_FPoint vel{80.0f, 0.0f};
    float t = 0.0f;
    float angleDeg = 0.0f;
} static demo;
static std::vector<DemoProjectile> demoProjectiles;
static float demoShotTimer = 0.0f;

static void ResetGameState() {
    player.Reset();
    player.SetAutopilot(true);
    wasMouseDown = false;
    projectiles.clear();
    coins.clear();
    bats.clear();
    coinCount = 0;
    gameTimer = 0.0f;
    coinSpawnTimer = 0.0f;
    coinSpawnInterval = 1.5f;
    batSpawnTimer = 0.0f;
    batSpawnInterval = 3.5f;
    playerDead = false;
    gameState = GameState::Playing;
    newRecord = false;
    showFlyHint = true;
    waitingForStart = true;
}


/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("ILUVMUSIC", kWinW, kWinH, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetWindowMouseGrab(window, false);  // start in menu: free cursor
    SDL_ShowCursor();                       // visible in menu/options

    // In SDL_AppInit, after creating renderer:
    SDL_SetRenderVSync(renderer, 1);  // Add this line

    SDL_SetRenderLogicalPresentation(renderer, kWinW, kWinH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    arrowTex = IMG_LoadTexture(renderer, "assets/arrow.png");
    cursorTex = IMG_LoadTexture(renderer, "assets/image.png");
    for (int i = 0; i < 4; ++i) {
        char file[64];
        SDL_snprintf(file, sizeof(file), "assets/coin/spr_coin_strip4_%d.png", i);
        coinFrames[i] = IMG_LoadTexture(renderer, file);
        if (!coinFrames[i]) {
            SDL_Log("Failed to load coin frame %d: %s", i, SDL_GetError());
            return SDL_APP_FAILURE;
        }
    }
    for (int i = 0; i < 9; ++i) {
        char file[128];
        SDL_snprintf(file, sizeof(file), "assets/warning/WarningSign%02d.png", i + 1);
        warningFrames[i] = IMG_LoadTexture(renderer, file);
        if (!warningFrames[i]) {
            SDL_Log("Warning: missing warning frame %d: %s", i, SDL_GetError());
        }
    }
    for (int i = 0; i < 15; ++i) {
        char file[128];
        SDL_snprintf(file, sizeof(file), "assets/Bat/Move/Frames/Bat Move%d.png", i + 1);
        batMoveFrames[i] = IMG_LoadTexture(renderer, file);
        if (!batMoveFrames[i]) {
            SDL_Log("Failed to load bat move frame %d: %s", i, SDL_GetError());
            return SDL_APP_FAILURE;
        }
    }
    for (int i = 0; i < 7; ++i) {
        char file[128];
        SDL_snprintf(file, sizeof(file), "assets/Bat/Death/Frames/Bat Death%d.png", i + 1);
        batDeathFrames[i] = IMG_LoadTexture(renderer, file);
        if (!batDeathFrames[i]) {
            SDL_Log("Failed to load bat death frame %d: %s", i, SDL_GetError());
            return SDL_APP_FAILURE;
        }
    }


        // Load projectile animation frames
    for (int i = 0; i < 30; ++i) {
        char file[64];
        SDL_snprintf(file, sizeof(file), "assets/1/1_%d.png", i);
        projectileFrames[i] = IMG_LoadTexture(renderer, file);
        if (!projectileFrames[i]) {
            SDL_Log("Failed to load projectile frame %d: %s", i, SDL_GetError());
            return SDL_APP_FAILURE;
        }
    }



    if (!TTF_Init()) {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }



    font = TTF_OpenFont("ARCADECLASSIC.ttf", 32);
    if (!font) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!LoadPausedLetters(renderer, font)) {
        SDL_Log("Failed to load paused letters: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Precache static labels
    SDL_Color white{255,255,255,255};
    UpdateCachedText(menuTitleText, "BIRD GAME", white);
    UpdateCachedText(menuStartText, "START", white);
    UpdateCachedText(menuOptionsText, "OPTIONS", white);
    UpdateCachedText(overTitleText, "GAME OVER!", white);
    UpdateCachedText(newRecordText, "NEW RECORD!", white);
    optionsStateText.text.clear(); // will be set on render
    hudLastCoins = -1;
    hudLastTime = -1;




    bgTexture = IMG_LoadTexture(renderer, "sky.png");
    if (!bgTexture) {
        SDL_Log("Failed to load background: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    } 
    
    psTextureL1 = IMG_LoadTexture(renderer, "far-clouds.png");
    if(!psTextureL1) {
        SDL_Log("Failed to load parallax layer 1: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }   

    psTextureL2 = IMG_LoadTexture(renderer, "near-clouds.png");
    if(!psTextureL2) {
        SDL_Log("Failed to load parallax layer 2: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    psTextureL3 = IMG_LoadTexture(renderer, "far-mountains.png");
    if (!psTextureL3) {
        SDL_Log("Failed to load parallax layer 3: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    psTextureL4 = IMG_LoadTexture(renderer, "mountains.png");
    if (!psTextureL4) {
         SDL_Log("Failed to load parallax layer 3: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }      
    
    psTextureL5 = IMG_LoadTexture(renderer, "trees.png");
    if (!psTextureL5) {
         SDL_Log("Failed to load parallax layer 3: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // assign textures and speeds

    farMountains.SetTexture(psTextureL3);
    farMountains.SetSpeed(60.0f); // slowest: farthest layer

    farClouds.SetTexture(psTextureL1);
    farClouds.SetSpeed(10.0f);   // slower: farther layer

    nearClouds.SetTexture(psTextureL2);
    nearClouds.SetSpeed(30.0f);  // faster: nearer 
    
    nearMountains.SetTexture(psTextureL4);
    nearMountains.SetSpeed(90.0f); // fastest: nearest layer

    nearTrees.SetTexture(psTextureL5);
    nearTrees.SetSpeed(120.0f);

    // Load player animations
    runningAnim.Init(0.12f, 4);
    runningAnim.LoadFrames("assets/Running/Running", 4, renderer);

    flyingAnim.Init(0.08f, 5);
    flyingAnim.LoadFrames("assets/Flying/Flying", 5, renderer);

    player.Init();


    SDL_SetRenderLogicalPresentation(renderer, kWinW, kWinH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    projectiles.reserve(256);
    coins.reserve(256);
    bats.reserve(128);
    demoProjectiles.reserve(64);

    rng.seed((uint32_t)SDL_GetTicks());
    startTicks = SDL_GetTicks();

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}





/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{


    SDL_RenderClear(renderer);

    //key
    input.Update();
    bool flyKeyDown = input.Down(SDL_SCANCODE_SPACE) || input.Down(SDL_SCANCODE_W);

    if (bgTexture) {
        SDL_RenderTexture(renderer, bgTexture, NULL, NULL); // fills the current viewport
    }


    
    //physics rendering stuff
    static uint64_t last = 0;
    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t freq = SDL_GetPerformanceFrequency();

    float dt = (last == 0) ? 0.0f : (float)(now - last) / (float)freq;
    if (dt > 0.1f) dt = 0.1f; // clamp huge spikes
    last = now;

    bool escapeDown = input.Down(SDL_SCANCODE_ESCAPE);
    if (escapeDown && !wasEscapeDown) {
        if (playerDead && gameState == GameState::GameOver) {
            ResetGameState();
            SDL_SetWindowMouseGrab(window, true);
            SDL_HideCursor();
            last = SDL_GetPerformanceCounter();
            dt = 0.0f;
        } else if (gameState == GameState::Playing) {
            gameState = GameState::Paused;
            SDL_SetWindowMouseGrab(window, false);
            SDL_ShowCursor();
            last = SDL_GetPerformanceCounter();
            dt = 0.0f;
        } else if (gameState == GameState::Paused) {
            gameState = GameState::Playing;
            SDL_SetWindowMouseGrab(window, true);
            SDL_HideCursor();
            last = SDL_GetPerformanceCounter();
            dt = 0.0f;
        } else if (gameState == GameState::Options) {
            gameState = GameState::Menu;
            SDL_ShowCursor();
            SDL_SetWindowMouseGrab(window, false);
            last = SDL_GetPerformanceCounter();
            dt = 0.0f;
        }
    }
    wasEscapeDown = escapeDown;

    if (playerDead || player.dead || gameState == GameState::Paused || gameState == GameState::Options) {
        dt = 0.0f;
    }

    if (IsPlayingActive() && !waitingForStart) {
        gameTimer += dt;
        warningTimer += dt;
        const float warnFrameTime = 0.08f;
        while (warningTimer >= warnFrameTime) {
            warningTimer -= warnFrameTime;
            warningFrame = (warningFrame + 1) % 9;
        }
    }

    // Demo motion in menu (simple sine path with anim swap on vertical movement)
    if (gameState == GameState::Menu) {
        float prevX = demo.pos.x;
        float prevY = demo.pos.y;
        demo.t += dt;
        demo.pos.x = kWinW * 0.5f + SDL_sinf(demo.t * 0.8f) * 70.0f;
        demo.pos.y = 260.0f + SDL_sinf(demo.t * 1.5f) * 28.0f;
        demo.vel.x = (demo.pos.x - prevX) / (dt > 0 ? dt : 1.0f);
        demo.vel.y = (demo.pos.y - prevY) / (dt > 0 ? dt : 1.0f);
        demo.angleDeg = SDL_atan2(demo.vel.y, demo.vel.x) * (180.0f / SDL_PI_D);

        bool flyingDemo = SDL_fabsf(demo.vel.y) > 5.0f;
        if (flyingDemo) {
            flyingAnim.Update(dt);
            runningAnim.Reset();
        } else {
            runningAnim.Update(dt);
            flyingAnim.Reset();
        }

        demoShotTimer += dt;
        if (demoShotTimer >= 0.9f) {
            demoShotTimer = 0.0f;
            float speed = 260.0f;
            float rad = demo.angleDeg * SDL_PI_F / 180.0f;
            float vx = SDL_cosf(rad) * speed;
            float vy = SDL_sinf(rad) * speed;
            demoProjectiles.emplace_back(demo.pos.x, demo.pos.y, vx, vy, demo.angleDeg);
        }
        for (auto &dp : demoProjectiles) dp.Update(dt);
        demoProjectiles.erase(std::remove_if(demoProjectiles.begin(), demoProjectiles.end(),
                            [](const DemoProjectile &p){ return !p.alive; }),
                            demoProjectiles.end());
    }

    farClouds.Update(dt);
    farClouds.Render(renderer, (float)kWinW);

    nearClouds.Update(dt);
    nearClouds.Render(renderer, (float)kWinW);

    farMountains.Update(dt);
    farMountains.Render(renderer, (float)kWinW);

    nearMountains.Update(dt);
    nearMountains.Render(renderer, (float)kWinW);

    nearTrees.Update(dt);
    nearTrees.Render(renderer, (float)kWinW);




    if (IsPlayingActive()) {
        if (waitingForStart && flyKeyDown) {
            waitingForStart = false;
            showFlyHint = false;
            player.SetAutopilot(false);
        }
        if (showFlyHint && flyKeyDown) {
            showFlyHint = false;
            player.SetAutopilot(false);
        }
        player.Update(dt);
    } else if (gameState == GameState::Playing) {
        // keep auto hovering before start
        player.Update(dt);
    }

    // Spawn coins off-screen and move left
    if (IsPlayingActive() && !waitingForStart) {
        coinSpawnTimer += dt;
        if (coinSpawnTimer >= coinSpawnInterval) {
            coinSpawnTimer = 0.0f;
            std::uniform_real_distribution<float> ydist(30.0f, kWinH - 30.0f);
            std::uniform_real_distribution<float> speedDist(150.0f, 240.0f);
            std::uniform_real_distribution<float> intervalDist(1.0f, 2.2f);
            std::uniform_real_distribution<float> ampDist(4.0f, 12.0f);
            std::uniform_real_distribution<float> freqDist(1.5f, 3.8f);
            std::uniform_real_distribution<float> phaseDist(0.0f, SDL_PI_F * 2.0f);
            std::uniform_int_distribution<int> patternDist(0, 2); // 0 single,1 row,2 stack/column
            std::uniform_int_distribution<int> countDist(1, 3);

            float spawnX = kWinW + 50.0f;
            float baseY = ydist(rng);
            int pattern = patternDist(rng);
            int count = countDist(rng);
            float gap = 36.0f;

            auto makeCoin = [&](float x, float y) {
                float speed = speedDist(rng);
                float amp = ampDist(rng);
                float freq = freqDist(rng);
                float phase = phaseDist(rng);
                coins.emplace_back(x, y, speed, amp, freq, phase);
            };

            if (pattern == 0) {
                makeCoin(spawnX, baseY);
                if (rng() % 3 == 0) makeCoin(spawnX + gap, baseY - gap * 0.3f); // occasional buddy
            } else if (pattern == 1) {
                for (int i = 0; i < count; ++i) {
                    makeCoin(spawnX + i * gap, baseY + SDL_sinf(0.6f * i) * 10.0f);
                }
            } else {
                for (int i = 0; i < count; ++i) {
                    makeCoin(spawnX, baseY + (i - 1) * gap + SDL_sinf(0.4f * i) * 8.0f);
                }
            }

            coinSpawnInterval = intervalDist(rng);
        }
    }

    if (IsPlayingActive() && !waitingForStart) {
        for (auto &c : coins) {
            c.Update(dt);
        }
        coins.erase(std::remove_if(coins.begin(), coins.end(), [](const Coin &c){ return !c.alive; }), coins.end());
    }

    // Spawn bats from all edges
    if (IsPlayingActive() && !waitingForStart) {
        batSpawnTimer += dt;
        if (batSpawnTimer >= batSpawnInterval) {
            batSpawnTimer = 0.0f;
            std::uniform_real_distribution<float> xdist(-60.0f, kWinW + 60.0f);
            std::uniform_real_distribution<float> ydist(-60.0f, kWinH + 60.0f);
            std::uniform_real_distribution<float> speedDist(220.0f, 300.0f);
            std::uniform_real_distribution<float> intervalDist(2.2f, 3.8f);
            std::uniform_int_distribution<int> sideDist(0, 3); // 0 right,1 left,2 top,3 bottom

            float spawnX = kWinW + 80.0f;
            float spawnY = kWinH * 0.5f;
            int side = sideDist(rng);
            if (side == 0) { // right
                spawnX = kWinW + 80.0f;
                spawnY = ydist(rng);
            } else if (side == 1) { // left
                spawnX = -80.0f;
                spawnY = ydist(rng);
            } else if (side == 2) { // top
                spawnX = xdist(rng);
                spawnY = -80.0f;
            } else { // bottom
                spawnX = xdist(rng);
                spawnY = kWinH + 80.0f;
            }

            float spd = speedDist(rng);
            bats.emplace_back(spawnX, spawnY, spd);
            batSpawnInterval = intervalDist(rng);
        }
    }

    std::vector<SDL_FRect> warningIcons;

    if (IsPlayingActive() && !waitingForStart) {
        SDL_FPoint target = player.Center();
        for (auto &b : bats) {
            b.Update(dt, target);
        }
        bats.erase(std::remove_if(bats.begin(), bats.end(), [](const Bat &b){ return !b.alive; }), bats.end());

        // collect warnings for approaching off-screen bats near player's edge
        float pcx = target.x;
        float pcy = target.y;
        float warnMargin = 18.0f;
        for (auto &b : bats) {
            if (!b.alive || b.state != BatState::Moving) continue;
            bool offLeft = b.pos.x < -40.0f;
            bool offRight = b.pos.x > kWinW + 40.0f;
            bool offTop = b.pos.y < -40.0f;
            bool offBot = b.pos.y > kWinH + 40.0f;
            if (!(offLeft || offRight || offTop || offBot)) continue;

            bool headingIn = (offLeft && b.vel.x > 0) || (offRight && b.vel.x < 0) ||
                             (offTop && b.vel.y > 0) || (offBot && b.vel.y < 0);
            if (!headingIn) continue;

            bool playerNearEdge = (offLeft && pcx < 140.0f) ||
                                  (offRight && pcx > kWinW - 140.0f) ||
                                  (offTop && pcy < 140.0f) ||
                                  (offBot && pcy > kWinH - 140.0f);
            if (!playerNearEdge) continue;

            float w = 40.0f, h = 40.0f;
            SDL_FRect icon{0,0,w,h};
            if (offLeft) {
                icon.x = warnMargin;
                icon.y = ClampF(b.pos.y, warnMargin, (float)kWinH - warnMargin - h);
            } else if (offRight) {
                icon.x = kWinW - warnMargin - w;
                icon.y = ClampF(b.pos.y, warnMargin, (float)kWinH - warnMargin - h);
            } else if (offTop) {
                icon.y = warnMargin;
                icon.x = ClampF(b.pos.x, warnMargin, (float)kWinW - warnMargin - w);
            } else { // bottom
                icon.y = kWinH - warnMargin - h;
                icon.x = ClampF(b.pos.x, warnMargin, (float)kWinW - warnMargin - w);
            }
            warningIcons.push_back(icon);
        }
    }

        // Mouse click to shoot projectile
    float mx = 0.0f, my = 0.0f;
    Uint32 mouseState = 0;
    GetLogicalMouse(mx, my, mouseState);
    bool mouseDown = (mouseState & SDL_BUTTON_LMASK) != 0;

    if (gameState == GameState::Menu && mouseDown && !wasMouseDown) {
        if (btnStart.hover) {
            ResetGameState();
            SDL_SetWindowMouseGrab(window, true);
            SDL_HideCursor();
        } else if (btnOptions.hover) {
            gameState = GameState::Options;
            SDL_ShowCursor();
            SDL_SetWindowMouseGrab(window, false);
        }
    }

    if (gameState == GameState::Options && mouseDown && !wasMouseDown) {
        // toggle mute when clicking anywhere on the options label area
        int w=0,h=0;
        const char *label = soundMuted ? "Sound: Muted" : "Sound: On";
        SDL_Surface *s = TTF_RenderText_Blended(font, label, SDL_strlen(label), SDL_Color{255,255,255,255});
        if (s) { w = s->w; h = s->h; SDL_DestroySurface(s); }
        float x = kWinW * 0.5f - w * 0.5f;
        float y = kWinH * 0.5f;
        if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
            soundMuted = !soundMuted;
            DestroyCached(optionsStateText);
        }
    }

    wasMouseDown = mouseDown;

    // Update projectiles
    if (IsPlayingActive() && !waitingForStart) {
        for (auto &p : projectiles) {
            p.Update(dt);
        }

        // Remove dead projectiles
        projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(), 
                [](const Projectile &p) { return !p.alive; }),
            projectiles.end()
        );

        // Coin collection check
        SDL_FRect playerBox = player.Bounds();
        const float coinSize = 32.0f;
        for (auto &c : coins) {
            if (!c.alive) continue;
            SDL_FRect coinBox{ c.pos.x - coinSize * 0.5f, c.pos.y - coinSize * 0.5f, coinSize, coinSize };
            if (AABBOverlap(playerBox, coinBox)) {
                c.alive = false;
                coinCount++;
            }
        }

        for (auto &p : projectiles) {
            if (!p.alive) continue;
            SDL_FRect projBox = p.Bounds();
            for (auto &b : bats) {
                if (!b.alive) continue;
                SDL_FRect batBox = b.Bounds();
                if (AABBOverlap(projBox, batBox)) {
                    b.Kill();
                    p.alive = false;
                    break; // projectile consumed
                }
            }
        }

        // Bat vs player
        for (auto &b : bats) {
            if (!b.alive || b.state != BatState::Moving) continue;
            if (AABBOverlap(playerBox, b.Bounds())) {
                player.dead = true;
                playerDead = true;
                gameState = GameState::GameOver;
                // update records
                if (coinCount > bestCoins || (coinCount == bestCoins && gameTimer > bestTime)) {
                    bestCoins = coinCount;
                    bestTime = gameTimer;
                    newRecord = true;
                } else {
                    newRecord = false;
                }
                SDL_SetWindowMouseGrab(window, false);
                SDL_ShowCursor();
                break;
            }
        }
    }





    // then draw
    if (gameState != GameState::Menu) {
        player.Render(renderer);

        for (auto &b : bats) {
            b.Render(renderer);
        }

        for (auto &p : projectiles) {
            p.Render(renderer);
        }

        for (auto &c : coins) {
            c.Render(renderer);
        }
    }

    // Warning icons for incoming bats off-screen
    if (warningFrames[warningFrame]) {
        for (const auto &wicon : warningIcons) {
            SDL_RenderTexture(renderer, warningFrames[warningFrame], nullptr, &wicon);
        }
    }

    // Aim arrow following mouse (larger and closer) only during play
    if (arrowTex && gameState == GameState::Playing) {
        static float aimX = 0.0f, aimY = 0.0f, aimAngleDeg = 0.0f;
        float mx = 0.0f, my = 0.0f;
        GetLogicalMouse(mx, my); // logical coords

        SDL_FPoint center = player.Center();
        float pcx = center.x;
        float pcy = center.y;

        float dx = mx - pcx;
        float dy = my - pcy;
        float len = SDL_sqrtf(dx*dx + dy*dy);
        if (len < 0.001f) len = 0.001f;
        dx /= len; dy /= len;

        const float radius = 25.0f;  // closer to the bird
        aimX = pcx + dx * radius;
        aimY = pcy + dy * radius;
        aimAngleDeg = SDL_atan2(dy, dx) * (180.0 / SDL_PI_D);

        float aw = 32.0f, ah = 32.0f;  // larger arrow
        SDL_FRect adst{ aimX - aw * 0.5f, aimY - ah * 0.5f, aw, ah };
        SDL_RenderTextureRotated(renderer, arrowTex, nullptr, &adst, aimAngleDeg, nullptr, SDL_FLIP_NONE);
    }

    // Custom cursor rendered at the actual mouse position while hidden system cursor is off
    if (gameState == GameState::Playing && cursorTex) {
        float cx = 0.0f, cy = 0.0f;
        GetLogicalMouse(cx, cy);
        if (cx < 0.0f) cx = 0.0f;
        if (cy < 0.0f) cy = 0.0f;
        if (cx > (float)kWinW) cx = (float)kWinW;
        if (cy > (float)kWinH) cy = (float)kWinH;

        float cw = 20.0f, ch = 20.0f;
        SDL_FRect cdst{ cx - cw * 0.5f, cy - ch * 0.5f, cw, ch };
        SDL_RenderTexture(renderer, cursorTex, nullptr, &cdst);
    }

    // HUD (only during gameplay)
    if (gameState == GameState::Playing) {
        int totalSeconds = (int)gameTimer;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        char timeBuf[8];
        SDL_snprintf(timeBuf, sizeof(timeBuf), "%02d %02d", minutes, seconds);
        std::string coinLabel = std::to_string(coinCount);
        std::string timeLabel = std::string("Time ") + timeBuf;
        SDL_Color white{255,255,255,255};
        if (coinCount != hudLastCoins) {
            hudLastCoins = coinCount;
            UpdateCachedText(hudCoinText, coinLabel, white);
        }
        if (totalSeconds != hudLastTime) {
            hudLastTime = totalSeconds;
            UpdateCachedText(hudTimeText, timeLabel, white);
        }
        float margin = 16.0f;
        double hudT = (double)SDL_GetTicks() / 1000.0;
        float bob = SDL_sinf((float)hudT * 3.0f) * 2.5f;
        if (hudCoinText.tex && coinFrames[0]) {
            float coinX = (float)kWinW - hudCoinText.w - margin;
            SDL_FRect shadow{ coinX + 2.0f, 10.0f + bob + 2.0f, hudCoinText.w, hudCoinText.h };
            SDL_SetTextureColorMod(hudCoinText.tex, 0,0,0);
            SDL_SetTextureAlphaMod(hudCoinText.tex, 160);
            SDL_RenderTexture(renderer, hudCoinText.tex, nullptr, &shadow);
            SDL_SetTextureColorMod(hudCoinText.tex, 255,255,255);
            SDL_SetTextureAlphaMod(hudCoinText.tex, 255);
            SDL_FRect dst{ coinX, 10.0f + bob, hudCoinText.w, hudCoinText.h };
            SDL_RenderTexture(renderer, hudCoinText.tex, nullptr, &dst);

            // animated coin icon
            static int coinFrame = 0;
            static float coinTimer = 0.0f;
            coinTimer += dt;
            if (coinTimer >= 0.1f) {
                coinTimer = 0.0f;
                coinFrame = (coinFrame + 1) % 4;
            }
            SDL_Texture *cTex = coinFrames[coinFrame];
            if (cTex) {
                float sz = 28.0f;
                SDL_FRect cdst{ coinX - sz - 4.0f, 10.0f + bob, sz, sz };
                SDL_RenderTexture(renderer, cTex, nullptr, &cdst);
            }
        }
        if (hudTimeText.tex) {
            float timeX = (float)kWinW - hudTimeText.w - margin;
            SDL_FRect shadow{ timeX + 2.0f, 32.0f + bob + 2.0f, hudTimeText.w, hudTimeText.h };
            SDL_SetTextureColorMod(hudTimeText.tex, 0,0,0);
            SDL_SetTextureAlphaMod(hudTimeText.tex, 160);
            SDL_RenderTexture(renderer, hudTimeText.tex, nullptr, &shadow);
            SDL_SetTextureColorMod(hudTimeText.tex, 255,255,255);
            SDL_SetTextureAlphaMod(hudTimeText.tex, 255);
            SDL_FRect dst{ timeX, 32.0f + bob, hudTimeText.w, hudTimeText.h };
            SDL_RenderTexture(renderer, hudTimeText.tex, nullptr, &dst);
        }

        if (showFlyHint && font) {
            const char *hint = "Press W or SPACE to fly";
            const float baseY = kWinH * 0.7f;
            const float amp = 8.0f;
            const float freq = 2.2f;
            double tNow = (double)SDL_GetTicks() / 1000.0;

            SDL_Surface *s = TTF_RenderText_Blended(font, hint, (int)SDL_strlen(hint), SDL_Color{255,255,255,255});
            if (s) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, s);
                if (tex) {
                    float yOffset = SDL_sinf((float)(tNow * freq)) * amp;
                    float x = kWinW * 0.5f - s->w * 0.5f - 12.0f; // slight nudge left
                    float y = baseY + yOffset;
                    SDL_FRect shadow{ x + 3.0f, y + 3.0f, (float)s->w, (float)s->h };
                    SDL_SetTextureColorMod(tex, 0,0,0);
                    SDL_SetTextureAlphaMod(tex, 180);
                    SDL_RenderTexture(renderer, tex, nullptr, &shadow);
                    SDL_SetTextureColorMod(tex, 255,255,255);
                    SDL_SetTextureAlphaMod(tex, 255);
                    SDL_FRect dst{ x, y, (float)s->w, (float)s->h };
                    SDL_RenderTexture(renderer, tex, nullptr, &dst);
                    SDL_DestroyTexture(tex);
                }
                SDL_DestroySurface(s);
            }
        }
    }

    // Paused wave text overlay
    if (gameState == GameState::Paused) {
        double t = (double)SDL_GetTicks() / 1000.0;
        float totalW = 0.0f;
        float maxH = 0.0f;
        const float spacing = 4.0f;
        for (int i = 0; i < 6; ++i) {
            totalW += pausedLetters[i].w;
            if (i < 5) totalW += spacing;
            if (pausedLetters[i].h > maxH) maxH = pausedLetters[i].h;
        }
        float startX = (kWinW - totalW) * 0.5f;
        float baseY = (kWinH - maxH) * 0.5f;
        float x = startX;
        for (int i = 0; i < 6; ++i) {
            float yOffset = SDL_sinf((float)(t * pausedWaveFreq + pausedWavePhaseStep * i)) * pausedWaveAmp;
            SDL_FRect dst{ x, baseY + yOffset, pausedLetters[i].w, pausedLetters[i].h };
            if (pausedLetters[i].tex) {
                SDL_SetTextureColorMod(pausedLetters[i].tex, 0, 0, 0);
                SDL_SetTextureAlphaMod(pausedLetters[i].tex, 200);
                SDL_FRect shadowDst{ dst.x + 3.0f, dst.y + 3.0f, dst.w, dst.h };
                SDL_RenderTexture(renderer, pausedLetters[i].tex, nullptr, &shadowDst);
                SDL_FRect shadowDst2{ dst.x + 5.0f, dst.y + 5.0f, dst.w, dst.h };
                SDL_RenderTexture(renderer, pausedLetters[i].tex, nullptr, &shadowDst2);
                SDL_SetTextureColorMod(pausedLetters[i].tex, 255, 255, 255);
                SDL_SetTextureAlphaMod(pausedLetters[i].tex, 255);
                SDL_RenderTexture(renderer, pausedLetters[i].tex, nullptr, &dst);
            }
            x += pausedLetters[i].w + spacing;
        }
    }

    // Menu
    if (gameState == GameState::Menu) {
        double t = (double)SDL_GetTicks() / 1000.0;
        float titleWave = SDL_sinf((float)t * 2.2f) * 4.0f;
        if (menuTitleText.tex) {
            SDL_FRect dst{ kWinW * 0.5f - menuTitleText.w * 0.5f, 46.0f + titleWave, menuTitleText.w, menuTitleText.h };
            SDL_RenderTexture(renderer, menuTitleText.tex, nullptr, &dst);
        }

        // buttons with sine wobble and hover blink
        btnStart.bounds.w = menuStartText.w;
        btnStart.bounds.h = menuStartText.h;
        btnStart.bounds.x = kWinW * 0.5f - btnStart.bounds.w * 0.5f;
        btnStart.bounds.y = 120.0f;

        btnOptions.bounds.w = menuOptionsText.w;
        btnOptions.bounds.h = menuOptionsText.h;
        btnOptions.bounds.x = kWinW * 0.5f - btnOptions.bounds.w * 0.5f;
        btnOptions.bounds.y = 160.0f;

        Uint32 mstate = SDL_GetMouseState(nullptr, nullptr);
        float mx=0.0f, my=0.0f; GetLogicalMouse(mx,my);
        auto hoverCheck = [&](MenuButton &b){
            b.hover = (mx >= b.bounds.x && mx <= b.bounds.x + b.bounds.w &&
                       my >= b.bounds.y && my <= b.bounds.y + b.bounds.h);
        };
        hoverCheck(btnStart);
        hoverCheck(btnOptions);

        auto renderButton = [&](const MenuButton &b, float phase){
            float hoverWave = b.hover ? SDL_sinf((float)t * 4.0f + phase) * 4.0f : 0.0f;
            Uint8 alpha = b.hover ? (Uint8)(200 + 55 * (0.5f + 0.5f * SDL_sinf((float)t * 6.0f + phase))) : 255;
            CachedText *ct = (b.label == "START") ? &menuStartText : &menuOptionsText;
            if (ct->tex) {
                SDL_FRect dst{ b.bounds.x, b.bounds.y + hoverWave, ct->w, ct->h };
                SDL_FRect s1{ dst.x + 2.0f, dst.y + 2.0f, dst.w, dst.h };
                SDL_FRect s2{ dst.x + 4.0f, dst.y + 4.0f, dst.w, dst.h };
                SDL_SetTextureColorMod(ct->tex, 0,0,0);
                SDL_SetTextureAlphaMod(ct->tex, 140);
                SDL_RenderTexture(renderer, ct->tex, nullptr, &s1);
                SDL_RenderTexture(renderer, ct->tex, nullptr, &s2);
                SDL_SetTextureColorMod(ct->tex, 255,255,255);
                SDL_SetTextureAlphaMod(ct->tex, alpha);
                SDL_RenderTexture(renderer, ct->tex, nullptr, &dst);
            }
        };
        renderButton(btnStart, 0.0f);
        renderButton(btnOptions, 1.0f);

        // demo player + arrow + demo shots
        SDL_FRect demoRect{ demo.pos.x - 20.0f, demo.pos.y - 20.0f, 40.0f, 40.0f };
        SDL_Texture *demoTex = runningAnim.Current();
        if (SDL_fabsf(demo.vel.y) > 5.0f && flyingAnim.Current()) {
            demoTex = flyingAnim.Current();
        }
        if (demoTex) {
            SDL_RenderTextureRotated(renderer, demoTex, nullptr, &demoRect, 0.0, nullptr, SDL_FLIP_NONE);
        }
        for (auto &dp : demoProjectiles) {
            dp.Render(renderer);
        }
        if (arrowTex) {
            float radius = 30.0f;
            float ax = demo.pos.x + SDL_cosf(demo.angleDeg * SDL_PI_F/180.0f) * radius;
            float ay = demo.pos.y + SDL_sinf(demo.angleDeg * SDL_PI_F/180.0f) * radius;
            SDL_FRect adst{ ax - 16.0f, ay - 16.0f, 32.0f, 32.0f };
            SDL_RenderTextureRotated(renderer, arrowTex, nullptr, &adst, demo.angleDeg, nullptr, SDL_FLIP_NONE);
        }
    }

    // Options placeholder
    if (gameState == GameState::Options) {
        SDL_Color white{255,255,255,255};
        UpdateCachedText(optionsStateText, soundMuted ? "Sound: Muted" : "Sound: On", white);
        RenderLabel("OPTIONS", kWinW * 0.5f - 70.0f, kWinH * 0.5f - 40.0f);
        if (optionsStateText.tex) {
            SDL_FRect dst{ kWinW * 0.5f - optionsStateText.w * 0.5f, kWinH * 0.5f, optionsStateText.w, optionsStateText.h };
            SDL_RenderTexture(renderer, optionsStateText.tex, nullptr, &dst);
        }
        RenderLabel("ESC to return", kWinW * 0.5f - 90.0f, kWinH * 0.5f + 30.0f);
    }

    // Game over overlay
    if (gameState == GameState::GameOver) {
        if (overTitleText.tex) {
            SDL_FRect dst{ kWinW * 0.5f - overTitleText.w * 0.5f, kWinH * 0.5f - 40.0f, overTitleText.w, overTitleText.h };
            SDL_RenderTexture(renderer, overTitleText.tex, nullptr, &dst);
        }
        int totalSeconds = (int)gameTimer;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        char timeBuf[8];
        SDL_snprintf(timeBuf, sizeof(timeBuf), "%02d %02d", minutes, seconds);
        SDL_Color white{255,255,255,255};
        UpdateCachedText(overCoinsText, "Coins " + std::to_string(coinCount), white);
        UpdateCachedText(overTimeText, std::string("Time ") + timeBuf, white);

        double t = (double)SDL_GetTicks() / 1000.0;
        float yOffset = SDL_sinf((float)(t * 3.0f)) * 4.0f;
        float margin = 14.0f;
        if (overCoinsText.tex) {
            SDL_FRect dst{ kWinW - overCoinsText.w - margin, 14.0f + yOffset, overCoinsText.w, overCoinsText.h };
            SDL_RenderTexture(renderer, overCoinsText.tex, nullptr, &dst);
        }
        if (overTimeText.tex) {
            SDL_FRect dst{ kWinW - overTimeText.w - margin, 36.0f + yOffset, overTimeText.w, overTimeText.h };
            SDL_RenderTexture(renderer, overTimeText.tex, nullptr, &dst);
        }

        if (newRecord && newRecordText.tex) {
            float recY = kWinH * 0.5f + 10.0f + SDL_sinf((float)(t * 3.5f)) * 8.0f;
            Uint8 r = (Uint8)(128 + 127 * SDL_sinf((float)t * 2.0f));
            Uint8 g = (Uint8)(128 + 127 * SDL_sinf((float)t * 2.4f + 2.0f));
            Uint8 b = (Uint8)(128 + 127 * SDL_sinf((float)t * 2.8f + 4.0f));
            SDL_SetTextureColorMod(newRecordText.tex, r, g, b);
            SDL_FRect dst{ kWinW * 0.5f - newRecordText.w * 0.5f + 20.0f, recY, newRecordText.w, newRecordText.h };
            SDL_RenderTexture(renderer, newRecordText.tex, nullptr, &dst);
        }

        RenderLabel("ESC TO RESTART", kWinW * 0.5f - 120.0f, kWinH * 0.5f + 100.0f);
    }





    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */

    if (bgTexture) {
        SDL_DestroyTexture(bgTexture);
        bgTexture = NULL;
    }
    if (psTextureL1) { SDL_DestroyTexture(psTextureL1); psTextureL1 = NULL; }
    if (psTextureL2) { SDL_DestroyTexture(psTextureL2); psTextureL2 = NULL; }
    if (psTextureL3) { SDL_DestroyTexture(psTextureL3); psTextureL3 = NULL; }
    if (psTextureL4) { SDL_DestroyTexture(psTextureL4); psTextureL4 = NULL; }
    if (psTextureL5) { SDL_DestroyTexture(psTextureL5); psTextureL5 = NULL; }

    runningAnim.DestroyFrames();
    flyingAnim.DestroyFrames();


    if (textTex) { SDL_DestroyTexture(textTex); textTex = nullptr; }
    DestroyCached(hudCoinText);
    DestroyCached(hudTimeText);
    DestroyCached(menuTitleText);
    DestroyCached(menuStartText);
    DestroyCached(menuOptionsText);
    DestroyCached(optionsStateText);
    DestroyCached(overTitleText);
    DestroyCached(overCoinsText);
    DestroyCached(overTimeText);
    if (arrowTex) { SDL_DestroyTexture(arrowTex); arrowTex = nullptr; }
    if (cursorTex) { SDL_DestroyTexture(cursorTex); cursorTex = nullptr; }
    for (auto &pl : pausedLetters) {
        if (pl.tex) { SDL_DestroyTexture(pl.tex); pl.tex = nullptr; }
    }
    for (int i = 0; i < 4; ++i) {
        if (coinFrames[i]) { SDL_DestroyTexture(coinFrames[i]); coinFrames[i] = nullptr; }
    }
    for (int i = 0; i < 15; ++i) {
        if (batMoveFrames[i]) { SDL_DestroyTexture(batMoveFrames[i]); batMoveFrames[i] = nullptr; }
    }
    for (int i = 0; i < 7; ++i) {
        if (batDeathFrames[i]) { SDL_DestroyTexture(batDeathFrames[i]); batDeathFrames[i] = nullptr; }
    }
    for (int i = 0; i < 9; ++i) {
        if (warningFrames[i]) { SDL_DestroyTexture(warningFrames[i]); warningFrames[i] = nullptr; }
    }
    DestroyCached(hudCoinText);
    DestroyCached(hudTimeText);
    DestroyCached(menuTitleText);
    DestroyCached(menuStartText);
    DestroyCached(menuOptionsText);
    DestroyCached(optionsStateText);
    DestroyCached(overTitleText);
    DestroyCached(overCoinsText);
    DestroyCached(overTimeText);
    DestroyCached(newRecordText);

    if (font) { TTF_CloseFont(font); font = nullptr; }
    for (int i = 0; i < 30; ++i) {
    if (projectileFrames[i]) {
        SDL_DestroyTexture(projectileFrames[i]);
        projectileFrames[i] = nullptr;
    }
    }
    TTF_Quit();
    SDL_Quit();
}
