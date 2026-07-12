#pragma once

#include <string>
#include <vector>

#include "core/GameInfo.h"

// Catalog of every game shown in the UI. To add a new game:
//   1. Create a class deriving from Game (include/games + src/games).
//   2. Register it in GameRegistry.cpp with a factory lambda.
// Everything else (cards, stats, XP) picks it up automatically.
class GameRegistry {
public:
    GameRegistry();

    const std::vector<GameInfo>& games() const { return games_; }
    const GameInfo* findById(const std::string& id) const;

private:
    std::vector<GameInfo> games_;
};
