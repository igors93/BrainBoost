#pragma once

#include <cstdint>
#include <string>
#include "core/GameCategory.h"

struct SessionRecord {
    std::string gameId;
    GameCategory category = GameCategory::Memory;
    std::int64_t timestamp = 0;
    
    int score = 0;
    int correct = 0;
    int total = 0;
    int difficulty = 0;
    int durationSeconds = 0;
};
