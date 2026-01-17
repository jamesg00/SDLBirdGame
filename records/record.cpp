#include "record.h"

#include <fstream>
#include <string>

static const char *kRecordPath = "records/record.txt";

static bool ParseInt(const std::string &line, const char *prefix, int &outVal) {
    if (line.rfind(prefix, 0) != 0) return false;
    try {
        outVal = std::stoi(line.substr(std::string(prefix).size()));
        return true;
    } catch (...) {
        return false;
    }
}

static bool ParseFloat(const std::string &line, const char *prefix, float &outVal) {
    if (line.rfind(prefix, 0) != 0) return false;
    try {
        outVal = std::stof(line.substr(std::string(prefix).size()));
        return true;
    } catch (...) {
        return false;
    }
}

bool LoadRecord(GameRecord &outRecord) {
    std::ifstream in(kRecordPath);
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        ParseInt(line, "best_score=", outRecord.bestScore);
        ParseInt(line, "best_coins=", outRecord.bestCoins);
        ParseFloat(line, "best_time=", outRecord.bestTime);
    }
    return true;
}

bool SaveRecord(const GameRecord &record) {
    std::ofstream out(kRecordPath, std::ios::trunc);
    if (!out) return false;

    out << "best_score=" << record.bestScore << "\n";
    out << "best_coins=" << record.bestCoins << "\n";
    out << "best_time=" << record.bestTime << "\n";
    return true;
}
