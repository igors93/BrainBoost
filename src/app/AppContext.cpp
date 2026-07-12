#include "app/AppContext.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <limits>

namespace {

void applyDefaultSystemName(UserProfile& profile) {
    const char* user = std::getenv("USER");
    if (user == nullptr || user[0] == '\0') user = std::getenv("USERNAME");
    if (user == nullptr || user[0] == '\0') return;

    profile.name = user;
    profile.name[0] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(profile.name[0])));
}

}  // namespace

void AppContext::loadProgress() {
    const SaveLoadResult result = saveManager.loadDetailed(profile, stats);
    lastLoadStatus = result.status;
    persistenceReadOnly = false;
    lastSaveSucceeded = true;
    lastSaveError.clear();

    switch (result.status) {
        case SaveLoadStatus::Success:
            return;

        case SaveLoadStatus::FileNotFound:
            profile.reset();
            stats.resetAll();
            applyDefaultSystemName(profile);
            return;

        case SaveLoadStatus::UnsupportedVersion:
            profile.reset();
            stats.resetAll();
            persistenceReadOnly = true;
            lastSaveSucceeded = false;
            lastSaveError =
                "O progresso foi criado por uma versão mais nova e não será sobrescrito.";
            return;

        case SaveLoadStatus::IoError:
            profile.reset();
            stats.resetAll();
            persistenceReadOnly = true;
            lastSaveSucceeded = false;
            lastSaveError =
                "O arquivo de progresso não pôde ser lido e foi protegido contra sobrescrita.";
            return;

        case SaveLoadStatus::Corrupted:
            profile.reset();
            stats.resetAll();
            applyDefaultSystemName(profile);
            lastSaveSucceeded = false;
            lastSaveError =
                "O progresso estava corrompido. O arquivo anterior será preservado como backup.";
            return;
    }
}

bool AppContext::saveProgress() {
    if (persistenceReadOnly) {
        lastSaveSucceeded = false;
        if (lastSaveError.empty()) {
            lastSaveError = "O progresso está em modo somente leitura e não foi sobrescrito.";
        }
        return false;
    }

    if (saveManager.save(profile, stats)) {
        lastSaveSucceeded = true;
        lastSaveError.clear();
        lastLoadStatus = SaveLoadStatus::Success;
        return true;
    }

    lastSaveSucceeded = false;
    lastSaveError = "Erro ao salvar o progresso.";
    return false;
}

void AppContext::startGame(const GameInfo& info) {
    if (!info.isImplemented()) return;

    activeGame = info.factory();
    activeGameId = info.id;
    activeGameStartedAt = std::chrono::steady_clock::now();
    resultApplied = false;
    lastUnlocks.clear();
    screen = ScreenId::Playing;
}

void AppContext::applyResultOnce() {
    const GameInfo* info = registry.findById(activeGameId);
    if (!activeGame || resultApplied || info == nullptr) return;

    GameResult result = activeGame->result();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                             std::chrono::steady_clock::now() - activeGameStartedAt)
                             .count();
    const auto boundedElapsed = std::clamp<long long>(
        elapsed, 0, std::numeric_limits<int>::max());
    result.durationSeconds =
        std::max(result.durationSeconds, static_cast<int>(boundedElapsed));

    profile.addXp(result.xpEarned);
    profile.registerPlayToday();

    const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
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
