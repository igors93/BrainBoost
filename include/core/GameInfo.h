#pragma once

#include <functional>
#include <memory>
#include <string>

#include "core/GameCategory.h"

class Game;

// Static metadata describing a game available in the catalog.
// `factory` is empty for games that are not implemented yet ("Em breve").
struct GameInfo {
    std::string id;           // stable identifier used in save files
    std::string title;        // display title (pt-BR)
    std::string description;  // short description shown on the card
    GameCategory category = GameCategory::Memory;
    unsigned int color = 0x1F2937;  // card background tint, 0xRRGGBB
    // Builds a session at `startingDifficulty` (the game's persisted
    // GameStats::difficultyLevel, rounded — see AppContext::startGame).
    std::function<std::unique_ptr<Game>(int startingDifficulty)> factory;

    bool isImplemented() const { return static_cast<bool>(factory); }
};
