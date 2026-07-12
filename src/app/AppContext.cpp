#include "app/AppContext.h"

#include <cctype>
#include <cstdlib>

void AppContext::loadProgress() {
    if (saveManager.load(profile, stats)) return;

    // First run: suggest a name based on the system user.
    if (const char* user = std::getenv("USER"); user != nullptr && user[0] != '\0') {
        profile.name = user;
        profile.name[0] =
            static_cast<char>(std::toupper(static_cast<unsigned char>(profile.name[0])));
    }
}

void AppContext::saveProgress() { saveManager.save(profile, stats); }

void AppContext::startGame(const GameInfo& info) {
    if (!info.isImplemented()) return;

    activeGame = info.factory();
    activeGameInfo = &info;
    resultApplied = false;
    lastUnlocks.clear();
    screen = ScreenId::Playing;
}

void AppContext::applyResultOnce() {
    if (!activeGame || resultApplied || activeGameInfo == nullptr) return;

    const GameResult result = activeGame->result();
    profile.addXp(result.xpEarned);
    profile.registerPlayToday();
    stats.recordResult(activeGameInfo->category, result);
    lastUnlocks = Achievements::evaluate(profile, stats);
    resultApplied = true;

    saveProgress();
}

void AppContext::closeGame(ScreenId fallback) {
    activeGame.reset();
    activeGameInfo = nullptr;
    resultApplied = false;
    screen = fallback;
}
