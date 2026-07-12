#include "app/AppContext.h"

#include <cctype>
#include <cstdlib>
#include <ctime>

void AppContext::loadProgress() {
    if (saveManager.load(profile, stats)) return;

    // First run: suggest a name based on the system user.
    if (const char* user = std::getenv("USER"); user != nullptr && user[0] != '\0') {
        profile.name = user;
        profile.name[0] =
            static_cast<char>(std::toupper(static_cast<unsigned char>(profile.name[0])));
    }
}

bool AppContext::saveProgress() {
    if (saveManager.save(profile, stats)) {
        lastSaveSucceeded = true;
        lastSaveError.clear();
        return true;
    } else {
        lastSaveSucceeded = false;
        lastSaveError = "Erro ao salvar o progresso.";
        return false;
    }
}

void AppContext::startGame(const GameInfo& info) {
    if (!info.isImplemented()) return;

    activeGame = info.factory();
    activeGameId = info.id;
    resultApplied = false;
    lastUnlocks.clear();
    screen = ScreenId::Playing;
}

void AppContext::applyResultOnce() {
    const GameInfo* info = registry.findById(activeGameId);
    if (!activeGame || resultApplied || info == nullptr) return;

    const GameResult result = activeGame->result();
    profile.addXp(result.xpEarned);
    profile.registerPlayToday();
    
    std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
    stats.recordResult(info->id, info->category, result, now);
    
    lastUnlocks = Achievements::evaluate(profile, stats);
    resultApplied = true;

    saveProgress();
}

void AppContext::closeGame(ScreenId fallback) {
    activeGame.reset();
    activeGameId.clear();
    resultApplied = false;
    screen = fallback;
}
