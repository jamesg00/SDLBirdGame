# SDLBirdGame

SDLBirdGame is an arcade side-scroller built with SDL3. You control a bird that hovers, dodges threats, and shoots bats while a parallax world scrolls by. The game focuses on fast reactions, aim control, and chaining combos for higher scores.

## Gameplay Overview
- You start in a menu with a demo bird, then press `W` or `Space` to begin.
- The bird can flap to stay airborne and move horizontally to avoid hazards.
- You aim with the mouse and shoot projectiles to defeat bats.
- Coins spawn in different patterns and reward careful movement.
- Combo count increases as you land hits without timing out.
- Score increases based on hits and combo thresholds.
- Best score/coins/time are saved between runs.

## Controls
- `W` or `Space`: flap / start the run
- `A/D` or Left/Right arrows: move horizontally
- Mouse move: aim
- Left mouse button: shoot
- `ESC`: pause / unpause
- `ESC` on Game Over: restart

## Scoring & Records
- Each bat hit adds score, with bonus points at higher combos.
- Perfect hits (near center of target) add extra points.
- Best score is tracked and saved to disk in `records/record.txt`.
- Best coins and best time are tracked as records too.

## Visuals & Feedback
- Layered parallax background (sky, clouds, mountains, trees).
- Animated sprites for the bird, bats, coins, shields, and projectiles.
- HUD with animated coin icon, time counter, score, and combo display.
- “NICE!” popup when combo reaches the threshold.
- “Beat Score!” popup with rainbow wave when a new best score is reached.
- Game Over screen shows coins, score, time, and best score with a subtle bob animation.

## Powerups
- Magnet: attracts coins for a short duration.
- Shield: blocks a hit once; can also grant combo.
- Spread: changes projectile behavior for a short duration.

## Build / Run
Requires SDL3, SDL3_image, SDL3_ttf. Adjust paths as needed.

```ps1
g++ -O2 -s main.cpp core/utils.cpp core/parallax.cpp player/player.cpp player/bat.cpp audio/audio.cpp core/floating_text.cpp collectables/coin.cpp core/sprite_anim.cpp core/projectile.cpp records/record.cpp -o MusiccGame.exe -I "C:\Users\deadt\Downloads\SDL\x86_64-w64-mingw32\include" -I "C:\Users\deadt\Downloads\SDL3_image-devel-3.2.4-mingw\SDL3_image-3.2.4\x86_64-w64-mingw32\include" -L "C:\Users\deadt\Downloads\SDL\x86_64-w64-mingw32\lib" -L "C:\Users\deadt\Downloads\SDL3_image-devel-3.2.4-mingw\SDL3_image-3.2.4\x86_64-w64-mingw32\lib" -lSDL3_image -lSDL3 -lSDL3_ttf
```

Run `MusiccGame.exe` from the `MusiccGame` folder so assets load correctly.

## Screenshots
<img width="446" height="471" alt="image" src="https://github.com/user-attachments/assets/1df86764-a5a0-4709-b88d-f4e7311b3175" />
<img width="446" height="449" alt="image" src="https://github.com/user-attachments/assets/774b1ab8-ebc6-4dbd-8168-049a97e4acd6" />
