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
#include <cmath>
#include <array>
#include <deque>
#include <fstream>
#include <enet/enet.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "core/parallax.h"
#include "core/utils.h"
#include "core/projectile.h"
#include "player/bat.h"
#include "player/player.h"
#include "collectables/coin.h"
#include "core/sprite_anim.h"
#include "core/floating_text.h"
#include "audio/audio.h"
#include "records/record.h"




static SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = nullptr;
static SDL_Texture *textTex = nullptr;
static SDL_Texture *bgTexture =  NULL;
static SDL_Texture *psTextureL1 = NULL;
static SDL_Texture *psTextureL2 = NULL;
static SDL_Texture *psTextureL3 = NULL;
static SDL_Texture *psTextureL4 = NULL;
static SDL_Texture *psTextureL5 = NULL;
static SDL_Texture *arrowTex = nullptr;
static SDL_Texture *cursorTex = nullptr;
static SDL_Texture *magnetTex = nullptr;
static SDL_Texture *shieldTex = nullptr;

static std::vector<FloatingText> floatingTexts;
SDL_Texture *coinFrames[4] = {nullptr};
SDL_Texture *batMoveFrames[16] = {nullptr};
SDL_Texture *batDeathFrames[8] = {nullptr};
static SDL_Texture *warningFrames[9] = {nullptr};
static SDL_Texture *shieldFrames[32] = {nullptr};
static const int kShieldFrameCount = 32;
static float shieldAnimTimer = 0.0f;
static int shieldAnimFrame = 0;
static const float kShieldFrameTime = 0.04f;
static int coinCount = 0;
static float gameTimer = 0.0f; // seconds
bool playerDead = false;
static int score = 0;
static int bestScore = 0;
static int comboCount = 0;
static float comboTimer = 0.0f;
static const float comboTimeout = 3.0f;
static bool magnetActive = false;
static float magnetTimer = 0.0f;
static bool shieldActive = false;
static float shieldTimer = 0.0f;
bool spreadActive = false;
static float spreadTimer = 0.0f;
static bool isFullscreen = false;
static bool wasF12Down = false;
bool perfectShot = false;


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

float gCoinSpinMultiplier = 1.0f;
static float coinSpinBoostTimer = 0.0f;
static const float kCoinSpinBoostDuration = 2.0f;
static float hudCoinAnimTimer = 0.0f;
static int hudCoinFrame = 0;



struct CachedText {
    SDL_Texture *tex = nullptr;
    float w = 0.0f;
    float h = 0.0f;
    std::string text;
    SDL_Color color{255,255,255,255};
};

static CachedText hudCoinText;
static CachedText hudTimeText;
static CachedText hudComboText;
static CachedText hudBestScoreText;
static CachedText hudScoreText;
static CachedText menuTitleText;
static CachedText menuStartText;
static CachedText menuOptionsText;
static CachedText menuMultiplayerText;
static CachedText menuQuitText;
static CachedText optionsStateText;
static CachedText overTitleText;
static CachedText overCoinsText;
static CachedText overScoreText;
static CachedText overTimeText;
static CachedText overBestScoreText;
static CachedText overRestartText;
static CachedText newRecordText;
static CachedText perfectShotText;
static CachedText nicePopupText;
static CachedText hudMenuText;

static int frameCounter = 0;

struct ComboPopup {
    SDL_FPoint pos;
    float timer = 0.0f;
    float lifetime = 2.0f;
};
static std::vector<ComboPopup> comboPopups;

struct BeatScorePopup {
    SDL_FPoint pos;
    float timer = 0.0f;
    float lifetime = 2.0f;
};
static std::vector<BeatScorePopup> beatScorePopups;
static bool beatScoreAnnounced = false;
static GameRecord record;

static constexpr int kMpMaxPlayers = 10;
static constexpr int kMpMaxBats = 96;
static constexpr int kMpMaxProjectiles = 192;
static constexpr float kMpMatchDuration = 300.0f;
static constexpr int kMpPort = 22100;
static constexpr int kMpNameLen = 16;
static constexpr Uint8 kMpBtnLeft = 1 << 0;
static constexpr Uint8 kMpBtnRight = 1 << 1;
static constexpr Uint8 kMpBtnJump = 1 << 2;
static constexpr Uint8 kMpBtnDown = 1 << 3;

enum class NetMsg : Uint8 {
    JoinRequest = 1,
    JoinAccept = 2,
    Input = 3,
    Snapshot = 4,
    KillNotice = 5,
    MatchOver = 6
};

#pragma pack(push, 1)
struct JoinRequestPacket {
    Uint8 type;
    char name[kMpNameLen];
};

struct JoinAcceptHeader {
    Uint8 type;
    Uint8 assignedId;
    Uint8 count;
};

struct JoinAcceptEntry {
    Uint8 id;
    char name[kMpNameLen];
};

struct InputPacket {
    Uint8 type;
    Uint8 id;
    Uint8 buttons;
    float aimX;
    float aimY;
    Uint8 shoot;
};

struct SnapshotHeader {
    Uint8 type;
    float matchTimer;
    Uint8 playerCount;
    Uint8 batCount;
    Uint16 projCount;
};

struct KillNoticePacket {
    Uint8 type;
    char name[kMpNameLen];
};

struct MatchOverPacket {
    Uint8 type;
};
#pragma pack(pop)

struct MpInputState {
    Uint8 buttons = 0;
    float aimX = 0.0f;
    float aimY = 0.0f;
    Uint8 shoot = 0;
};

struct MpPlayer {
    int id = -1;
    std::string name;
    SDL_FRect rect{0,0,40,40};
    SDL_FRect targetRect{0,0,40,40};
    SDL_FPoint vel{0.0f, 0.0f};
    SDL_FPoint targetVel{0.0f, 0.0f};
    bool alive = false;
    bool facingRight = true;
    bool wasJumpDown = false;
    int health = 3;
    int kills = 0;
    int deaths = 0;
    int score = 0;
    bool invincible = false;
    float invincibleTimer = 0.0f;
    float respawnTimer = 0.0f;
    bool connected = true;
    bool hasTarget = false;
};

struct MpProjectile {
    SDL_FPoint pos;
    SDL_FPoint vel;
    float angle = 0.0f;
    float timer = 0.0f;
    int frame = 0;
    bool alive = true;
    int ownerId = -1;

    MpProjectile(float x, float y, float vx, float vy, float ang, int owner)
        : pos{x, y}, vel{vx, vy}, angle{ang}, ownerId(owner) {}

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
        if (pos.x < -100 || pos.x > kWinW + 100 || pos.y < -100 || pos.y > kWinH + 100) {
            alive = false;
        }
    }

    void Render(SDL_Renderer *r) const {
        if (!alive || !projectileFrames[frame]) return;
        float w = 128.0f, h = 128.0f;
        SDL_FRect dst{pos.x - w * 0.5f, pos.y - h * 0.5f, w, h};
        SDL_RenderTextureRotated(r, projectileFrames[frame], nullptr, &dst, angle, nullptr, SDL_FLIP_NONE);
    }

    SDL_FRect Bounds() const {
        return SDL_FRect{ pos.x - 10.0f, pos.y - 10.0f, 20.0f, 20.0f };
    }
};

struct KillPopup {
    std::string text;
    float timer = 0.0f;
    float lifetime = 2.0f;
};
static std::vector<KillPopup> killPopups;

static ENetHost *netHost = nullptr;
static ENetPeer *netServer = nullptr;
static bool netIsHost = false;
static bool netConnected = false;

static std::vector<MpPlayer> mpPlayers;
static std::vector<MpProjectile> mpProjectiles;
static std::vector<Bat> mpBats;
static std::vector<SDL_FPoint> mpBatTargets;
static std::array<MpInputState, kMpMaxPlayers> mpInputs;
static std::array<Uint8, kMpMaxPlayers> mpPrevShoot;
static int mpLocalId = -1;
static float mpMatchTimer = 0.0f;
static bool mpMatchOver = false;
static std::string mpUserName = "Player";
static std::string mpHostIp = "127.0.0.1";
static std::string mpLocalIp = "Unknown";
static bool mpNameActive = false;
static bool mpIpActive = false;
static std::deque<std::string> mpSavedIps;
static float mpSnapshotTimer = 0.0f;
static float mpBatSpawnTimer = 0.0f;
static float mpBatSpawnInterval = 2.5f;
static SDL_FRect mpHostBtn{0,0,0,0};
static SDL_FRect mpJoinBtn{0,0,0,0};
static SDL_FRect mpBackBtn{0,0,0,0};
static SDL_FRect mpNameBox{0,0,0,0};
static SDL_FRect mpIpBox{0,0,0,0};
static SDL_FRect mpSavedBoxes[5];

struct WaveTextCache {
    std::string text;
    std::vector<CachedText> letters;
};
static WaveTextCache overCoinsWave;
static WaveTextCache overScoreWave;
static WaveTextCache overTimeWave;
static WaveTextCache overBestScoreWave;
static WaveTextCache overRestartWave;
static WaveTextCache overTitleWave;
static WaveTextCache overNewRecordWave;

static SDL_Color HSVToRGB(float hDeg, float s, float v) {
    float h = fmodf(hDeg, 360.0f);
    if (h < 0.0f) h += 360.0f;
    float c = v * s;
    float x = c * (1.0f - SDL_fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (h < 60.0f) { r = c; g = x; b = 0.0f; }
    else if (h < 120.0f) { r = x; g = c; b = 0.0f; }
    else if (h < 180.0f) { r = 0.0f; g = c; b = x; }
    else if (h < 240.0f) { r = 0.0f; g = x; b = c; }
    else if (h < 300.0f) { r = x; g = 0.0f; b = c; }
    else { r = c; g = 0.0f; b = x; }
    SDL_Color col;
    col.r = (Uint8)SDL_roundf((r + m) * 255.0f);
    col.g = (Uint8)SDL_roundf((g + m) * 255.0f);
    col.b = (Uint8)SDL_roundf((b + m) * 255.0f);
    col.a = 255;
    return col;
}

static SDL_Color RainbowColor(float t, float phase) {
    float hue = fmodf(t * 120.0f + phase, 360.0f);
    return HSVToRGB(hue, 1.0f, 1.0f);
}

static bool GetComboHudRect(SDL_FRect &outRect) {
    if (!font) return false;
    std::string comboStr = "Combo " + std::to_string(comboCount);
    int w = 0, h = 0;
    if (!TTF_GetStringSize(font, comboStr.c_str(), comboStr.size(), &w, &h)) return false;
    const float margin = 16.0f;
    const float baseY = 54.0f;
    outRect = SDL_FRect{ (float)kWinW - margin - (float)w, baseY, (float)w, (float)h };
    return true;
}

static void TriggerComboPopup() {
    if (comboCount < 10) return;
    if (!nicePopupText.tex) return;
    SDL_FRect comboRect;
    if (!GetComboHudRect(comboRect)) return;
    const float scale = 0.7f;
    ComboPopup popup;
    popup.pos.x = comboRect.x + comboRect.w - nicePopupText.w * scale;
    popup.pos.y = comboRect.y - nicePopupText.h * scale - 6.0f;
    comboPopups.push_back(popup);
}

static void DestroyCached(CachedText &ct) {
    if (ct.tex) { SDL_DestroyTexture(ct.tex); ct.tex = nullptr; }
    ct.w = ct.h = 0.0f;
    ct.text.clear();
}

static void UpdateCachedText(CachedText &ct, const std::string &s, SDL_Color col) {
    if (s == ct.text && ct.tex &&
        ct.color.r == col.r && ct.color.g == col.g && ct.color.b == col.b && ct.color.a == col.a) return;
    DestroyCached(ct);
    if (!font) return;
    SDL_Surface *surf = TTF_RenderText_Blended(font, s.c_str(), 0, col);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex && SDL_GetTextureSize(tex, &ct.w, &ct.h)) {
        ct.tex = tex;
        ct.text = s;
        ct.color = col;
    } else {
        if (tex) SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(surf);
}

static void UpdateComboText() {
    SDL_Color col{255, 255, 255, 255};
    UpdateCachedText(hudComboText, "Combo: " + std::to_string(comboCount), col);
}


static void UpdatePerfectScoreText() {
    SDL_Color col{255, 215, 0, 255}; // gold color
    UpdateCachedText(perfectShotText, "Perfect Shot!", col);
}

// Render a per-letter wave title with a subtle drop shadow.
struct CachedWaveLetter {
    SDL_Texture *tex = nullptr;
    float w = 0.0f;
    float h = 0.0f;
};
static std::vector<CachedWaveLetter> waveTitleCache;
static std::string cachedWaveTitle;

static void ClearWaveCache() {
    for (auto &c : waveTitleCache) {
        if (c.tex) SDL_DestroyTexture(c.tex);
    }
    waveTitleCache.clear();
    cachedWaveTitle.clear();
}

static void RenderWaveTitle(const std::string &text, float centerX, float baseY) {
    if (!font || text.empty()) return;

    if (text != cachedWaveTitle) {
        ClearWaveCache();
        cachedWaveTitle = text;
        SDL_Color white{255, 255, 255, 255};
        for (char ch : text) {
            char buf[2] = {ch, '\0'};
            SDL_Surface *surf = TTF_RenderText_Blended(font, buf, 0, white);
            if (!surf) continue;
            CachedWaveLetter letter;
            letter.tex = SDL_CreateTextureFromSurface(renderer, surf);
            letter.w = (float)surf->w;
            letter.h = (float)surf->h;
            SDL_DestroySurface(surf);
            waveTitleCache.push_back(letter);
        }
    }

    const float spacing = 2.0f;
    float totalW = 0.0f;
    for (const auto &c : waveTitleCache) totalW += c.w + spacing;
    if (!waveTitleCache.empty()) totalW -= spacing;

    double t = (double)SDL_GetTicks() / 1000.0;
    const float waveAmp = 6.0f;
    const float waveFreq = 3.0f;
    const float phaseStep = 0.5f;
    const float shadowOffset = 3.0f;

    float x = centerX - totalW * 0.5f;
    for (size_t i = 0; i < waveTitleCache.size(); ++i) {
        const auto &letter = waveTitleCache[i];
        if (!letter.tex) continue;
        float waveY = SDL_sinf((float)t * waveFreq + phaseStep * (float)i) * waveAmp;
        float y = baseY + waveY;

        SDL_SetTextureColorMod(letter.tex, 0, 0, 0);
        SDL_SetTextureAlphaMod(letter.tex, 180);
        SDL_FRect shadow{ x + shadowOffset, y + shadowOffset, letter.w, letter.h };
        SDL_RenderTexture(renderer, letter.tex, nullptr, &shadow);

        SDL_SetTextureColorMod(letter.tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(letter.tex, 255);
        SDL_FRect dst{ x, y, letter.w, letter.h };
        SDL_RenderTexture(renderer, letter.tex, nullptr, &dst);

        x += letter.w + spacing;
    }
}

static void ClearWaveTextCache(WaveTextCache &cache) {
    for (auto &c : cache.letters) DestroyCached(c);
    cache.letters.clear();
    cache.text.clear();
}

static float EnsureWaveTextCache(WaveTextCache &cache, const std::string &text, SDL_Color col, float spacing) {
    if (text != cache.text) {
        ClearWaveTextCache(cache);
        cache.letters.resize(text.size());
        cache.text = text;
    }
    float totalW = 0.0f;
    for (size_t i = 0; i < text.size(); ++i) {
        char cstr[2] = {text[i], '\0'};
        UpdateCachedText(cache.letters[i], cstr, col);
        totalW += cache.letters[i].w + spacing;
    }
    if (!text.empty()) totalW -= spacing;
    return totalW;
}

static void RenderWaveTextAtX(const WaveTextCache &cache, float startX, float baseY, double t,
                              float waveAmp, float waveFreq, float phaseStep, SDL_Color col, float spacing) {
    float x = startX;
    for (size_t i = 0; i < cache.letters.size(); ++i) {
        const auto &ct = cache.letters[i];
        if (!ct.tex) continue;
        float waveY = SDL_sinf((float)(t * waveFreq + phaseStep * (float)i)) * waveAmp;
        SDL_FRect shadow{ x + 2.0f, baseY + waveY + 2.0f, ct.w, ct.h };
        SDL_SetTextureColorMod(ct.tex, 0, 0, 0);
        SDL_SetTextureAlphaMod(ct.tex, 170);
        SDL_RenderTexture(renderer, ct.tex, nullptr, &shadow);
        SDL_SetTextureColorMod(ct.tex, col.r, col.g, col.b);
        SDL_SetTextureAlphaMod(ct.tex, 255);
        SDL_FRect dst{ x, baseY + waveY, ct.w, ct.h };
        SDL_RenderTexture(renderer, ct.tex, nullptr, &dst);
        x += ct.w + spacing;
    }
}

static void RenderWaveLineCentered(WaveTextCache &cache, const std::string &text, float centerX, float baseY,
                                   double t, float waveAmp, float waveFreq, float phaseStep, SDL_Color col) {
    const float spacing = 2.0f;
    float totalW = EnsureWaveTextCache(cache, text, col, spacing);
    float startX = centerX - totalW * 0.5f;
    RenderWaveTextAtX(cache, startX, baseY, t, waveAmp, waveFreq, phaseStep, col, spacing);
}

static bool GetBestScoreHudRect(SDL_FRect &outRect) {
    if (!font) return false;
    std::string label = "Best " + std::to_string(bestScore);
    int w = 0, h = 0;
    if (!TTF_GetStringSize(font, label.c_str(), label.size(), &w, &h)) return false;
    const float margin = 16.0f;
    outRect = SDL_FRect{ margin, 10.0f, (float)w, (float)h };
    return true;
}

static std::vector<CachedWaveLetter> beatScoreLetterCache;
static std::string cachedBeatScoreLabel;

static void BuildBeatScoreCache(const std::string &text) {
    if (!font) return;
    if (text == cachedBeatScoreLabel && !beatScoreLetterCache.empty()) return;
    for (auto &c : beatScoreLetterCache) {
        if (c.tex) SDL_DestroyTexture(c.tex);
    }
    beatScoreLetterCache.clear();
    cachedBeatScoreLabel = text;
    SDL_Color white{255, 255, 255, 255};
    for (char ch : text) {
        char buf[2] = {ch, '\0'};
        SDL_Surface *surf = TTF_RenderText_Blended(font, buf, 0, white);
        if (!surf) continue;
        CachedWaveLetter letter;
        letter.tex = SDL_CreateTextureFromSurface(renderer, surf);
        letter.w = (float)surf->w;
        letter.h = (float)surf->h;
        SDL_DestroySurface(surf);
        beatScoreLetterCache.push_back(letter);
    }
}

static void TriggerBeatScorePopup() {
    SDL_FRect bestRect;
    if (!GetBestScoreHudRect(bestRect)) return;
    BeatScorePopup popup;
    popup.pos.x = bestRect.x + bestRect.w + 2.0f;
    popup.pos.y = bestRect.y;
    beatScorePopups.push_back(popup);
}

static void RenderBeatScorePopup(const BeatScorePopup &bp) {
    const std::string text = "Beat Score!";
    BuildBeatScoreCache(text);
    if (beatScoreLetterCache.empty()) return;

    const float spacing = 1.0f;

    double tNow = (double)SDL_GetTicks() / 1000.0;
    float tNorm = bp.timer / bp.lifetime;
    float rise = 18.0f;
    float baseY = bp.pos.y - rise * tNorm;
    float x = bp.pos.x;
    float waveAmp = 6.0f;
    float waveFreq = 3.5f;
    float phaseStep = 0.6f;

    Uint8 alpha = 255;
    if (tNorm > 0.7f) {
        float fade = (1.0f - (tNorm - 0.7f) / 0.3f);
        if (fade < 0.0f) fade = 0.0f;
        alpha = (Uint8)SDL_roundf(255.0f * fade);
    }

    for (size_t i = 0; i < beatScoreLetterCache.size(); ++i) {
        const auto &letter = beatScoreLetterCache[i];
        if (!letter.tex) continue;
        float waveY = SDL_sinf((float)tNow * waveFreq + phaseStep * (float)i) * waveAmp;
        SDL_Color col = RainbowColor((float)tNow, 25.0f * (float)i);
        SDL_SetTextureColorMod(letter.tex, 0, 0, 0);
        SDL_SetTextureAlphaMod(letter.tex, (Uint8)SDL_roundf(alpha * 0.6f));
        SDL_FRect shadow{ x + 2.0f, baseY + waveY + 2.0f, letter.w, letter.h };
        SDL_RenderTexture(renderer, letter.tex, nullptr, &shadow);
        SDL_SetTextureColorMod(letter.tex, col.r, col.g, col.b);
        SDL_SetTextureAlphaMod(letter.tex, alpha);
        SDL_FRect dst{ x, baseY + waveY, letter.w, letter.h };
        SDL_RenderTexture(renderer, letter.tex, nullptr, &dst);
        x += letter.w + spacing;
    }

}

static void RenderLabel(const std::string &text, float x, float y) {
    if (!font || text.empty()) return;
    SDL_Color white{255, 255, 255, 255};
    SDL_Color shadow{0, 0, 0, 160};

    auto makeTex = [&](SDL_Color col) -> SDL_Texture* {
        SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), 0, col);
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

static void RenderLabelScaled(const std::string &text, float x, float y, float scale) {
    if (!font || text.empty()) return;
    SDL_Color white{255, 255, 255, 255};
    SDL_Color shadow{0, 0, 0, 160};

    auto makeTex = [&](SDL_Color col) -> SDL_Texture* {
        SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), 0, col);
        if (!surf) return nullptr;
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
        return tex;
    };

    SDL_Texture *texShadow = makeTex(shadow);
    SDL_Texture *texMain = makeTex(white);
    float w = 0.0f, h = 0.0f;
    if (texShadow && SDL_GetTextureSize(texShadow, &w, &h)) {
        SDL_FRect dst1{ x + 2.0f, y + 2.0f, w * scale, h * scale };
        SDL_FRect dst2{ x + 4.0f, y + 4.0f, w * scale, h * scale };
        SDL_RenderTexture(renderer, texShadow, nullptr, &dst1);
        SDL_RenderTexture(renderer, texShadow, nullptr, &dst2);
    }
    if (texMain && SDL_GetTextureSize(texMain, &w, &h)) {
        SDL_FRect dst{ x, y, w * scale, h * scale };
        SDL_RenderTexture(renderer, texMain, nullptr, &dst);
    }
    if (texShadow) SDL_DestroyTexture(texShadow);
    if (texMain) SDL_DestroyTexture(texMain);
}

static void RenderLabelColoredScaled(const std::string &text, float x, float y, float scale, SDL_Color col) {
    if (!font || text.empty()) return;
    SDL_Color shadow{0, 0, 0, 160};

    auto makeTex = [&](SDL_Color c) -> SDL_Texture* {
        SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), 0, c);
        if (!surf) return nullptr;
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
        return tex;
    };

    SDL_Texture *texShadow = makeTex(shadow);
    SDL_Texture *texMain = makeTex(col);
    float w = 0.0f, h = 0.0f;
    if (texShadow && SDL_GetTextureSize(texShadow, &w, &h)) {
        SDL_FRect dst1{ x + 2.0f, y + 2.0f, w * scale, h * scale };
        SDL_FRect dst2{ x + 4.0f, y + 4.0f, w * scale, h * scale };
        SDL_RenderTexture(renderer, texShadow, nullptr, &dst1);
        SDL_RenderTexture(renderer, texShadow, nullptr, &dst2);
    }
    if (texMain && SDL_GetTextureSize(texMain, &w, &h)) {
        SDL_FRect dst{ x, y, w * scale, h * scale };
        SDL_RenderTexture(renderer, texMain, nullptr, &dst);
    }
    if (texShadow) SDL_DestroyTexture(texShadow);
    if (texMain) SDL_DestroyTexture(texMain);
}

static SDL_FRect GetBirdBatHitbox(const SDL_FRect &rect) {
    float padX = 6.0f;
    float padTop = 4.0f;
    float padBottom = 8.0f;
    SDL_FRect hb{
        rect.x + padX,
        rect.y + padTop,
        rect.w - padX * 2.0f,
        rect.h - padTop - padBottom
    };
    if (hb.w < 1.0f) hb.w = 1.0f;
    if (hb.h < 1.0f) hb.h = 1.0f;
    return hb;
}

static void RenderCachedTextShadowed(const CachedText &ct, float x, float y, float alpha = 255.0f) {
    if (!ct.tex) return;
    Uint8 a = (Uint8)SDL_roundf(alpha);
    SDL_SetTextureColorMod(ct.tex, 0, 0, 0);
    SDL_SetTextureAlphaMod(ct.tex, (Uint8)SDL_roundf(a * 0.6f));
    SDL_FRect shadow{ x + 2.0f, y + 2.0f, ct.w, ct.h };
    SDL_RenderTexture(renderer, ct.tex, nullptr, &shadow);
    SDL_SetTextureColorMod(ct.tex, 255, 255, 255);
    SDL_SetTextureAlphaMod(ct.tex, a);
    SDL_FRect dst{ x, y, ct.w, ct.h };
    SDL_RenderTexture(renderer, ct.tex, nullptr, &dst);
}

static void RenderCachedTextShadowedScaled(const CachedText &ct, float x, float y, float scale, float alpha = 255.0f) {
    if (!ct.tex) return;
    float w = ct.w * scale;
    float h = ct.h * scale;
    Uint8 a = (Uint8)SDL_roundf(alpha);
    SDL_SetTextureColorMod(ct.tex, 0, 0, 0);
    SDL_SetTextureAlphaMod(ct.tex, (Uint8)SDL_roundf(a * 0.6f));
    SDL_FRect shadow{ x + 2.0f, y + 2.0f, w, h };
    SDL_RenderTexture(renderer, ct.tex, nullptr, &shadow);
    SDL_SetTextureColorMod(ct.tex, 255, 255, 255);
    SDL_SetTextureAlphaMod(ct.tex, a);
    SDL_FRect dst{ x, y, w, h };
    SDL_RenderTexture(renderer, ct.tex, nullptr, &dst);
}

template <typename T>
static void WriteValue(std::vector<Uint8> &buf, const T &value) {
    const Uint8 *ptr = reinterpret_cast<const Uint8 *>(&value);
    buf.insert(buf.end(), ptr, ptr + sizeof(T));
}

static void WriteBytes(std::vector<Uint8> &buf, const void *data, size_t size) {
    const Uint8 *ptr = reinterpret_cast<const Uint8 *>(data);
    buf.insert(buf.end(), ptr, ptr + size);
}

template <typename T>
static bool ReadValue(const Uint8 *data, size_t len, size_t &offset, T &out) {
    if (offset + sizeof(T) > len) return false;
    SDL_memcpy(&out, data + offset, sizeof(T));
    offset += sizeof(T);
    return true;
}

static bool ReadBytes(const Uint8 *data, size_t len, size_t &offset, void *out, size_t size) {
    if (offset + size > len) return false;
    SDL_memcpy(out, data + offset, size);
    offset += size;
    return true;
}

static bool InitNet() {
    if (enet_initialize() != 0) {
        SDL_Log("ENet init failed");
        return false;
    }
    return true;
}

static std::string GetLocalIp() {
#ifdef _WIN32
    char host[256] = {};
    if (gethostname(host, sizeof(host)) != 0) return "Unknown";
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo *res = nullptr;
    if (getaddrinfo(host, nullptr, &hints, &res) != 0 || !res) return "Unknown";
    std::string out = "Unknown";
    for (addrinfo *p = res; p; p = p->ai_next) {
        sockaddr_in *addr = (sockaddr_in *)p->ai_addr;
        const char *ip = inet_ntoa(addr->sin_addr);
        if (ip) {
            std::string s = ip;
            if (s.rfind("127.", 0) != 0) { out = s; break; }
            out = s;
        }
    }
    freeaddrinfo(res);
    return out;
#else
    return "Unknown";
#endif
}

static void ShutdownNet() {
    if (netServer) {
        enet_peer_disconnect(netServer, 0);
        netServer = nullptr;
    }
    if (netHost) {
        enet_host_destroy(netHost);
        netHost = nullptr;
    }
    netConnected = false;
    netIsHost = false;
    enet_deinitialize();
}

static void NetResetSession() {
    mpPlayers.clear();
    mpProjectiles.clear();
    mpBats.clear();
    for (auto &in : mpInputs) in = MpInputState{};
    for (auto &s : mpPrevShoot) s = 0;
    mpLocalId = -1;
    mpMatchTimer = 0.0f;
    mpBatSpawnTimer = 0.0f;
    mpBatSpawnInterval = 2.5f;
    mpMatchOver = false;
    killPopups.clear();
}

static bool NetStartHost() {
    NetResetSession();
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = kMpPort;
    netHost = enet_host_create(&address, kMpMaxPlayers, 2, 0, 0);
    if (!netHost) {
        SDL_Log("Failed to create ENet host");
        return false;
    }
    netIsHost = true;
    netConnected = true;
    mpLocalId = 0;
    MpPlayer hostPlayer;
    hostPlayer.id = 0;
    hostPlayer.name = mpUserName;
    hostPlayer.rect = SDL_FRect{ kWinW * 0.5f - 20.0f, kWinH * 0.5f - 20.0f, 40.0f, 40.0f };
    hostPlayer.alive = true;
    mpPlayers.push_back(hostPlayer);
    return true;
}

static bool NetStartClient(const std::string &ip) {
    NetResetSession();
    ENetAddress address;
    if (enet_address_set_host(&address, ip.c_str()) != 0) {
        SDL_Log("Invalid host: %s", ip.c_str());
        return false;
    }
    address.port = kMpPort;
    netHost = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!netHost) {
        SDL_Log("Failed to create ENet client");
        return false;
    }
    netServer = enet_host_connect(netHost, &address, 2, 0);
    if (!netServer) {
        SDL_Log("Failed to connect");
        return false;
    }
    netIsHost = false;
    netConnected = false;
    return true;
}

static void NetDisconnect() {
    if (netServer) {
        enet_peer_disconnect(netServer, 0);
        netServer = nullptr;
    }
    if (netHost) {
        enet_host_destroy(netHost);
        netHost = nullptr;
    }
    netConnected = false;
    netIsHost = false;
}


static void LoadSavedIps() {
    mpSavedIps.clear();
    std::ifstream in("records/ips.txt");
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        mpSavedIps.push_back(line);
        if (mpSavedIps.size() >= 5) break;
    }
}

static void SaveSavedIps() {
    std::ofstream out("records/ips.txt", std::ios::trunc);
    if (!out) return;
    for (const auto &ip : mpSavedIps) {
        out << ip << "\n";
    }
}

static void NetSendJoinRequest() {
    if (!netHost || !netServer) return;
    JoinRequestPacket pkt{};
    pkt.type = (Uint8)NetMsg::JoinRequest;
    SDL_strlcpy(pkt.name, mpUserName.c_str(), sizeof(pkt.name));
    ENetPacket *packet = enet_packet_create(&pkt, sizeof(pkt), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(netServer, 0, packet);
}

static void NetSendInput(int playerId, const MpInputState &input) {
    if (!netHost || (!netIsHost && !netServer)) return;
    InputPacket pkt{};
    pkt.type = (Uint8)NetMsg::Input;
    pkt.id = (Uint8)playerId;
    pkt.buttons = input.buttons;
    pkt.aimX = input.aimX;
    pkt.aimY = input.aimY;
    pkt.shoot = input.shoot;
    ENetPacket *packet = enet_packet_create(&pkt, sizeof(pkt), 0);
    if (netIsHost) {
        enet_host_broadcast(netHost, 0, packet);
    } else {
        enet_peer_send(netServer, 0, packet);
    }
}

static void NetSendSnapshot() {
    if (!netIsHost || !netHost) return;
    std::vector<Uint8> buf;
    SnapshotHeader hdr{};
    hdr.type = (Uint8)NetMsg::Snapshot;
    hdr.matchTimer = mpMatchTimer;
    hdr.playerCount = 0;
    hdr.batCount = (Uint8)SDL_min((int)mpBats.size(), kMpMaxBats);
    hdr.projCount = (Uint16)SDL_min((int)mpProjectiles.size(), kMpMaxProjectiles);
    for (const auto &p : mpPlayers) {
        if (p.connected) hdr.playerCount++;
    }
    WriteBytes(buf, &hdr, sizeof(hdr));

    int sent = 0;
    for (const auto &p : mpPlayers) {
        if (!p.connected) continue;
        if (sent >= hdr.playerCount) break;
        WriteValue(buf, (Uint8)p.id);
        char nameBuf[kMpNameLen] = {};
        SDL_strlcpy(nameBuf, p.name.c_str(), sizeof(nameBuf));
        WriteBytes(buf, nameBuf, sizeof(nameBuf));
        WriteValue(buf, p.rect.x);
        WriteValue(buf, p.rect.y);
        WriteValue(buf, p.vel.x);
        WriteValue(buf, p.vel.y);
        WriteValue(buf, (Uint8)p.health);
        WriteValue(buf, (Uint8)(p.alive ? 1 : 0));
        WriteValue(buf, (Uint8)(p.facingRight ? 1 : 0));
        WriteValue(buf, (Uint16)p.kills);
        WriteValue(buf, (Uint16)p.deaths);
        WriteValue(buf, p.score);
        WriteValue(buf, (Uint8)(p.invincible ? 1 : 0));
        WriteValue(buf, p.respawnTimer);
        sent++;
    }

    for (int i = 0; i < hdr.batCount; ++i) {
        const auto &b = mpBats[i];
        WriteValue(buf, b.pos.x);
        WriteValue(buf, b.pos.y);
        WriteValue(buf, (Uint8)b.state);
        WriteValue(buf, (Uint8)b.frame);
        WriteValue(buf, (Uint8)(b.alive ? 1 : 0));
    }

    for (int i = 0; i < hdr.projCount; ++i) {
        const auto &p = mpProjectiles[i];
        WriteValue(buf, p.pos.x);
        WriteValue(buf, p.pos.y);
        WriteValue(buf, p.angle);
        WriteValue(buf, (Uint8)p.frame);
        WriteValue(buf, (Uint8)(p.alive ? 1 : 0));
    }

    ENetPacket *pkt = enet_packet_create(buf.data(), buf.size(), 0);
    enet_host_broadcast(netHost, 0, pkt);
}

static void NetSendKillPopup(const std::string &who) {
    if (!netIsHost || !netHost) return;
    KillNoticePacket pkt{};
    pkt.type = (Uint8)NetMsg::KillNotice;
    SDL_strlcpy(pkt.name, who.c_str(), sizeof(pkt.name));
    ENetPacket *packet = enet_packet_create(&pkt, sizeof(pkt), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(netHost, 0, packet);
}

static void NetSendMatchOver() {
    if (!netIsHost || !netHost) return;
    MatchOverPacket pkt{};
    pkt.type = (Uint8)NetMsg::MatchOver;
    ENetPacket *packet = enet_packet_create(&pkt, sizeof(pkt), ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(netHost, 0, packet);
}

static MpPlayer* FindMpPlayer(int id) {
    for (auto &p : mpPlayers) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

static int FindFreePlayerId() {
    for (int id = 0; id < kMpMaxPlayers; ++id) {
        if (!FindMpPlayer(id)) return id;
    }
    return -1;
}

static void NetHandleSnapshot(const Uint8 *data, size_t len) {
    size_t off = 0;
    SnapshotHeader hdr{};
    if (!ReadBytes(data, len, off, &hdr, sizeof(hdr))) return;
    mpMatchTimer = hdr.matchTimer;

    std::vector<int> incomingIds;
    for (int i = 0; i < hdr.playerCount; ++i) {
        MpPlayer p;
        Uint8 id = 0;
        Uint8 health = 0, alive = 0, facing = 0, inv = 0;
        Uint16 kills = 0, deaths = 0;
        if (!ReadValue(data, len, off, id)) return;
        char nameBuf[kMpNameLen] = {};
        if (!ReadBytes(data, len, off, nameBuf, sizeof(nameBuf))) return;
        if (!ReadValue(data, len, off, p.rect.x)) return;
        if (!ReadValue(data, len, off, p.rect.y)) return;
        if (!ReadValue(data, len, off, p.vel.x)) return;
        if (!ReadValue(data, len, off, p.vel.y)) return;
        if (!ReadValue(data, len, off, health)) return;
        if (!ReadValue(data, len, off, alive)) return;
        if (!ReadValue(data, len, off, facing)) return;
        if (!ReadValue(data, len, off, kills)) return;
        if (!ReadValue(data, len, off, deaths)) return;
        if (!ReadValue(data, len, off, p.score)) return;
        if (!ReadValue(data, len, off, inv)) return;
        if (!ReadValue(data, len, off, p.respawnTimer)) return;
        p.id = id;
        p.name = nameBuf;
        p.health = health;
        p.alive = alive != 0;
        p.facingRight = facing != 0;
        p.kills = kills;
        p.deaths = deaths;
        p.invincible = inv != 0;
        p.rect.w = 40.0f;
        p.rect.h = 40.0f;
        p.targetRect = p.rect;
        p.targetVel = p.vel;
        p.connected = true;
        p.hasTarget = true;
        incomingIds.push_back(id);

        MpPlayer *existing = FindMpPlayer(id);
        if (existing) {
            existing->targetRect = p.rect;
            existing->targetVel = p.vel;
            existing->alive = p.alive;
            existing->facingRight = p.facingRight;
            existing->health = p.health;
            existing->kills = p.kills;
            existing->deaths = p.deaths;
            existing->score = p.score;
            existing->invincible = p.invincible;
            existing->respawnTimer = p.respawnTimer;
            existing->name = p.name;
            existing->connected = true;
            existing->hasTarget = true;
        } else {
            mpPlayers.push_back(p);
        }
    }

    mpPlayers.erase(std::remove_if(mpPlayers.begin(), mpPlayers.end(),
        [&](const MpPlayer &p){
            return std::find(incomingIds.begin(), incomingIds.end(), p.id) == incomingIds.end();
        }), mpPlayers.end());

    if (mpBats.size() != hdr.batCount) {
        mpBats.clear();
        mpBatTargets.clear();
        mpBats.reserve(hdr.batCount);
        mpBatTargets.reserve(hdr.batCount);
        for (int i = 0; i < hdr.batCount; ++i) {
            mpBats.emplace_back(0.0f, 0.0f, 0.0f);
            mpBatTargets.push_back(SDL_FPoint{0.0f, 0.0f});
        }
    }
    for (int i = 0; i < hdr.batCount; ++i) {
        Bat &b = mpBats[i];
        Uint8 state = 0, frame = 0, alive = 0;
        float bx = 0.0f, by = 0.0f;
        if (!ReadValue(data, len, off, bx)) return;
        if (!ReadValue(data, len, off, by)) return;
        if (!ReadValue(data, len, off, state)) return;
        if (!ReadValue(data, len, off, frame)) return;
        if (!ReadValue(data, len, off, alive)) return;
        mpBatTargets[i] = SDL_FPoint{bx, by};
        if (b.pos.x == 0.0f && b.pos.y == 0.0f) {
            b.pos.x = bx;
            b.pos.y = by;
        }
        b.state = (BatState)state;
        b.frame = frame;
        b.alive = alive != 0;
    }

    mpProjectiles.clear();
    for (int i = 0; i < hdr.projCount; ++i) {
        float x = 0.0f, y = 0.0f, angle = 0.0f;
        Uint8 frame = 0, alive = 0;
        if (!ReadValue(data, len, off, x)) return;
        if (!ReadValue(data, len, off, y)) return;
        if (!ReadValue(data, len, off, angle)) return;
        if (!ReadValue(data, len, off, frame)) return;
        if (!ReadValue(data, len, off, alive)) return;
        MpProjectile p(x, y, 0.0f, 0.0f, angle, -1);
        p.frame = frame;
        p.alive = alive != 0;
        mpProjectiles.push_back(p);
    }
}

static std::mt19937 rng;
enum class GameState { Menu, Playing, Paused, Options, GameOver, MultiplayerMenu, MultiplayerLobby, MultiplayerPlaying, MultiplayerOver };
static GameState gameState = GameState::Menu;
static void NetPoll() {
    if (!netHost) return;
    ENetEvent event;
    while (enet_host_service(netHost, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if (!netIsHost) {
                    netConnected = true;
                    NetSendJoinRequest();
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE: {
                const Uint8 *data = event.packet->data;
                size_t len = event.packet->dataLength;
                if (len < 1) { enet_packet_destroy(event.packet); break; }
                NetMsg type = (NetMsg)data[0];
                size_t off = 1;
                if (type == NetMsg::JoinRequest && netIsHost) {
                    if (mpPlayers.size() >= kMpMaxPlayers) {
                        enet_packet_destroy(event.packet);
                        break;
                    }
                    if (len < sizeof(JoinRequestPacket)) break;
                    JoinRequestPacket req{};
                    SDL_memcpy(&req, data, sizeof(req));
                    MpPlayer p;
                    p.id = FindFreePlayerId();
                    if (p.id < 0) break;
                    p.name = req.name;
                    p.rect = SDL_FRect{ 120.0f + 30.0f * p.id, 200.0f, 40.0f, 40.0f };
                    p.alive = true;
                    p.connected = true;
                    mpPlayers.push_back(p);
                    event.peer->data = reinterpret_cast<void*>((intptr_t)p.id);

                    std::vector<Uint8> buf;
                    JoinAcceptHeader hdr{};
                    hdr.type = (Uint8)NetMsg::JoinAccept;
                    hdr.assignedId = (Uint8)p.id;
                    hdr.count = 0;
                    for (const auto &pl : mpPlayers) {
                        if (pl.connected) hdr.count++;
                    }
                    WriteBytes(buf, &hdr, sizeof(hdr));
                    int sent = 0;
                    for (const auto &pl : mpPlayers) {
                        if (!pl.connected) continue;
                        if (sent >= hdr.count) break;
                        JoinAcceptEntry entry{};
                        entry.id = (Uint8)pl.id;
                        SDL_strlcpy(entry.name, pl.name.c_str(), sizeof(entry.name));
                        WriteBytes(buf, &entry, sizeof(entry));
                        sent++;
                    }
                    ENetPacket *pkt = enet_packet_create(buf.data(), buf.size(), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(event.peer, 0, pkt);
                } else if (type == NetMsg::JoinAccept && !netIsHost) {
                    if (len < sizeof(JoinAcceptHeader)) break;
                    JoinAcceptHeader hdr{};
                    SDL_memcpy(&hdr, data, sizeof(hdr));
                    size_t entryOff = sizeof(hdr);
                    mpLocalId = hdr.assignedId;
                    mpPlayers.clear();
                    for (int i = 0; i < hdr.count; ++i) {
                        if (entryOff + sizeof(JoinAcceptEntry) > len) break;
                        JoinAcceptEntry entry{};
                        SDL_memcpy(&entry, data + entryOff, sizeof(entry));
                        entryOff += sizeof(entry);
                        MpPlayer p;
                        p.id = entry.id;
                        p.name = entry.name;
                        p.rect = SDL_FRect{ 120.0f + 30.0f * entry.id, 200.0f, 40.0f, 40.0f };
                        p.targetRect = p.rect;
                        p.alive = true;
                        p.connected = true;
                        mpPlayers.push_back(p);
                    }
                    gameState = GameState::MultiplayerPlaying;
                    SDL_SetWindowMouseGrab(window, true);
                    SDL_HideCursor();
                } else if (type == NetMsg::Input && netIsHost) {
                    if (len < sizeof(InputPacket)) break;
                    InputPacket pkt{};
                    SDL_memcpy(&pkt, data, sizeof(pkt));
                    MpInputState input;
                    input.buttons = pkt.buttons;
                    input.aimX = pkt.aimX;
                    input.aimY = pkt.aimY;
                    input.shoot = pkt.shoot;
                    if (pkt.id < mpInputs.size()) {
                        mpInputs[pkt.id] = input;
                    }
                } else if (type == NetMsg::Snapshot && !netIsHost) {
                    NetHandleSnapshot(data, len);
                } else if (type == NetMsg::KillNotice) {
                    if (len < sizeof(KillNoticePacket)) break;
                    KillNoticePacket pkt{};
                    SDL_memcpy(&pkt, data, sizeof(pkt));
                    KillPopup kp;
                    kp.text = std::string(pkt.name) + " killed!";
                    killPopups.push_back(kp);
                } else if (type == NetMsg::MatchOver) {
                    mpMatchOver = true;
                }
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                netConnected = false;
                if (!netIsHost && (gameState == GameState::MultiplayerPlaying || gameState == GameState::MultiplayerOver)) {
                    gameState = GameState::MultiplayerMenu;
                    SDL_ShowCursor();
                    SDL_SetWindowMouseGrab(window, false);
                    SDL_StartTextInput(window);
                }
                if (netIsHost) {
                    int id = (int)(intptr_t)event.peer->data;
                    mpPlayers.erase(std::remove_if(mpPlayers.begin(), mpPlayers.end(),
                        [&](const MpPlayer &p){ return p.id == id; }), mpPlayers.end());
                }
                break;
            default:
                break;
        }
    }
}

static SDL_FPoint MpGetTargetPoint() {
    for (const auto &p : mpPlayers) {
        if (p.alive) {
            return SDL_FPoint{ p.rect.x + p.rect.w * 0.5f, p.rect.y + p.rect.h * 0.5f };
        }
    }
    return SDL_FPoint{ kWinW * 0.5f, kWinH * 0.5f };
}

static void MpDamagePlayer(MpPlayer &p, int killerId) {
    if (!p.alive || p.invincible) return;
    p.health -= 1;
    p.invincible = true;
    p.invincibleTimer = 1.2f;
    if (p.health <= 0) {
        p.alive = false;
        p.deaths += 1;
        p.respawnTimer = 2.5f;
        if (killerId >= 0 && killerId < (int)mpPlayers.size()) {
            mpPlayers[killerId].score += 10;
            mpPlayers[killerId].kills += 1;
            NetSendKillPopup(p.name);
            KillPopup kp;
            kp.text = p.name + " killed!";
            killPopups.push_back(kp);
        }
    }
}

static void MpRespawnPlayer(MpPlayer &p) {
    p.health = 3;
    p.alive = true;
    p.invincible = true;
    p.invincibleTimer = 1.5f;
    p.vel = SDL_FPoint{0.0f, 0.0f};
    p.rect = SDL_FRect{ kWinW * 0.5f - 20.0f, kWinH * 0.5f - 20.0f, 40.0f, 40.0f };
}

static void MpApplyInput(MpPlayer &p, const MpInputState &in, float dt) {
    if (!p.alive) {
        if (p.respawnTimer > 0.0f) {
            p.respawnTimer -= dt;
            if (p.respawnTimer <= 0.0f) {
                MpRespawnPlayer(p);
            }
        }
        return;
    }

    if (p.invincibleTimer > 0.0f) {
        p.invincibleTimer -= dt;
        if (p.invincibleTimer <= 0.0f) {
            p.invincibleTimer = 0.0f;
            p.invincible = false;
        }
    }

    const float kGravity = 900.0f;
    const float kJumpImpulse = -350.0f;
    const float kDiveAccel = 1200.0f;
    const float kFloorY = 480.0f;

    bool leftDown = (in.buttons & kMpBtnLeft) != 0;
    bool rightDown = (in.buttons & kMpBtnRight) != 0;
    bool jumpDown = (in.buttons & kMpBtnJump) != 0;
    bool downDown = (in.buttons & kMpBtnDown) != 0;

    if (leftDown && !rightDown) {
        p.vel.x = -200.0f;
        p.facingRight = false;
    } else if (rightDown && !leftDown) {
        p.vel.x = 200.0f;
        p.facingRight = true;
    } else {
        p.vel.x = 0.0f;
    }

    if (jumpDown && !p.wasJumpDown) {
        p.vel.y = kJumpImpulse;
    }
    p.wasJumpDown = jumpDown;

    p.vel.y += kGravity * dt;
    if (downDown) {
        p.vel.y += kDiveAccel * dt;
    }
    p.rect.x += p.vel.x * dt;
    p.rect.y += p.vel.y * dt;

    const float floor = kFloorY - p.rect.h;
    if (p.rect.y > floor || p.rect.y < 0.0f || p.rect.x < 0.0f || p.rect.x + p.rect.w > (float)kWinW) {
        MpDamagePlayer(p, -1);
        p.rect.x = ClampF(p.rect.x, 0.0f, (float)kWinW - p.rect.w);
        p.rect.y = ClampF(p.rect.y, 0.0f, floor);
    }
}

static void MpSpawnBat() {
    if (mpBats.size() >= kMpMaxBats) return;
    std::uniform_real_distribution<float> xdist(-60.0f, kWinW + 60.0f);
    std::uniform_real_distribution<float> ydist(-60.0f, kWinH + 60.0f);
    std::uniform_real_distribution<float> speedDist(220.0f, 300.0f);
    std::uniform_int_distribution<int> sideDist(0, 3);

    float spawnX = kWinW + 80.0f;
    float spawnY = kWinH * 0.5f;
    int side = sideDist(rng);
    if (side == 0) {
        spawnX = kWinW + 80.0f;
        spawnY = ydist(rng);
    } else if (side == 1) {
        spawnX = -80.0f;
        spawnY = ydist(rng);
    } else if (side == 2) {
        spawnX = xdist(rng);
        spawnY = -80.0f;
    } else {
        spawnX = xdist(rng);
        spawnY = kWinH + 80.0f;
    }

    float spd = speedDist(rng);
    mpBats.emplace_back(spawnX, spawnY, spd);
}

static void MpUpdateHost(float dt) {
    if (mpMatchOver) return;
    mpMatchTimer += dt;
    if (mpMatchTimer >= kMpMatchDuration) {
        mpMatchTimer = kMpMatchDuration;
        mpMatchOver = true;
        NetSendMatchOver();
        return;
    }

    for (auto &p : mpPlayers) {
        if (p.id >= 0 && p.id < (int)mpInputs.size()) {
            MpApplyInput(p, mpInputs[p.id], dt);
            if (p.id < (int)mpPrevShoot.size()) {
                if (mpInputs[p.id].shoot && !mpPrevShoot[p.id]) {
                    float pcx = p.rect.x + p.rect.w * 0.5f;
                    float pcy = p.rect.y + p.rect.h * 0.5f;
                    float dx = mpInputs[p.id].aimX - pcx;
                    float dy = mpInputs[p.id].aimY - pcy;
                    float len = SDL_sqrtf(dx*dx + dy*dy);
                    if (len < 0.001f) len = 0.001f;
                    dx /= len; dy /= len;
                    const float speed = 400.0f;
                    float vx = dx * speed;
                    float vy = dy * speed;
                    float angle = SDL_atan2(dy, dx) * (180.0f / SDL_PI_D);
                    mpProjectiles.emplace_back(pcx, pcy, vx, vy, angle, p.id);
                    if (mpProjectiles.size() > kMpMaxProjectiles) {
                        mpProjectiles.erase(mpProjectiles.begin());
                    }
                }
                mpPrevShoot[p.id] = mpInputs[p.id].shoot;
            }
        }
    }

    for (auto &proj : mpProjectiles) proj.Update(dt);
    mpProjectiles.erase(std::remove_if(mpProjectiles.begin(), mpProjectiles.end(),
        [](const MpProjectile &p){ return !p.alive; }), mpProjectiles.end());

    mpBatSpawnTimer += dt;
    if (mpBatSpawnTimer >= mpBatSpawnInterval) {
        mpBatSpawnTimer = 0.0f;
        MpSpawnBat();
        float diff = ClampF(1.0f + mpMatchTimer * 0.02f, 1.0f, 3.0f);
        mpBatSpawnInterval = ClampF(2.2f / diff, 0.8f, 2.2f);
    }

    SDL_FPoint target = MpGetTargetPoint();
    for (auto &b : mpBats) {
        b.Update(dt, target);
    }
    mpBats.erase(std::remove_if(mpBats.begin(), mpBats.end(), [](const Bat &b){ return !b.alive; }), mpBats.end());

    for (auto &proj : mpProjectiles) {
        if (!proj.alive) continue;
        SDL_FRect projBox = proj.Bounds();
        for (auto &b : mpBats) {
            if (!b.alive || b.state != BatState::Moving) continue;
            if (AABBOverlap(projBox, b.Bounds())) {
                b.Kill();
                proj.alive = false;
                if (proj.ownerId >= 0 && proj.ownerId < (int)mpPlayers.size()) {
                    mpPlayers[proj.ownerId].score += 1;
                    mpPlayers[proj.ownerId].kills += 1;
                }
                break;
            }
        }
    }

    for (auto &proj : mpProjectiles) {
        if (!proj.alive) continue;
        SDL_FRect projBox = proj.Bounds();
        for (auto &p : mpPlayers) {
            if (!p.alive) continue;
            if (p.id == proj.ownerId) continue;
            if (AABBOverlap(projBox, p.rect)) {
                proj.alive = false;
                MpDamagePlayer(p, proj.ownerId);
                break;
            }
        }
    }

    for (auto &b : mpBats) {
        if (!b.alive || b.state != BatState::Moving) continue;
        for (auto &p : mpPlayers) {
            if (!p.alive) continue;
            if (AABBOverlap(GetBirdBatHitbox(p.rect), b.Bounds())) {
                b.Kill();
                MpDamagePlayer(p, -1);
                break;
            }
        }
    }

    for (auto &kp : killPopups) kp.timer += dt;
    killPopups.erase(std::remove_if(killPopups.begin(), killPopups.end(),
        [](const KillPopup &k){ return k.timer >= k.lifetime; }), killPopups.end());
}

static void MpUpdateClient(float dt) {
    const float lerpSpeed = 8.0f;
    for (auto &p : mpPlayers) {
        if (p.hasTarget) {
            float t = SDL_min(1.0f, dt * lerpSpeed);
            p.rect.x += (p.targetRect.x - p.rect.x) * t;
            p.rect.y += (p.targetRect.y - p.rect.y) * t;
            p.vel.x += (p.targetVel.x - p.vel.x) * t;
            p.vel.y += (p.targetVel.y - p.vel.y) * t;
        }
    }
    for (size_t i = 0; i < mpBats.size() && i < mpBatTargets.size(); ++i) {
        float t = SDL_min(1.0f, dt * lerpSpeed);
        mpBats[i].pos.x += (mpBatTargets[i].x - mpBats[i].pos.x) * t;
        mpBats[i].pos.y += (mpBatTargets[i].y - mpBats[i].pos.y) * t;
    }
    for (auto &kp : killPopups) kp.timer += dt;
    killPopups.erase(std::remove_if(killPopups.begin(), killPopups.end(),
        [](const KillPopup &k){ return k.timer >= k.lifetime; }), killPopups.end());
}

static void MpRenderPlayers() {
    for (const auto &p : mpPlayers) {
        if (!p.alive) continue;
        SDL_Texture *tex = runningAnim.Current();
        if (SDL_fabsf(p.vel.y) > 5.0f && flyingAnim.Current()) {
            tex = flyingAnim.Current();
        }
        if (!tex) continue;
        SDL_FlipMode flip = p.facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
        SDL_RenderTextureRotated(renderer, tex, nullptr, &p.rect, 0.0, nullptr, flip);
        RenderLabel(p.name, p.rect.x - 4.0f, p.rect.y - 16.0f);
        if (p.invincible) {
            SDL_SetRenderDrawColor(renderer, 120, 220, 255, 120);
            SDL_FRect halo{ p.rect.x - 4.0f, p.rect.y - 4.0f, p.rect.w + 8.0f, p.rect.h + 8.0f };
            SDL_RenderRect(renderer, &halo);
        }
    }
}

static void MpRenderHUD() {
    SDL_Color white{255,255,255,255};
    float x = 16.0f;
    float y = 10.0f;
    RenderLabel("MP MATCH", x, y);
    y += 22.0f;

    int remaining = (int)std::max(0.0f, kMpMatchDuration - mpMatchTimer);
    char timeBuf[16];
    SDL_snprintf(timeBuf, sizeof(timeBuf), "Time %02d:%02d", remaining / 60, remaining % 60);
    RenderLabel(timeBuf, x, y);
    y += 22.0f;

    for (const auto &p : mpPlayers) {
        std::string line = p.name + "  K:" + std::to_string(p.kills) +
            " D:" + std::to_string(p.deaths) + " S:" + std::to_string(p.score);
        RenderLabel(line, x, y);
        y += 18.0f;
    }
}

static void MpRenderPing() {
    float pingMs = 0.0f;
    if (netIsHost) {
        pingMs = 0.0f;
    } else if (netServer) {
        pingMs = (float)netServer->roundTripTime;
    }

    SDL_Color col{0, 220, 80, 255};
    if (pingMs > 160.0f) col = SDL_Color{220, 60, 60, 255};
    else if (pingMs > 80.0f) col = SDL_Color{230, 190, 60, 255};

    std::string label = netIsHost ? "Ping: host" : ("Ping: " + std::to_string((int)pingMs) + "ms");
    int w = 0, h = 0;
    if (!TTF_GetStringSize(font, label.c_str(), label.size(), &w, &h)) return;
    float scale = 0.7f;
    float x = (float)kWinW - (float)w * scale - 16.0f;
    float y = (float)kWinH - (float)h * scale - 16.0f;
    RenderLabelColoredScaled(label, x, y, scale, col);
}

static void MpRenderMatchOver() {
    RenderLabel("MATCH OVER", kWinW * 0.5f - 90.0f, kWinH * 0.5f - 80.0f);
    std::vector<MpPlayer> sorted = mpPlayers;
    std::sort(sorted.begin(), sorted.end(), [](const MpPlayer &a, const MpPlayer &b){
        return a.score > b.score;
    });
    float y = kWinH * 0.5f - 40.0f;
    for (const auto &p : sorted) {
        std::string line = p.name + "  Score " + std::to_string(p.score) +
            "  K:" + std::to_string(p.kills) + " D:" + std::to_string(p.deaths);
        RenderLabel(line, kWinW * 0.5f - 160.0f, y);
        y += 22.0f;
    }
    RenderLabel("ESC to exit", kWinW * 0.5f - 80.0f, y + 16.0f);
}

static void MpRenderKillPopups() {
    if (killPopups.empty()) return;
    double tNow = (double)SDL_GetTicks() / 1000.0;
    float y = 80.0f;
    for (const auto &kp : killPopups) {
        SDL_Color col = RainbowColor((float)tNow, kp.timer * 120.0f);
        SDL_Surface *surf = TTF_RenderText_Blended(font, kp.text.c_str(), 0, col);
        if (!surf) continue;
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        if (tex) {
            float w = (float)surf->w;
            float h = (float)surf->h;
            float x = kWinW * 0.5f - w * 0.5f;
            float bob = SDL_sinf((float)(tNow * 4.0f)) * 6.0f;
            SDL_SetTextureColorMod(tex, 0, 0, 0);
            SDL_SetTextureAlphaMod(tex, 160);
            SDL_FRect shadow{ x + 2.0f, y + bob + 2.0f, w, h };
            SDL_RenderTexture(renderer, tex, nullptr, &shadow);
            SDL_SetTextureColorMod(tex, col.r, col.g, col.b);
            SDL_SetTextureAlphaMod(tex, 255);
            SDL_FRect dst{ x, y + bob, w, h };
            SDL_RenderTexture(renderer, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
        }
        SDL_DestroySurface(surf);
        y += 26.0f;
    }
}

static bool LoadPausedLetters(SDL_Renderer *r, TTF_Font *f) {
    const char *word = "Paused";
    SDL_Color white{255, 255, 255, 255};
    for (int i = 0; i < 6; ++i) {
        char cstr[2] = {word[i], '\0'};
        SDL_Surface *surf = TTF_RenderText_Blended(f, cstr, 0, white);
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
static uint64_t startTicks = 0;
static int bestCoins = 0;
static float bestTime = 0.0f;


static bool newRecord = false;
SpriteAnim runningAnim;
SpriteAnim flyingAnim;
Player player;
static bool showFlyHint = true;
static bool waitingForStart = true;
inline bool IsPlayingActive() { return gameState == GameState::Playing && !playerDead && !player.dead; }


static int hudLastCoins = -1;
static int hudLastTime = -1;
static int hudLastBestScore = -1;
static int hudLastScore = -1;
static float musicVolumeSetting = 0.1f; // 0..1
static float sfxVolumeSetting = 0.1f;    // 0..1
static bool draggingMusicSlider = false;
static bool draggingSfxSlider = false;
static bool optionsFromPaused = false;
struct MenuButton {
    std::string label;
    SDL_FRect bounds{0,0,0,0};
    bool hover = false;
    float phase = 0.0f;
};
static MenuButton btnStart{ "START", {0,0,0,0}, false, 0.0f };
static MenuButton btnOptions{ "OPTIONS", {0,0,0,0}, false, 0.4f };
static MenuButton btnMultiplayer{ "MULTIPLAYER", {0,0,0,0}, false, 0.8f };
static MenuButton btnQuit{ "QUIT", {0,0,0,0}, false, 0.8f };
struct Demo {
    SDL_FPoint pos{240.0f, 260.0f};
    SDL_FPoint vel{80.0f, 0.0f};
    float t = 0.0f;
    float angleDeg = 0.0f;
} static demo;
static std::vector<DemoProjectile> demoProjectiles;
static float demoShotTimer = 0.0f;

enum class PowerupType { Magnet, Shield, Spread };
struct Powerup {
    PowerupType type;
    SDL_FPoint pos;
    bool alive = true;
    float ttl = 12.0f;
    float speed = 0.0f;
    float amp = 0.0f;
    float freq = 0.0f;
    float phase = 0.0f;
    float t = 0.0f;
};
static std::vector<Powerup> powerups;
static float powerupSpawnTimer = 0.0f;
static float powerupSpawnInterval = 10.0f;

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
    floatingTexts.clear();
    score = 0;
    comboCount = 0;
    comboTimer = 0.0f;
    DestroyCached(hudComboText);
    comboPopups.clear();
    beatScorePopups.clear();
    beatScoreAnnounced = false;
    coinSpinBoostTimer = 0.0f;
    gCoinSpinMultiplier = 1.0f;
    hudCoinAnimTimer = 0.0f;
    hudCoinFrame = 0;
    magnetActive = false;
    shieldActive = false;
    spreadActive = false;
    magnetTimer = shieldTimer = spreadTimer = 0.0f;
    powerups.clear();
    powerupSpawnTimer = 0.0f;
    powerupSpawnInterval = 10.0f;
}

static void GoToMenuFromGame() {
    gameState = GameState::Menu;
    playerDead = false;
    player.dead = false;
    waitingForStart = true;
    showFlyHint = true;
    optionsFromPaused = false;
    SDL_SetWindowMouseGrab(window, false);
    SDL_ShowCursor();
}

static void UpdateRecordsOnGameOver() {
    newRecord = false;
    bool recordChanged = false;
    if (score > bestScore) {
        bestScore = score;
        record.bestScore = score;
        recordChanged = true;
    }
    if (coinCount > bestCoins || (coinCount == bestCoins && gameTimer > bestTime)) {
        bestCoins = coinCount;
        bestTime = gameTimer;
        record.bestCoins = bestCoins;
        record.bestTime = bestTime;
        newRecord = true;
        recordChanged = true;
    }
    if (recordChanged) SaveRecord(record);
}


/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!InitNet()) {
        SDL_Log("Couldn't initialize ENet.");
    }
    mpLocalIp = GetLocalIp();

    if (!gSoundManager.Init()) {
        SDL_Log("Couldn't initialize audio: %s", SDL_GetError());
    }

    // Load your sounds
    gSoundManager.LoadSound("jump", "assets/sounds/jump.wav");
    gSoundManager.LoadSound("shoot", "assets/sounds/shoot.wav");
    gSoundManager.LoadSound("bat_hit", "assets/sounds/bat_hit.wav");
    gSoundManager.LoadSound("coin", "assets/sounds/coin.wav");
    gSoundManager.SetSfxVolume(sfxVolumeSetting);
    gSoundManager.PlayMusic("assets/sounds/bgMusic.wav", musicVolumeSetting);

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



    font = TTF_OpenFont("BoldPixels.ttf", 32);
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
    UpdateCachedText(menuTitleText, "Arr0wB1rd", white);
    UpdateCachedText(menuStartText, "START", white);
    UpdateCachedText(menuOptionsText, "OPTIONS", white);
    UpdateCachedText(menuMultiplayerText, "MULTIPLAYER", white);
    UpdateCachedText(menuQuitText, "QUIT", white);
    UpdateCachedText(overTitleText, "GAME OVER!", white);
    UpdateCachedText(newRecordText, "NEW RECORD!", white);
    UpdateCachedText(nicePopupText, "NICE!", white);
    UpdateCachedText(hudMenuText, "MENU", white);
    
    optionsStateText.text.clear(); // will be set on render
    hudLastCoins = -1;
    hudLastTime = -1;
    hudLastBestScore = -1;
    hudLastScore = -1;

    if (LoadRecord(record)) {
        bestScore = record.bestScore;
        bestCoins = record.bestCoins;
        bestTime = record.bestTime;
    } else {
        record = GameRecord{};
        bestScore = 0;
        bestCoins = 0;
        bestTime = 0.0f;
    }
    LoadSavedIps();




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

    magnetTex = IMG_LoadTexture(renderer, "assets/magnet.png");
    shieldTex = IMG_LoadTexture(renderer, "assets/shield.png");
    for (int i = 0; i < kShieldFrameCount; ++i) {
        char file[64];
        SDL_snprintf(file, sizeof(file), "assets/frames/frame%04d.png", i * 3 + 1);
        shieldFrames[i] = IMG_LoadTexture(renderer, file);
    }

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
    if (event->type == SDL_EVENT_TEXT_INPUT) {
        if (gameState == GameState::MultiplayerMenu || gameState == GameState::MultiplayerLobby) {
            const char *txt = event->text.text;
            if (mpNameActive && mpUserName.size() < (size_t)(kMpNameLen - 1)) {
                mpUserName += txt;
            } else if (mpIpActive && mpHostIp.size() < 64) {
                mpHostIp += txt;
            }
        }
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (gameState == GameState::MultiplayerMenu || gameState == GameState::MultiplayerLobby) {
            if (event->key.scancode == SDL_SCANCODE_BACKSPACE) {
                if (mpNameActive && !mpUserName.empty()) mpUserName.pop_back();
                if (mpIpActive && !mpHostIp.empty()) mpHostIp.pop_back();
            } else if (event->key.scancode == SDL_SCANCODE_TAB) {
                mpNameActive = !mpNameActive;
                mpIpActive = !mpIpActive;
            }
        }
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{

    frameCounter++;
    SDL_RenderClear(renderer);
    if (frameCounter % 4 == 0) {
        gSoundManager.Update();
    }

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
    bool f12Down = input.Down(SDL_SCANCODE_F12);
    if (f12Down && !wasF12Down) {
        bool target = !isFullscreen;
        if (SDL_SetWindowFullscreen(window, target) == 0) {
            isFullscreen = target;
        } else {
            SDL_Log("Fullscreen toggle failed: %s", SDL_GetError());
        }
        last = SDL_GetPerformanceCounter();
        dt = 0.0f;
    }
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
            if (optionsFromPaused) {
                gameState = GameState::Paused;
                SDL_ShowCursor();
                SDL_SetWindowMouseGrab(window, false);
            } else {
                gameState = GameState::Menu;
                SDL_ShowCursor();
                SDL_SetWindowMouseGrab(window, false);
            }
            gSoundManager.PlaySound("coin");
            optionsFromPaused = false;
            last = SDL_GetPerformanceCounter();
            dt = 0.0f;
        } else if (gameState == GameState::MultiplayerMenu || gameState == GameState::MultiplayerLobby) {
            NetDisconnect();
            gameState = GameState::Menu;
            SDL_StopTextInput(window);
            SDL_ShowCursor();
            SDL_SetWindowMouseGrab(window, false);
        } else if (gameState == GameState::MultiplayerPlaying || gameState == GameState::MultiplayerOver) {
            NetDisconnect();
            gameState = GameState::Menu;
            SDL_StopTextInput(window);
            SDL_ShowCursor();
            SDL_SetWindowMouseGrab(window, false);
        }
    }
    wasEscapeDown = escapeDown;
    wasF12Down = f12Down;

    NetPoll();

    if ((gameState == GameState::Playing || gameState == GameState::Paused || gameState == GameState::GameOver) &&
        (playerDead || player.dead || gameState == GameState::Paused ||
        (gameState == GameState::Options && optionsFromPaused))) {
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
    if (gameState == GameState::Menu || (gameState == GameState::Options && !optionsFromPaused)) {
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
        if (demoProjectiles.size() > 10) {
            demoProjectiles.erase(demoProjectiles.begin(), demoProjectiles.begin() + (demoProjectiles.size() - 10));
        }
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

    bool inMultiplayer = (gameState == GameState::MultiplayerPlaying || gameState == GameState::MultiplayerOver);
    if (gameState == GameState::MultiplayerPlaying) {
        MpInputState localInput;
        const bool *keys = SDL_GetKeyboardState(nullptr);
        localInput.buttons |= (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) ? kMpBtnLeft : 0;
        localInput.buttons |= (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) ? kMpBtnRight : 0;
        localInput.buttons |= (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_SPACE]) ? kMpBtnJump : 0;
        localInput.buttons |= (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) ? kMpBtnDown : 0;
        float mx = 0.0f, my = 0.0f;
        Uint32 mstate = 0;
        GetLogicalMouse(mx, my, mstate);
        localInput.aimX = mx;
        localInput.aimY = my;
        localInput.shoot = (mstate & SDL_BUTTON_LMASK) ? 1 : 0;

        if (mpLocalId >= 0 && mpLocalId < (int)mpInputs.size()) {
            mpInputs[mpLocalId] = localInput;
        }
        if (!netIsHost) {
            NetSendInput(mpLocalId, localInput);
        }
    }

    if (gameState == GameState::MultiplayerPlaying && netIsHost) {
        MpUpdateHost(dt);
        mpSnapshotTimer += dt;
        if (mpSnapshotTimer >= 0.05f) {
            mpSnapshotTimer = 0.0f;
            NetSendSnapshot();
        }
        if (mpMatchOver) {
            gameState = GameState::MultiplayerOver;
        }
    } else if (gameState == GameState::MultiplayerPlaying && !netIsHost) {
        MpUpdateClient(dt);
        if (mpMatchOver) {
            gameState = GameState::MultiplayerOver;
        }
    }
    if (inMultiplayer) {
        runningAnim.Update(dt);
        flyingAnim.Update(dt);
    }


    for (auto &ft : floatingTexts) ft.Update(dt);
    for (auto &ft : floatingTexts) ft.Render(renderer);
    floatingTexts.erase(std::remove_if(floatingTexts.begin(), floatingTexts.end(), [](const FloatingText &ft) { return !ft.alive; }), floatingTexts.end());
    for (auto &cp : comboPopups) cp.timer += dt;
    comboPopups.erase(std::remove_if(comboPopups.begin(), comboPopups.end(),
        [](const ComboPopup &cp){ return cp.timer >= cp.lifetime; }), comboPopups.end());
    for (auto &bp : beatScorePopups) bp.timer += dt;
    beatScorePopups.erase(std::remove_if(beatScorePopups.begin(), beatScorePopups.end(),
        [](const BeatScorePopup &bp){ return bp.timer >= bp.lifetime; }), beatScorePopups.end());

    if (IsPlayingActive()) {
        if (waitingForStart && flyKeyDown) {
            waitingForStart = false;
            showFlyHint = false;
            player.SetAutopilot(false);
            coinSpinBoostTimer = kCoinSpinBoostDuration;
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

    // Handle edge death transitioning to game over
    if (gameState == GameState::Playing && player.dead && !playerDead) {
        playerDead = true;
        comboCount = 0;
        comboTimer = 0.0f;
        UpdateComboText();
        gameState = GameState::GameOver;
        UpdateRecordsOnGameOver();
        SDL_SetWindowMouseGrab(window, false);
        SDL_ShowCursor();
    }

    if (IsPlayingActive() && !waitingForStart && comboCount > 0) {
        comboTimer -= dt;
        if (comboTimer <= 0.0f) {
            comboCount = 0;
            comboTimer = 0.0f;
            UpdateComboText();
        }
    }

    if (IsPlayingActive() && !waitingForStart) {
        if (coinSpinBoostTimer > 0.0f) {
            coinSpinBoostTimer -= dt;
            if (coinSpinBoostTimer < 0.0f) coinSpinBoostTimer = 0.0f;
            gCoinSpinMultiplier = 1.0f + 2.0f * (coinSpinBoostTimer / kCoinSpinBoostDuration);
        } else {
            gCoinSpinMultiplier = 1.0f;
        }
    } else if (gameState == GameState::GameOver) {
        gCoinSpinMultiplier = 1.0f;
    }
    if (gameState == GameState::Playing || gameState == GameState::GameOver) {
        hudCoinAnimTimer += dt * gCoinSpinMultiplier;
        const float frameTime = 0.1f;
        while (hudCoinAnimTimer >= frameTime) {
            hudCoinAnimTimer -= frameTime;
            hudCoinFrame = (hudCoinFrame + 1) % 4;
        }
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



            if (rng() % 8 == 0) {  // ~12.5% chance
                float platY = std::uniform_real_distribution<float>(100.0f, kWinH - 150.0f)(rng);
                float platSpeed = std::uniform_real_distribution<float>(80.0f, 140.0f)(rng);
                float amp = std::uniform_real_distribution<float>(20.0f, 40.0f)(rng);
                float freq = std::uniform_real_distribution<float>(1.5f, 3.0f)(rng);
                float phase = std::uniform_real_distribution<float>(0.0f, SDL_PI_F * 2.0f)(rng);
            }
            coinSpawnInterval = intervalDist(rng);

        }
    }

    // Powerup spawn
    if (IsPlayingActive() && !waitingForStart) {
        powerupSpawnTimer += dt;
        if (powerupSpawnTimer >= powerupSpawnInterval && powerups.size() < 2) {
            powerupSpawnTimer = 0.0f;
            powerupSpawnInterval = 8.0f + (rng() % 3000) / 1000.0f; // 8-11 sec
            PowerupType type = (rng() % 2 == 0) ? PowerupType::Magnet : PowerupType::Shield;
            if (rng() % 3 == 0) type = PowerupType::Spread; // occasionally spread
            float y = 50.0f + (rng() % (kWinH - 100));
            float x = kWinW + 60.0f; // spawn off-screen to the right, drift left
            float spd = std::uniform_real_distribution<float>(140.0f, 200.0f)(rng);
            float amp = std::uniform_real_distribution<float>(6.0f, 18.0f)(rng);
            float freq = std::uniform_real_distribution<float>(1.5f, 3.2f)(rng);
            float phase = std::uniform_real_distribution<float>(0.0f, SDL_PI_F * 2.0f)(rng);
            powerups.push_back({type, SDL_FPoint{ x, y }, true, 12.0f, spd, amp, freq, phase, 0.0f});
        }
    }

    // Update powerups
    if (!powerups.empty()) {
        for (auto &p : powerups) {
            p.ttl -= dt;
            p.t += dt;
            p.pos.x -= p.speed * dt;
            float bob = p.amp * SDL_sinf(p.freq * p.t + p.phase);
            p.pos.y += bob * dt;
            if (p.pos.x < -60.0f) p.alive = false;
            if (p.ttl <= 0.0f) p.alive = false;
        }
        powerups.erase(std::remove_if(powerups.begin(), powerups.end(), [](const Powerup &p){ return !p.alive; }), powerups.end());
    }

    // Active powerup timers
    if (magnetActive) {
        magnetTimer -= dt;
        if (magnetTimer <= 0.0f) { magnetActive = false; magnetTimer = 0.0f; }
    }
    if (shieldActive) {
        shieldTimer = 0.0f; // shield lasts until a hit consumes it
    }
    if (shieldActive && dt > 0.0f) {
        shieldAnimTimer += dt;
        while (shieldAnimTimer >= kShieldFrameTime) {
            shieldAnimTimer -= kShieldFrameTime;
            shieldAnimFrame = (shieldAnimFrame + 1) % kShieldFrameCount;
        }
    } else {
        shieldAnimFrame = 0;
        shieldAnimTimer = 0.0f;
    }
    if (spreadActive) {
        spreadTimer -= dt;
        if (spreadTimer <= 0.0f) { spreadActive = false; spreadTimer = 0.0f; }
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
            // ramp difficulty over time (more bats, shorter interval)
            float diff = ClampF(1.0f + gameTimer * 0.03f, 1.0f, 3.5f);
            float minInt = std::max(0.6f, 2.2f / diff);
            float maxInt = std::max(1.0f, 3.8f / diff);
            std::uniform_real_distribution<float> intervalDist(minInt, maxInt);
            batSpawnInterval = std::max(1.0f, intervalDist(rng));
        }
    }

    std::vector<SDL_FRect> warningIcons;

    if (IsPlayingActive() && !waitingForStart) {
        SDL_FPoint target = player.Center();
        for (auto &b : bats) {
            b.Update(dt, target);
        }
        bats.erase(std::remove_if(bats.begin(), bats.end(), [](const Bat &b){ return !b.alive; }), bats.end());

        // collect warnings for approaching off-screen bats
        float warnMargin = 18.0f;
        const float offscreenMargin = 32.0f; // small buffer to consider offscreen
        SDL_FRect screenRect{0.0f, 0.0f, (float)kWinW, (float)kWinH};
        for (auto &b : bats) {
            if (!b.alive || b.state != BatState::Moving) continue;
            SDL_FRect batBox = b.Bounds();
            if (AABBOverlap(batBox, screenRect)) continue; // already on screen

            bool offLeft = batBox.x + batBox.w < -offscreenMargin;
            bool offRight = batBox.x > kWinW + offscreenMargin;
            bool offTop = batBox.y + batBox.h < -offscreenMargin;
            bool offBot = batBox.y > kWinH + offscreenMargin;
            if (!(offLeft || offRight || offTop || offBot)) continue;

            bool headingIn = (offLeft && b.vel.x > 0.0f) || (offRight && b.vel.x < 0.0f) ||
                             (offTop && b.vel.y > 0.0f) || (offBot && b.vel.y < 0.0f);
            if (!headingIn) continue;

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
            gSoundManager.PlaySound("coin");
            ResetGameState();
            SDL_SetWindowMouseGrab(window, true);
            SDL_HideCursor();
        } else if (btnMultiplayer.hover) {
            gSoundManager.PlaySound("coin");
            gameState = GameState::MultiplayerMenu;
            mpNameActive = true;
            mpIpActive = false;
            SDL_StartTextInput(window);
            SDL_ShowCursor();
            SDL_SetWindowMouseGrab(window, false);
        } else if (btnOptions.hover) {
            gSoundManager.PlaySound("coin");
            optionsFromPaused = false;
            gameState = GameState::Options;
            SDL_ShowCursor();
            SDL_SetWindowMouseGrab(window, false);
        } else if (btnQuit.hover) {
            SDL_Event quitEvent;
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
        }
    }

    if ((gameState == GameState::MultiplayerMenu || gameState == GameState::MultiplayerLobby) && mouseDown && !wasMouseDown) {
        if (mx >= mpNameBox.x && mx <= mpNameBox.x + mpNameBox.w &&
            my >= mpNameBox.y && my <= mpNameBox.y + mpNameBox.h) {
            mpNameActive = true;
            mpIpActive = false;
        } else if (mx >= mpIpBox.x && mx <= mpIpBox.x + mpIpBox.w &&
                   my >= mpIpBox.y && my <= mpIpBox.y + mpIpBox.h) {
            mpNameActive = false;
            mpIpActive = true;
        } else if (mx >= mpHostBtn.x && mx <= mpHostBtn.x + mpHostBtn.w &&
                   my >= mpHostBtn.y && my <= mpHostBtn.y + mpHostBtn.h) {
            if (NetStartHost()) {
                gameState = GameState::MultiplayerPlaying;
                SDL_StopTextInput(window);
                SDL_SetWindowMouseGrab(window, true);
                SDL_HideCursor();
                mpSnapshotTimer = 0.0f;
                if (!mpHostIp.empty()) {
                    mpSavedIps.erase(std::remove(mpSavedIps.begin(), mpSavedIps.end(), mpHostIp), mpSavedIps.end());
                    mpSavedIps.push_front(mpHostIp);
                    while (mpSavedIps.size() > 5) mpSavedIps.pop_back();
                    SaveSavedIps();
                }
            }
        } else if (mx >= mpJoinBtn.x && mx <= mpJoinBtn.x + mpJoinBtn.w &&
                   my >= mpJoinBtn.y && my <= mpJoinBtn.y + mpJoinBtn.h) {
            if (NetStartClient(mpHostIp)) {
                gameState = GameState::MultiplayerLobby;
                SDL_StopTextInput(window);
                if (!mpHostIp.empty()) {
                    mpSavedIps.erase(std::remove(mpSavedIps.begin(), mpSavedIps.end(), mpHostIp), mpSavedIps.end());
                    mpSavedIps.push_front(mpHostIp);
                    while (mpSavedIps.size() > 5) mpSavedIps.pop_back();
                    SaveSavedIps();
                }
            }
        } else if (mx >= mpBackBtn.x && mx <= mpBackBtn.x + mpBackBtn.w &&
                   my >= mpBackBtn.y && my <= mpBackBtn.y + mpBackBtn.h) {
            NetDisconnect();
            gameState = GameState::Menu;
            SDL_StopTextInput(window);
        } else {
            for (int i = 0; i < 5; ++i) {
                const SDL_FRect &box = mpSavedBoxes[i];
                if (box.w <= 0.0f) continue;
                if (mx >= box.x && mx <= box.x + box.w &&
                    my >= box.y && my <= box.y + box.h) {
                    if (i < (int)mpSavedIps.size()) {
                        mpHostIp = mpSavedIps[i];
                        mpIpActive = true;
                        mpNameActive = false;
                    }
                }
            }
        }
    }

    if (gameState == GameState::Playing && mouseDown && !wasMouseDown) {
        if (hudMenuText.tex) {
            double tNow = (double)SDL_GetTicks() / 1000.0;
            float bob = SDL_sinf((float)tNow * 3.0f) * 2.5f;
            float x = 16.0f;
            float y = 54.0f + bob;
            if (mx >= x && mx <= x + hudMenuText.w && my >= y && my <= y + hudMenuText.h) {
                GoToMenuFromGame();
            }
        }
    }

    if (gameState == GameState::Paused && mouseDown && !wasMouseDown) {
        const char *optLabel = "OPTIONS";
        int lw = 0, lh = 0;
        float hitSpacing = 4.0f;
        if (TTF_GetStringSize(font, optLabel, SDL_strlen(optLabel), &lw, &lh)) {
            float totalW = (float)lw + hitSpacing * ((float)SDL_strlen(optLabel) - 1.0f);
            float maxH = 0.0f;
            for (int i = 0; i < 6; ++i) {
                if (pausedLetters[i].h > maxH) maxH = pausedLetters[i].h;
            }
            float baseY = (kWinH - maxH) * 0.5f;
            float textStart = kWinW * 0.5f - totalW * 0.5f;
            float baseTextY = baseY + maxH + 18.0f;
            float hitX = textStart;
            float hitY = baseTextY - 4.0f;
            float hitW = totalW;
            float hitH = lh + 8.0f;
            if (mx >= hitX && mx <= hitX + hitW && my >= hitY && my <= hitY + hitH) {
                optionsFromPaused = true;
                gameState = GameState::Options;
                SDL_ShowCursor();
                SDL_SetWindowMouseGrab(window, false);
            }
        }
    }

    if (gameState == GameState::Options) {
        float sliderW = 180.0f;
        float sliderX = kWinW * 0.5f - sliderW * 0.5f;
        float musicY = kWinH * 0.5f - 10.0f;
        float sfxY = musicY + 50.0f;

        auto adjustFromMouse = [&](float y, bool &dragging, float &value, auto setter) {
            float trackH = 16.0f;
            float hitTop = y - trackH;
            float hitBottom = y + trackH;
            bool inside = (mx >= sliderX && mx <= sliderX + sliderW && my >= hitTop && my <= hitBottom);
            if (mouseDown && !wasMouseDown && inside) dragging = true;
            if (dragging && mouseDown) {
                float t = ClampF((mx - sliderX) / sliderW, 0.0f, 1.0f);
                value = t;
                setter(t);
            }
        };

        adjustFromMouse(musicY, draggingMusicSlider, musicVolumeSetting, [&](float v){ gSoundManager.SetMusicVolume(v); });
        adjustFromMouse(sfxY, draggingSfxSlider, sfxVolumeSetting, [&](float v){ gSoundManager.SetSfxVolume(v); });

        if (!mouseDown) {
            draggingMusicSlider = false;
            draggingSfxSlider = false;
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
        if (projectiles.size() > 50) {
            projectiles.erase(projectiles.begin(), projectiles.begin() + (projectiles.size() - 50));
        }

        // Coin collection check
        SDL_FRect playerBox = player.Bounds();
        const float coinSize = 32.0f;
        for (auto &c : coins) {
            if (!c.alive) continue;

            if (magnetActive) {
                SDL_FPoint pc = player.Center();
                float dx = pc.x - c.pos.x;
                float dy = pc.y - c.pos.y;
                float dist2 = dx*dx + dy*dy;
                const float radius = 180.0f;
                if (dist2 < radius * radius) {
                    float dist = SDL_sqrtf(dist2);
                    if (dist < 1.0f) dist = 1.0f;
                    dx /= dist; dy /= dist;
                    float pull = 420.0f;
                    c.pos.x += dx * pull * dt;
                    c.pos.y += dy * pull * dt;
                }
            }

            SDL_FRect coinBox{ c.pos.x - coinSize * 0.5f, c.pos.y - coinSize * 0.5f, coinSize, coinSize };
            if (AABBOverlap(playerBox, coinBox)) {
                gSoundManager.PlaySound("coin");
                c.alive = false;
                coinCount++;
            }
        }

        SDL_FRect screenRect{0.0f, 0.0f, (float)kWinW, (float)kWinH};

        for (auto &p : projectiles) {
            if (!p.alive) continue;
            SDL_FRect projBox = p.Bounds();
            for (auto &b : bats) {
                if (!b.alive || b.state != BatState::Moving) continue;
                SDL_FRect batBox = b.Bounds();
                bool batOnScreen = AABBOverlap(batBox, screenRect);
                if (!batOnScreen) continue;
                if (AABBOverlap(projBox, batBox)) {
                    gSoundManager.PlaySound("bat_hit");
                    b.Kill();
                    p.alive = false;

                    // Perfect shot: projectile center near bat center
                    float projCx = projBox.x + projBox.w * 0.5f;
                    float projCy = projBox.y + projBox.h * 0.5f;
                    float batCx = batBox.x + batBox.w * 0.5f;
                    float batCy = batBox.y + batBox.h * 0.5f;
                    float dx = projCx - batCx;
                    float dy = projCy - batCy;
                    float tolX = batBox.w * 0.15f;
                    float tolY = batBox.h * 0.15f;
                    bool isPerfect = (SDL_fabsf(dx) <= tolX && SDL_fabsf(dy) <= tolY);

                    int points = (comboCount + 1 >= 10) ? 3 : 1;
                    if (comboCount + 1 >= 10) comboCount += 3;
                    else comboCount++;
                    if (isPerfect) points += 1;
                    score += points;
                    if (!beatScoreAnnounced && score > bestScore) {
                        bestScore = score;
                        record.bestScore = bestScore;
                        SaveRecord(record);
                        beatScoreAnnounced = true;
                        TriggerBeatScorePopup();
                    }
                    comboCount++;
                    comboTimer = comboTimeout;
                    UpdateComboText();
                    floatingTexts.emplace_back(b.pos.x, b.pos.y - 40.0f, "+" + std::to_string(points), SDL_Color{255, 255, 0, 255});
                    TriggerComboPopup();
                    if (isPerfect) {
                        perfectShot = true;
                        UpdatePerfectScoreText();
                        floatingTexts.emplace_back(b.pos.x, b.pos.y - 60.0f, "Perfect!", SDL_Color{255, 215, 0, 255});
                    } else {
                        perfectShot = false;
                    }
                    break;
                }
            }
        }

        // Bat vs player
        for (auto &b : bats) {
            if (!b.alive || b.state != BatState::Moving) continue;
            SDL_FRect batBox = b.Bounds();
            bool batOnScreen = AABBOverlap(batBox, screenRect);
            if (!batOnScreen) continue;
            if (AABBOverlap(GetBirdBatHitbox(playerBox), batBox)) {
                if (shieldActive) {
                    gSoundManager.PlaySound("bat_hit");
                    b.Kill();
                    comboCount++;
                    comboTimer = comboTimeout;
                    UpdateComboText();
                    TriggerComboPopup();
                    shieldActive = false;
                    shieldTimer = 0.0f;
                    continue;
                }
                gSoundManager.PlaySound("bat_hit");
                player.dead = true;
                playerDead = true;
                comboCount = 0;
                comboTimer = 0.0f;
                UpdateComboText();
                gameState = GameState::GameOver;
                // update records
                UpdateRecordsOnGameOver();
                SDL_SetWindowMouseGrab(window, false);
                SDL_ShowCursor();
                break;
            }
        }

        // Powerup collection
        for (auto &p : powerups) {
            if (!p.alive) continue;
            float bob = p.amp * SDL_sinf(p.freq * p.t + p.phase);
            float sz = 26.0f;
            SDL_FRect puBox{ p.pos.x - sz * 0.5f, p.pos.y - sz * 0.5f + bob, sz, sz };
            if (AABBOverlap(playerBox, puBox)) {
                if (p.type == PowerupType::Magnet) {
                    magnetActive = true;
                    magnetTimer = 10.0f;
                } else if (p.type == PowerupType::Shield) {
                    shieldActive = true;
                    shieldTimer = 0.0f;
                } else if (p.type == PowerupType::Spread) {
                    spreadActive = true;
                    spreadTimer = 8.0f;
                }
                p.alive = false;
            }
        }
        powerups.erase(std::remove_if(powerups.begin(), powerups.end(), [](const Powerup &p){ return !p.alive; }), powerups.end());
    }





    // then draw
    if (gameState == GameState::Playing || gameState == GameState::Paused || gameState == GameState::GameOver) {
        player.Render(renderer);

        // Shield animation overlay
        if (shieldActive && shieldFrames[shieldAnimFrame]) {
            SDL_FRect pbox = player.Bounds();
            float sz = pbox.w + 18.0f;
            SDL_FRect sdst{ pbox.x + pbox.w * 0.5f - sz * 0.5f, pbox.y + pbox.h * 0.5f - sz * 0.5f, sz, sz };
            SDL_RenderTexture(renderer, shieldFrames[shieldAnimFrame], nullptr, &sdst);
        }

        for (auto &b : bats) {
            b.Render(renderer);
        }

        for (auto &p : projectiles) {
            p.Render(renderer);
        }

        for (auto &c : coins) {
            c.Render(renderer);
        }

        // Powerups
        for (auto &p : powerups) {
            if (!p.alive) continue;
            SDL_Texture *tex = nullptr;
            if (p.type == PowerupType::Magnet) tex = magnetTex;
            else if (p.type == PowerupType::Shield) tex = shieldTex;
            else if (p.type == PowerupType::Spread) tex = magnetTex;
            float sz = 26.0f;
            float bob = p.amp * SDL_sinf(p.freq * p.t + p.phase);
            SDL_FRect dst{ p.pos.x - sz * 0.5f, p.pos.y - sz * 0.5f + bob, sz, sz };
            if (tex) {
                if (p.type == PowerupType::Spread) SDL_SetTextureColorMod(tex, 64, 200, 255);
                SDL_RenderTexture(renderer, tex, nullptr, &dst);
                if (p.type == PowerupType::Spread) SDL_SetTextureColorMod(tex, 255, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderFillRect(renderer, &dst);
            }
        }
    }

    // Warning icons for incoming bats off-screen
    if (warningFrames[warningFrame]) {
        for (const auto &wicon : warningIcons) {
            SDL_RenderTexture(renderer, warningFrames[warningFrame], nullptr, &wicon);
        }
    }

    if (inMultiplayer) {
        for (auto &b : mpBats) {
            b.Render(renderer);
        }
        for (auto &p : mpProjectiles) {
            p.Render(renderer);
        }
        MpRenderPlayers();
        MpRenderKillPopups();
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
    if ((gameState == GameState::Playing || gameState == GameState::MultiplayerPlaying) && cursorTex) {
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
        std::string bestLabel = "Best " + std::to_string(bestScore);
        std::string scoreLabel = "Score " + std::to_string(score);
        SDL_Color white{255,255,255,255};
        if (coinCount != hudLastCoins) {
            hudLastCoins = coinCount;
            UpdateCachedText(hudCoinText, coinLabel, white);
        }
        if (totalSeconds != hudLastTime) {
            hudLastTime = totalSeconds;
            UpdateCachedText(hudTimeText, timeLabel, white);
        }
        if (bestScore != hudLastBestScore) {
            hudLastBestScore = bestScore;
            UpdateCachedText(hudBestScoreText, bestLabel, white);
        }
        if (score != hudLastScore) {
            hudLastScore = score;
            UpdateCachedText(hudScoreText, scoreLabel, white);
        }
        float margin = 16.0f;
        double hudT = (double)SDL_GetTicks() / 1000.0;
        float bob = SDL_sinf((float)hudT * 3.0f) * 2.5f;
        if (hudBestScoreText.tex) {
            float bestX = margin;
            SDL_FRect shadow{ bestX + 2.0f, 10.0f + bob + 2.0f, hudBestScoreText.w, hudBestScoreText.h };
            SDL_SetTextureColorMod(hudBestScoreText.tex, 0,0,0);
            SDL_SetTextureAlphaMod(hudBestScoreText.tex, 160);
            SDL_RenderTexture(renderer, hudBestScoreText.tex, nullptr, &shadow);
            SDL_SetTextureColorMod(hudBestScoreText.tex, 255,255,255);
            SDL_SetTextureAlphaMod(hudBestScoreText.tex, 255);
            SDL_FRect dst{ bestX, 10.0f + bob, hudBestScoreText.w, hudBestScoreText.h };
            SDL_RenderTexture(renderer, hudBestScoreText.tex, nullptr, &dst);
        }
        if (hudScoreText.tex) {
            float scoreX = margin;
            float scoreY = 32.0f + bob;
            RenderCachedTextShadowed(hudScoreText, scoreX, scoreY);
        }
        if (hudMenuText.tex) {
            float hmx = 0.0f, hmy = 0.0f;
            Uint32 hstate = 0;
            GetLogicalMouse(hmx, hmy, hstate);
            float menuX = margin;
            float menuY = 54.0f + bob;
            bool hover = (hmx >= menuX && hmx <= menuX + hudMenuText.w &&
                          hmy >= menuY && hmy <= menuY + hudMenuText.h);
            Uint8 alpha = hover ? 255 : 200;
            SDL_FRect shadow{ menuX + 2.0f, menuY + 2.0f, hudMenuText.w, hudMenuText.h };
            SDL_SetTextureColorMod(hudMenuText.tex, 0,0,0);
            SDL_SetTextureAlphaMod(hudMenuText.tex, 160);
            SDL_RenderTexture(renderer, hudMenuText.tex, nullptr, &shadow);
            SDL_SetTextureColorMod(hudMenuText.tex, 255,255,255);
            SDL_SetTextureAlphaMod(hudMenuText.tex, alpha);
            SDL_FRect dst{ menuX, menuY, hudMenuText.w, hudMenuText.h };
            SDL_RenderTexture(renderer, hudMenuText.tex, nullptr, &dst);
            SDL_SetTextureAlphaMod(hudMenuText.tex, 255);
        }
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
            SDL_Texture *cTex = coinFrames[hudCoinFrame];
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
        // Combo HUD: always visible. Wave after 3, rainbow after 5, per-letter wave.
        {
            static std::string lastComboStr;
            static std::vector<CachedText> comboLetterCache;

            std::string comboStr = "Combo " + std::to_string(comboCount);
            if (comboStr != lastComboStr) {
                for (auto &c : comboLetterCache) DestroyCached(c);
                comboLetterCache.clear();
                comboLetterCache.resize(comboStr.size());
                lastComboStr = comboStr;
            }

            double t = (double)SDL_GetTicks() / 1000.0;
            float baseX = (float)kWinW - margin;
            float baseY = 54.0f + bob;
            float spacing = 1.0f;
            float waveAmp = (comboCount >= 3) ? ((comboCount >= 5) ? 5.0f : 3.0f) : 0.0f;
            float waveFreq = 3.5f;

            // measure to right-align
            float totalW = 0.0f;
            for (size_t i = 0; i < comboStr.size(); ++i) {
                char cstr[2] = {comboStr[i], '\0'};
                SDL_Color col{255,255,255,255};
                if (comboCount >= 5) {
                    col = RainbowColor((float)t, 20.0f * (float)i);
                }
                UpdateCachedText(comboLetterCache[i], cstr, col);
                totalW += comboLetterCache[i].w + spacing;
            }
            float startX = baseX - totalW;

            float x = startX;
            for (size_t i = 0; i < comboStr.size(); ++i) {
                auto &ct = comboLetterCache[i];
                if (!ct.tex) continue;
                float waveY = (waveAmp > 0.0f) ? SDL_sinf((float)t * waveFreq + 0.5f * (float)i) * waveAmp : 0.0f;
                SDL_FRect shadow{ x + 2.0f, baseY + waveY + 2.0f, ct.w, ct.h };
                SDL_SetTextureColorMod(ct.tex, 0,0,0);
                SDL_SetTextureAlphaMod(ct.tex, 160);
                SDL_RenderTexture(renderer, ct.tex, nullptr, &shadow);
                SDL_SetTextureColorMod(ct.tex, 255,255,255);
                SDL_SetTextureAlphaMod(ct.tex, 255);
                SDL_FRect dst{ x, baseY + waveY, ct.w, ct.h };
                SDL_RenderTexture(renderer, ct.tex, nullptr, &dst);
                x += ct.w + spacing;
            }
        }

        if (nicePopupText.tex && !comboPopups.empty()) {
            double tNow = (double)SDL_GetTicks() / 1000.0;
            for (const auto &cp : comboPopups) {
                const float scale = 0.7f;
                float tNorm = cp.timer / cp.lifetime;
                float rise = 18.0f;
                float y = cp.pos.y - rise * tNorm;
                Uint8 alpha = 255;
                if (tNorm > 0.7f) {
                    float fade = (1.0f - (tNorm - 0.7f) / 0.3f);
                    if (fade < 0.0f) fade = 0.0f;
                    alpha = (Uint8)SDL_roundf(255.0f * fade);
                }
                SDL_Color col = RainbowColor((float)tNow, cp.timer * 60.0f);
                SDL_SetTextureColorMod(nicePopupText.tex, 0, 0, 0);
                SDL_SetTextureAlphaMod(nicePopupText.tex, (Uint8)SDL_roundf(alpha * 0.6f));
                SDL_FRect shadow{ cp.pos.x + 2.0f, y + 2.0f, nicePopupText.w * scale, nicePopupText.h * scale };
                SDL_RenderTexture(renderer, nicePopupText.tex, nullptr, &shadow);
                SDL_SetTextureColorMod(nicePopupText.tex, col.r, col.g, col.b);
                SDL_SetTextureAlphaMod(nicePopupText.tex, alpha);
                SDL_FRect dst{ cp.pos.x, y, nicePopupText.w * scale, nicePopupText.h * scale };
                SDL_RenderTexture(renderer, nicePopupText.tex, nullptr, &dst);
            }
            SDL_SetTextureColorMod(nicePopupText.tex, 255, 255, 255);
            SDL_SetTextureAlphaMod(nicePopupText.tex, 255);
        }

        if (!beatScorePopups.empty()) {
            for (const auto &bp : beatScorePopups) {
                RenderBeatScorePopup(bp);
            }
        }

        if (showFlyHint && font) {
            const char *hint = "Press W or SPACE to fly";
            const char *edgeWarn = "Stay away from the screen edges!";
            const float baseY = kWinH * 0.7f;
            const float amp = 8.0f;
            const float freq = 2.2f;
            double tNow = (double)SDL_GetTicks() / 1000.0;

            SDL_Surface *s = TTF_RenderText_Blended(font, hint, 0, SDL_Color{255,255,255,255});
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
            // Edge warning just below the fly hint
            SDL_Surface *wSurf = TTF_RenderText_Blended(font, edgeWarn, 0, SDL_Color{255, 120, 120, 255});
            if (wSurf) {
                SDL_Texture *wTex = SDL_CreateTextureFromSurface(renderer, wSurf);
                if (wTex) {
                    float waveY = SDL_sinf((float)(tNow * 2.5f)) * 6.0f;
                    float x = kWinW * 0.5f - wSurf->w * 0.5f;
                    float y = baseY + 30.0f + waveY;
                    SDL_FRect shadow{ x + 3.0f, y + 3.0f, (float)wSurf->w, (float)wSurf->h };
                    SDL_SetTextureColorMod(wTex, 0,0,0);
                    SDL_SetTextureAlphaMod(wTex, 180);
                    SDL_RenderTexture(renderer, wTex, nullptr, &shadow);
                    SDL_SetTextureColorMod(wTex, 255,255,255);
                    SDL_SetTextureAlphaMod(wTex, 255);
                    SDL_FRect dst{ x, y, (float)wSurf->w, (float)wSurf->h };
                    SDL_RenderTexture(renderer, wTex, nullptr, &dst);
                    SDL_DestroyTexture(wTex);
                }
                SDL_DestroySurface(wSurf);
            }
        }

        // Powerup icons/timers
        float puY = 70.0f;
        float iconSize = 22.0f;
        if (magnetActive && magnetTex) {
            SDL_FRect dst{ 14.0f, puY, iconSize, iconSize };
            SDL_RenderTexture(renderer, magnetTex, nullptr, &dst);
            if (magnetTimer > 0.0f) {
                char buf[16];
                SDL_snprintf(buf, sizeof(buf), "%.1fs", magnetTimer);
                RenderLabel(buf, dst.x + iconSize + 8.0f, dst.y + 2.0f);
            }
            puY += iconSize + 6.0f;
        }
        if (shieldActive && shieldTex) {
            SDL_FRect dst{ 14.0f, puY, iconSize, iconSize };
            SDL_RenderTexture(renderer, shieldTex, nullptr, &dst);
            if (shieldTimer > 0.0f) {
                char buf[16];
                SDL_snprintf(buf, sizeof(buf), "%.1fs", shieldTimer);
                RenderLabel(buf, dst.x + iconSize + 8.0f, dst.y + 2.0f);
            }
            puY += iconSize + 6.0f;
        }
        if (spreadActive && magnetTex) {
            SDL_FRect dst{ 14.0f, puY, iconSize, iconSize };
            SDL_SetTextureColorMod(magnetTex, 64, 200, 255);
            SDL_RenderTexture(renderer, magnetTex, nullptr, &dst);
            SDL_SetTextureColorMod(magnetTex, 255, 255, 255);
            puY += iconSize + 6.0f;
        }
    }

    if (gameState == GameState::MultiplayerPlaying || gameState == GameState::MultiplayerOver) {
        MpRenderHUD();
        MpRenderPing();
        if (gameState == GameState::MultiplayerOver) {
            MpRenderMatchOver();
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
        // Options text while paused with same wave feel as PAUSED
        const char *optLabel = "OPTIONS";
        int lw = 0, lh = 0;
        float optSpacing = 4.0f;
        if (TTF_GetStringSize(font, optLabel, SDL_strlen(optLabel), &lw, &lh)) {
            float totalW = (float)lw + optSpacing * ((float)SDL_strlen(optLabel) - 1.0f);
            float textStart = kWinW * 0.5f - totalW * 0.5f;
            float baseTextY = baseY + maxH + 18.0f;
            float hitX = textStart;
            float hitY = baseTextY - 4.0f;
            float hitW = totalW;
            float hitH = lh + 8.0f;
            bool optHover = (mx >= hitX && mx <= hitX + hitW && my >= hitY && my <= hitY + hitH);
            Uint8 mainAlpha = optHover ? 255 : 180;
            Uint8 shadowAlpha = optHover ? 220 : 160;
            float tx = textStart;
            float waveAmp = pausedWaveAmp;
            float waveFreq = pausedWaveFreq;
            for (size_t i = 0; i < SDL_strlen(optLabel); ++i) {
                char cbuf[2] = {optLabel[i], '\0'};
                SDL_Color shadow{0,0,0,shadowAlpha};
                SDL_Color mainCol{255,255,255,mainAlpha};
                SDL_Surface *ss = TTF_RenderText_Blended(font, cbuf, 0, shadow);
                SDL_Surface *sm = TTF_RenderText_Blended(font, cbuf, 0, mainCol);
                if (!sm) { if (ss) SDL_DestroySurface(ss); continue; }
                float waveY = SDL_sinf((float)(t * waveFreq + pausedWavePhaseStep * (float)i)) * waveAmp;
                float w = (float)sm->w;
                float h = (float)sm->h;
                SDL_Texture *ts = ss ? SDL_CreateTextureFromSurface(renderer, ss) : nullptr;
                SDL_Texture *tm = SDL_CreateTextureFromSurface(renderer, sm);
                if (ts) {
                    SDL_FRect dst{ tx + 2.0f, baseTextY + waveY + 2.0f, w, h };
                    SDL_RenderTexture(renderer, ts, nullptr, &dst);
                }
                if (tm) {
                    SDL_FRect dst{ tx, baseTextY + waveY, w, h };
                    SDL_RenderTexture(renderer, tm, nullptr, &dst);
                }
                if (ts) SDL_DestroyTexture(ts);
                if (tm) SDL_DestroyTexture(tm);
                SDL_DestroySurface(sm);
                if (ss) SDL_DestroySurface(ss);
                tx += w + optSpacing;
            }
        }
    }

    // Menu
    if (gameState == GameState::Menu || (gameState == GameState::Options && !optionsFromPaused)) {
        double t = (double)SDL_GetTicks() / 1000.0;
        RenderWaveTitle("Arr0wb1rd", kWinW * 0.5f, 46.0f);

        // buttons with sine wobble and hover blink (only interactable in menu)
        btnStart.bounds.w = menuStartText.w;
        btnStart.bounds.h = menuStartText.h;
        btnStart.bounds.x = kWinW * 0.5f - btnStart.bounds.w * 0.5f;
        btnStart.bounds.y = 120.0f;

        btnMultiplayer.bounds.w = menuMultiplayerText.w;
        btnMultiplayer.bounds.h = menuMultiplayerText.h;
        btnMultiplayer.bounds.x = kWinW * 0.5f - btnMultiplayer.bounds.w * 0.5f;
        btnMultiplayer.bounds.y = 160.0f;

        btnOptions.bounds.w = menuOptionsText.w;
        btnOptions.bounds.h = menuOptionsText.h;
        btnOptions.bounds.x = kWinW * 0.5f - btnOptions.bounds.w * 0.5f;
        btnOptions.bounds.y = 200.0f;
        
        btnQuit.bounds.w = menuQuitText.w;
        btnQuit.bounds.h = menuQuitText.h;
        btnQuit.bounds.x = kWinW * 0.5f - btnQuit.bounds.w * 0.5f;
        btnQuit.bounds.y = 240.0f;

        Uint32 mstate = SDL_GetMouseState(nullptr, nullptr);
        float mx=0.0f, my=0.0f; GetLogicalMouse(mx,my);
        auto hoverCheck = [&](MenuButton &b){
            b.hover = (mx >= b.bounds.x && mx <= b.bounds.x + b.bounds.w &&
                       my >= b.bounds.y && my <= b.bounds.y + b.bounds.h);
        };
        if (gameState == GameState::Menu) {
            hoverCheck(btnStart);
            hoverCheck(btnMultiplayer);
            hoverCheck(btnOptions);
            hoverCheck(btnQuit);
        } else {
            btnStart.hover = btnMultiplayer.hover = btnOptions.hover = btnQuit.hover = false;
        }

        auto renderButton = [&](const MenuButton &b, float phase){
            float hoverWave = b.hover ? SDL_sinf((float)t * 4.0f + phase) * 4.0f : 0.0f;
            Uint8 alpha = b.hover ? (Uint8)(200 + 55 * (0.5f + 0.5f * SDL_sinf((float)t * 6.0f + phase))) : 255;
            CachedText *ct = nullptr;
            if (b.label == "START") ct = &menuStartText;
            else if (b.label == "MULTIPLAYER") ct = &menuMultiplayerText;
            else if (b.label == "OPTIONS") ct = &menuOptionsText;
            else if (b.label == "QUIT") ct = &menuQuitText;
            if (ct && ct->tex) {
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
        if (gameState == GameState::Menu) {
            renderButton(btnStart, 0.0f);
            renderButton(btnMultiplayer, 0.6f);
            renderButton(btnOptions, 1.0f);
            renderButton(btnQuit, 1.6f);
        }

        // demo player + arrow + demo shots
        if (gameState == GameState::Menu || (gameState == GameState::Options && !optionsFromPaused)) {
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
    }

    // Multiplayer menu / lobby
    if (gameState == GameState::MultiplayerMenu || gameState == GameState::MultiplayerLobby) {
        float centerX = kWinW * 0.5f;
        const float mpScale = 0.75f;
        const float mpHeaderScale = 0.9f;
        RenderLabelScaled("MULTIPLAYER", centerX - 110.0f, 40.0f, mpHeaderScale);

        mpNameBox = SDL_FRect{ centerX - 150.0f, 110.0f, 300.0f, 36.0f };
        mpIpBox = SDL_FRect{ centerX - 150.0f, 164.0f, 300.0f, 36.0f };
        mpHostBtn = SDL_FRect{ centerX - 150.0f, 210.0f, 140.0f, 30.0f };
        mpJoinBtn = SDL_FRect{ centerX + 10.0f, 210.0f, 140.0f, 30.0f };
        mpBackBtn = SDL_FRect{ centerX - 60.0f, 260.0f, 120.0f, 26.0f };

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);
        SDL_RenderFillRect(renderer, &mpNameBox);
        SDL_RenderFillRect(renderer, &mpIpBox);

        SDL_SetRenderDrawColor(renderer, mpNameActive ? 255 : 140, mpNameActive ? 255 : 140, mpNameActive ? 255 : 140, 255);
        SDL_RenderRect(renderer, &mpNameBox);
        SDL_SetRenderDrawColor(renderer, mpIpActive ? 255 : 140, mpIpActive ? 255 : 140, mpIpActive ? 255 : 140, 255);
        SDL_RenderRect(renderer, &mpIpBox);

        RenderLabelScaled("Name", mpNameBox.x - 70.0f, mpNameBox.y + 8.0f, mpScale);
        RenderLabelScaled("Join IP", mpIpBox.x - 90.0f, mpIpBox.y + 8.0f, mpScale);
        RenderLabelScaled(mpUserName, mpNameBox.x + 8.0f, mpNameBox.y + 10.0f, mpScale);
        RenderLabelScaled(mpHostIp, mpIpBox.x + 8.0f, mpIpBox.y + 10.0f, mpScale);

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 200);
        SDL_RenderFillRect(renderer, &mpHostBtn);
        SDL_RenderFillRect(renderer, &mpJoinBtn);
        SDL_RenderFillRect(renderer, &mpBackBtn);
        RenderLabelScaled("HOST", mpHostBtn.x + 42.0f, mpHostBtn.y + 6.0f, mpScale);
        RenderLabelScaled("JOIN", mpJoinBtn.x + 44.0f, mpJoinBtn.y + 6.0f, mpScale);
        RenderLabelScaled("BACK", mpBackBtn.x + 38.0f, mpBackBtn.y + 5.0f, mpScale);

        float listY = 320.0f;
        RenderLabelScaled("Saved IPs", centerX - 80.0f, listY, mpScale);
        listY += 26.0f;
        int idx = 0;
        for (const auto &ip : mpSavedIps) {
            if (idx >= 5) break;
            mpSavedBoxes[idx] = SDL_FRect{ centerX - 120.0f, listY, 240.0f, 22.0f };
            SDL_SetRenderDrawColor(renderer, 20, 20, 20, 180);
            SDL_RenderFillRect(renderer, &mpSavedBoxes[idx]);
            RenderLabelScaled(ip, mpSavedBoxes[idx].x + 6.0f, mpSavedBoxes[idx].y + 3.0f, mpScale);
            listY += 26.0f;
            idx++;
        }
        for (; idx < 5; ++idx) {
            mpSavedBoxes[idx] = SDL_FRect{ 0,0,0,0 };
        }

        if (gameState == GameState::MultiplayerLobby) {
            RenderLabelScaled(netIsHost ? "Waiting for players..." : "Connecting...", centerX - 150.0f, 430.0f, mpScale);
        }
        RenderLabelScaled("Your IP (share): " + mpLocalIp, centerX - 150.0f, 460.0f, mpScale);
    }

    // Options with volume sliders
    if (gameState == GameState::Options) {
        SDL_Color white{255,255,255,255};
        UpdateCachedText(optionsStateText, "Adjust volumes", white);
        RenderLabel("OPTIONS", kWinW * 0.5f - 70.0f, kWinH * 0.5f - 110.0f);
        float sliderW = 180.0f;
        float sliderX = kWinW * 0.5f - sliderW * 0.5f;
        float musicY = kWinH * 0.5f - 6.0f;
        float sfxY = musicY + 48.0f;

        auto drawSlider = [&](const char *label, float value, float y) {
            RenderLabel(label, sliderX - 110.0f, y - 10.0f);
            SDL_FRect track{ sliderX, y, sliderW, 10.0f };
            SDL_SetRenderDrawColor(renderer, 20, 20, 30, 200);
            SDL_RenderFillRect(renderer, &track);
            SDL_FRect fill{ sliderX, y, sliderW * value, 10.0f };
            SDL_SetRenderDrawColor(renderer, 90, 180, 255, 230);
            SDL_RenderFillRect(renderer, &fill);
            float knobW = 14.0f;
            SDL_FRect knob{ sliderX + sliderW * value - knobW * 0.5f, y - 2.0f, knobW, 14.0f };
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &knob);
            char pct[16];
            SDL_snprintf(pct, sizeof(pct), "%3d%%", (int)SDL_roundf(value * 100.0f));
            RenderLabel(pct, sliderX + sliderW + 14.0f, y - 6.0f);
        };

        drawSlider("Music", musicVolumeSetting, musicY);
        drawSlider("FX", sfxVolumeSetting, sfxY);

        if (optionsStateText.tex) {
            SDL_FRect shadow{ kWinW * 0.5f - optionsStateText.w * 0.5f + 2.0f, kWinH * 0.5f - 52.0f + 2.0f, optionsStateText.w, optionsStateText.h };
            SDL_SetTextureColorMod(optionsStateText.tex, 0,0,0);
            SDL_SetTextureAlphaMod(optionsStateText.tex, 200);
            SDL_RenderTexture(renderer, optionsStateText.tex, nullptr, &shadow);
            SDL_SetTextureColorMod(optionsStateText.tex, 255,255,255);
            SDL_SetTextureAlphaMod(optionsStateText.tex, 255);
            SDL_FRect dst{ kWinW * 0.5f - optionsStateText.w * 0.5f, kWinH * 0.5f - 52.0f, optionsStateText.w, optionsStateText.h };
            SDL_RenderTexture(renderer, optionsStateText.tex, nullptr, &dst);
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        RenderLabel("ESC to return", kWinW * 0.5f - 90.0f, kWinH * 0.5f + 120.0f);
    }

    // Game over overlay
    if (gameState == GameState::GameOver) {
        int totalSeconds = (int)gameTimer;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        char timeBuf[8];
        SDL_snprintf(timeBuf, sizeof(timeBuf), "%02d %02d", minutes, seconds);
        SDL_Color white{255,255,255,255};
        double t = (double)SDL_GetTicks() / 1000.0;
        float centerX = kWinW * 0.5f;
        float y = kWinH * 0.5f - 150.0f;
        const float bobAmp = 6.0f;
        const float bobFreq = 2.8f;

        UpdateCachedText(overTitleText, "GAME OVER!", white);
        UpdateCachedText(overCoinsText, "Coins " + std::to_string(coinCount), white);
        UpdateCachedText(overScoreText, "Score " + std::to_string(score), white);
        UpdateCachedText(overTimeText, std::string("Time ") + timeBuf, white);
        UpdateCachedText(overBestScoreText, "Best " + std::to_string(bestScore), white);
        UpdateCachedText(newRecordText, "NEW RECORD!", white);
        UpdateCachedText(overRestartText, "ESC TO RESTART", white);

        auto bobY = [&](float phase) {
            return SDL_sinf((float)(t * bobFreq + phase)) * bobAmp;
        };

        if (overTitleText.tex) {
            float yOff = bobY(0.0f);
            float x = centerX - overTitleText.w * 0.5f;
            RenderCachedTextShadowed(overTitleText, x, y + yOff);
        }
        y += 42.0f;

        {
            float yOff = bobY(0.6f);
            float scale = 0.8f;
            float iconSize = 28.0f * scale;
            float gap = 6.0f;
            float blockGap = 18.0f;
            float lineW = iconSize + gap;
            if (overCoinsText.tex) lineW += overCoinsText.w * scale;
            if (overScoreText.tex) lineW += blockGap + overScoreText.w * scale;
            if (overTimeText.tex) lineW += blockGap + overTimeText.w * scale;
            float startX = centerX - lineW * 0.5f;
            float lineH = iconSize;
            if (overCoinsText.tex) lineH = SDL_max(lineH, overCoinsText.h * scale);
            if (overScoreText.tex) lineH = SDL_max(lineH, overScoreText.h * scale);
            if (overTimeText.tex) lineH = SDL_max(lineH, overTimeText.h * scale);

            float padX = 10.0f;
            float padY = 6.0f;
            SDL_FRect bg{ startX - padX, y + yOff - padY, lineW + padX * 2.0f, lineH + padY * 2.0f };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 77);
            SDL_RenderFillRect(renderer, &bg);

            SDL_Texture *cTex = coinFrames[hudCoinFrame];
            if (cTex) {
                float textH = overCoinsText.tex ? overCoinsText.h * scale : 0.0f;
                SDL_FRect cdst{ startX, y + yOff + (textH - iconSize) * 0.5f, iconSize, iconSize };
                SDL_RenderTexture(renderer, cTex, nullptr, &cdst);
            }
            float cursorX = startX + iconSize + gap;
            if (overCoinsText.tex) {
                RenderCachedTextShadowedScaled(overCoinsText, cursorX, y + yOff, scale);
                cursorX += overCoinsText.w * scale + blockGap;
            }
            if (overScoreText.tex) {
                RenderCachedTextShadowedScaled(overScoreText, cursorX, y + yOff, scale);
                cursorX += overScoreText.w * scale + blockGap;
            }
            if (overTimeText.tex) {
                RenderCachedTextShadowedScaled(overTimeText, cursorX, y + yOff, scale);
            }
        }
        y += 42.0f;
        if (overBestScoreText.tex) {
            float yOff = bobY(2.4f);
            float x = centerX - overBestScoreText.w * 0.5f;
            RenderCachedTextShadowed(overBestScoreText, x, y + yOff);
        }
        y += 40.0f;

        if (newRecord && newRecordText.tex) {
            float yOff = bobY(3.0f);
            float x = centerX - newRecordText.w * 0.5f;
            RenderCachedTextShadowed(newRecordText, x, y + yOff);
            y += 36.0f;
        }

        if (overRestartText.tex) {
            float yOff = bobY(3.6f);
            float x = centerX - overRestartText.w * 0.5f;
            RenderCachedTextShadowed(overRestartText, x, y + yOff);
        }
    }





    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
    ShutdownNet();

    if (bgTexture) {
        SDL_DestroyTexture(bgTexture);
        bgTexture = NULL;
    }
    if (psTextureL1) { SDL_DestroyTexture(psTextureL1); psTextureL1 = NULL; }
    if (psTextureL2) { SDL_DestroyTexture(psTextureL2); psTextureL2 = NULL; }
    if (psTextureL3) { SDL_DestroyTexture(psTextureL3); psTextureL3 = NULL; }
    if (psTextureL4) { SDL_DestroyTexture(psTextureL4); psTextureL4 = NULL; }
    if (psTextureL5) { SDL_DestroyTexture(psTextureL5); psTextureL5 = NULL; }
    if (magnetTex) { SDL_DestroyTexture(magnetTex); magnetTex = nullptr; }
    if (shieldTex) { SDL_DestroyTexture(shieldTex); shieldTex = nullptr; }
    for (int i = 0; i < kShieldFrameCount; ++i) {
        if (shieldFrames[i]) {
            SDL_DestroyTexture(shieldFrames[i]);
            shieldFrames[i] = nullptr;
        }
    }

    runningAnim.DestroyFrames();
    flyingAnim.DestroyFrames();


    if (textTex) { SDL_DestroyTexture(textTex); textTex = nullptr; }
    DestroyCached(hudCoinText);
    DestroyCached(hudTimeText);
    DestroyCached(hudBestScoreText);
    DestroyCached(hudScoreText);
    DestroyCached(menuTitleText);
    DestroyCached(menuStartText);
    DestroyCached(menuOptionsText);
    DestroyCached(menuMultiplayerText);
    DestroyCached(menuQuitText);
    DestroyCached(optionsStateText);
    DestroyCached(overTitleText);
    DestroyCached(overCoinsText);
    DestroyCached(overScoreText);
    DestroyCached(overTimeText);
    DestroyCached(overBestScoreText);
    DestroyCached(overRestartText);
    DestroyCached(hudComboText);
    DestroyCached(nicePopupText);
    DestroyCached(hudMenuText);
    if (arrowTex) { SDL_DestroyTexture(arrowTex); arrowTex = nullptr; }
    if (cursorTex) { SDL_DestroyTexture(cursorTex); cursorTex = nullptr; }
    for (auto &pl : pausedLetters) {
        if (pl.tex) { SDL_DestroyTexture(pl.tex); pl.tex = nullptr; }
    }
    ClearWaveCache();
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
    DestroyCached(hudBestScoreText);
    DestroyCached(hudScoreText);
    DestroyCached(menuTitleText);
    DestroyCached(menuStartText);
    DestroyCached(menuOptionsText);
    DestroyCached(menuMultiplayerText);
    DestroyCached(menuQuitText);
    DestroyCached(optionsStateText);
    DestroyCached(overTitleText);
    DestroyCached(overCoinsText);
    DestroyCached(overScoreText);
    DestroyCached(overTimeText);
    DestroyCached(overBestScoreText);
    DestroyCached(overRestartText);
    DestroyCached(newRecordText);
    DestroyCached(hudMenuText);

    if (font) { TTF_CloseFont(font); font = nullptr; }
    for (int i = 0; i < 30; ++i) {
    if (projectileFrames[i]) {
        SDL_DestroyTexture(projectileFrames[i]);
        projectileFrames[i] = nullptr;
    }
    }
    ClearWaveTextCache(overCoinsWave);
    ClearWaveTextCache(overScoreWave);
    ClearWaveTextCache(overTimeWave);
    ClearWaveTextCache(overBestScoreWave);
    ClearWaveTextCache(overRestartWave);
    ClearWaveTextCache(overTitleWave);
    ClearWaveTextCache(overNewRecordWave);
    for (auto &c : beatScoreLetterCache) {
        if (c.tex) SDL_DestroyTexture(c.tex);
    }
    beatScoreLetterCache.clear();
    cachedBeatScoreLabel.clear();
    TTF_Quit();
    SDL_Quit();
}
