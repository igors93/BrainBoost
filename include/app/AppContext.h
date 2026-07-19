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
    // Application::init() replaces this with the stable user-directory path
    // (see SavePaths) before loadProgress() runs; tests inject temp paths.
    SaveManager saveManager;

    ScreenId screen = ScreenId::Home;
    bool quitRequested = false;

    // Active game session (only valid while screen == ScreenId::Playing).
    std::unique_ptr<Game> activeGame;
    std::string activeGameId;
    bool resultApplied = false;
    std::vector<AchievementUnlockResult> lastUnlocks;
    std::chrono::steady_clock::time_point activeGameStartedAt{};

    // Persistence state. Unsupported or unreadable saves are opened read-only
    // so a newer/unknown file is never overwritten on shutdown.
    SaveLoadStatus lastLoadStatus = SaveLoadStatus::FileNotFound;
    bool persistenceReadOnly = false;
    std::string lastSaveError;
    bool lastSaveSucceeded = true;

    // True only after loadProgress() ran. saveProgress() refuses to write
    // before that, so an init failure can never replace an existing save
    // with a default profile.
    bool progressLoaded = false;

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
