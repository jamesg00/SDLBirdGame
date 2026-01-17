#pragma once

struct GameRecord {
    int bestScore = 0;
    int bestCoins = 0;
    float bestTime = 0.0f;
};

bool LoadRecord(GameRecord &outRecord);
bool SaveRecord(const GameRecord &record);
