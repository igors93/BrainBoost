#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "core/Achievements.h"
#include "core/GameRegistry.h"
#include "core/SaveManager.h"
#include "core/Statistics.h"
#include "core/UserProfile.h"
#include "games/Game.h"
#include "ui/ScreenId.h"

// Shared application state passed to every screen. Owns the player data,
// the game catalog and the currently running game session.
struct AppContext {
    UserProfile profile;
    Statistics stats;
    GameRegistry registry;
    SaveManager saveManager{"save/brainboost_save.ini"};

    ScreenId screen = ScreenId::Home;
    bool quitRequested = false;

    // Active game session (only valid while screen == ScreenId::Playing).
    std::unique_ptr<Game> activeGame;
    std::string activeGameId;
    bool resultApplied = false;
    std::vector<const AchievementDef*> lastUnlocks;
    std::chrono::steady_clock::time_point activeGameStartedAt{};

    // Persistence state. Unsupported or unreadable saves are opened read-only
    // so a newer/unknown file is never overwritten on shutdown.
    SaveLoadStatus lastLoadStatus = SaveLoadStatus::FileNotFound;
    bool persistenceReadOnly = false;
    std::string lastSaveError;
    bool lastSaveSucceeded = true;

    void loadProgress();
    bool saveProgress();

    // Starts a session of `info` and switches to the Playing screen.
    void startGame(const GameInfo& info);

    // Applies the finished game's result exactly once: XP, streak,
    // statistics, achievements, and persists everything.
    void applyResultOnce();

    // Abandons/ends the current session and returns to `fallback`.
    void closeGame(ScreenId fallback = ScreenId::Home);
};
