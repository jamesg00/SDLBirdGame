# SDLBirdGame

An arcade-y side scroller built with SDL3: flap to hover, shoot at bats, dodge threats, and collect coins while a parallax background scrolls by.

## Gameplay
- Press `W` or `Space` to start and flap; move with `A/D` or arrows.
- Aim with the mouse; left-click to shoot once play has started.
- Collect coins, survive bats, and set a best time/coin record.
- ESC pauses/unpauses; ESC on Game Over restarts.

## Visuals & FX
- Layered parallax sky/trees/mountains.
- Sprite animations for running/flying, bats, coins, and projectiles.
- HUD with animated coin icon, time counter, and drop-shadowed text.
- Menu demo bird and warning indicators for off-screen bats.

## Build/Run
- Requires SDL3, SDL3_image, SDL3_ttf (paths configured in the compile command you're using).
- Example (adjust include/lib paths for your setup):

```ps1
g++ -O2 -s main.cpp core/utils.cpp core/parallax.cpp player/player.cpp player/bat.cpp audio/audio.cpp core/floating_text.cpp collectables/coin.cpp core/sprite_anim.cpp core/projectile.cpp records/record.cpp -o MusiccGame.exe -I "C:\Users\deadt\Downloads\SDL\x86_64-w64-mingw32\include" -I "C:\Users\deadt\Downloads\SDL3_image-devel-3.2.4-mingw\SDL3_image-3.2.4\x86_64-w64-mingw32\include" -L "C:\Users\deadt\Downloads\SDL\x86_64-w64-mingw32\lib" -L "C:\Users\deadt\Downloads\SDL3_image-devel-3.2.4-mingw\SDL3_image-3.2.4\x86_64-w64-mingw32\lib" -lSDL3_image -lSDL3 -lSDL3_ttf
```

- Run `MusiccGame.exe` from the `MusiccGame` folder so assets load correctly.
- Best score is saved to `records/record.txt`.

<img width="446" height="471" alt="image" src="https://github.com/user-attachments/assets/1df86764-a5a0-4709-b88d-f4e7311b3175" />
<img width="446" height="449" alt="image" src="https://github.com/user-attachments/assets/774b1ab8-ebc6-4dbd-8168-049a97e4acd6" />
