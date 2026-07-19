#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../TestUtils.h"
#include "app/AppContext.h"

namespace {
namespace fs = std::filesystem;

fs::path uniqueTestDirectory() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return fs::temp_directory_path() /
           ("brainboost_app_context_" + std::to_string(stamp));
}

std::string readAll(const fs::path& path) {
    std::ifstream file(path);
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

void testActiveGameMetadataSurvivesTemporaryCatalogCopy() {
    AppContext context;
    {
        auto games = context.registry.games();
        context.startGame(games.front());
    }
    TEST_CHECK(context.activeGame != nullptr);
    TEST_CHECK(!context.activeGameId.empty());
    TEST_CHECK(context.registry.findById(context.activeGameId) != nullptr);
}

void testUnsupportedSaveIsNeverOverwritten() {
    const fs::path directory = uniqueTestDirectory();
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    const std::string original = "save.version=999\nprofile.xp=9999\n";
    { std::ofstream file(savePath); file << original; }

    AppContext context;
    context.saveManager = SaveManager(savePath.string());
    context.loadProgress();
    TEST_CHECK(context.lastLoadStatus == SaveLoadStatus::UnsupportedVersion);
    TEST_CHECK(context.persistenceReadOnly);
    TEST_CHECK(!context.saveProgress());
    TEST_CHECK(readAll(savePath) == original);
    fs::remove_all(directory);
}

// The original bug: Application::shutdown() saved unconditionally, so an
// init failure before loadProgress() (SDL, window, renderer or fonts)
// replaced a valid save with a default profile. saveProgress() must refuse
// to write while progressLoaded is false and leave the file byte-identical.
void testSaveBeforeLoadNeverTouchesTheSaveFile() {
    const fs::path directory = uniqueTestDirectory();
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    {
        UserProfile profile;
        Statistics stats;
        profile.addXp(1234);
        TEST_CHECK(SaveManager(savePath.string()).save(profile, stats));
    }
    const std::string original = readAll(savePath);

    AppContext context;
    context.saveManager = SaveManager(savePath.string());
    // Simulates Application::shutdown() after init() failed before
    // loadProgress() ran (exactly what main() does on init failure).
    TEST_CHECK(!context.progressLoaded);
    TEST_CHECK(!context.saveProgress());
    TEST_CHECK(readAll(savePath) == original);
    TEST_CHECK(!fs::exists(savePath.string() + ".tmp"));
    TEST_CHECK(!fs::exists(savePath.string() + ".bak"));

    // After a successful load the same context saves normally again.
    context.loadProgress();
    TEST_CHECK(context.progressLoaded);
    TEST_CHECK(context.profile.xp() == 1234);
    TEST_CHECK(context.saveProgress());
    fs::remove_all(directory);
}

void testRecoveredBackupRemainsWritable() {
    const fs::path directory = uniqueTestDirectory();
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    SaveManager manager(savePath.string());
    UserProfile first;
    Statistics stats;
    first.addXp(100);
    TEST_CHECK(manager.save(first, stats));
    UserProfile second;
    second.addXp(200);
    TEST_CHECK(manager.save(second, stats));
    { std::ofstream file(savePath, std::ios::trunc); file << "save.version=bad\n"; }

    AppContext context;
    context.saveManager = manager;
    context.loadProgress();
    TEST_CHECK(context.lastLoadStatus == SaveLoadStatus::RecoveredFromBackup);
    TEST_CHECK(!context.persistenceReadOnly);
    TEST_CHECK(context.profile.xp() == 100);
    context.profile.addXp(25);
    TEST_CHECK(context.saveProgress());
    fs::remove_all(directory);
}

// Corrupted main + future-version backup: the whole session must stay
// read-only — saving after load, finishing games and the shutdown save all
// refuse, and both files stay byte-identical.
void testProtectedBackupBlocksAllWrites() {
    const fs::path directory = uniqueTestDirectory();
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    const fs::path backupPath = savePath.string() + ".bak";
    const std::string corruptedMain = "save.version=2\n";
    const std::string futureBackup = "save.version=999\nprofile.xp=9999\n";
    { std::ofstream file(savePath); file << corruptedMain; }
    { std::ofstream file(backupPath); file << futureBackup; }

    AppContext context;
    context.saveManager = SaveManager(savePath.string());
    context.loadProgress();
    TEST_CHECK(context.lastLoadStatus == SaveLoadStatus::BackupProtected);
    TEST_CHECK(context.persistenceReadOnly);
    TEST_CHECK(!context.lastSaveSucceeded);
    TEST_CHECK(!context.lastSaveError.empty());  // situation is reported

    // Saving right after the load refuses.
    TEST_CHECK(!context.saveProgress());

    // Finishing a game also cannot write.
    {
        auto games = context.registry.games();
        context.startGame(games.front());
    }
    context.applyResultOnce();
    TEST_CHECK(!context.lastSaveSucceeded);

    // The shutdown-time save refuses as well.
    TEST_CHECK(!context.saveProgress());

    TEST_CHECK(readAll(savePath) == corruptedMain);
    TEST_CHECK(readAll(backupPath) == futureBackup);
    fs::remove_all(directory);
}

// The result screen reads context.lastUnlocks: entries must carry whether
// the XP was actually granted, and the profile XP delta must match exactly
// what the screen reports.
void testResultScreenDataMatchesGrantedXp() {
    const fs::path directory = uniqueTestDirectory();
    fs::create_directories(directory);
    AppContext context;
    context.saveManager = SaveManager((directory / "save.ini").string());
    context.loadProgress();

    const GameInfo front = context.registry.games().front();
    GameResult seeded;
    seeded.score = 60;
    for (int i = 0; i < 4; ++i) {
        context.stats.recordResult(front.id, front.category, seeded, 1000 + i);
    }

    // 5th session: real unlocks, every reported entry granted XP.
    context.startGame(front);
    context.applyResultOnce();
    TEST_CHECK(!context.lastUnlocks.empty());
    for (const AchievementUnlockResult& unlock : context.lastUnlocks) {
        TEST_CHECK(unlock.firstUnlock);
        TEST_CHECK(unlock.rewardGranted);
        TEST_CHECK(unlock.xpGranted == unlock.achievement->xpReward);
    }
    context.closeGame();

    // After a partial reset the achievements reappear without XP, and the
    // profile only gains the game XP — never a hidden achievement reward.
    context.profile.resetAchievementsOnly();
    context.startGame(front);
    const int xpBefore = context.profile.xp();
    const int gameXp = context.activeGame->result().xpEarned;
    context.applyResultOnce();
    TEST_CHECK(!context.lastUnlocks.empty());
    for (const AchievementUnlockResult& unlock : context.lastUnlocks) {
        TEST_CHECK(!unlock.firstUnlock);
        TEST_CHECK(!unlock.rewardGranted);
        TEST_CHECK(unlock.xpGranted == 0);
    }
    TEST_CHECK(context.profile.xp() == xpBefore + gameXp);
    fs::remove_all(directory);
}

}  // namespace

int main() {
    std::cout << "Running AppContextTest...\n";
    testActiveGameMetadataSurvivesTemporaryCatalogCopy();
    testUnsupportedSaveIsNeverOverwritten();
    testSaveBeforeLoadNeverTouchesTheSaveFile();
    testProtectedBackupBlocksAllWrites();
    testResultScreenDataMatchesGrantedXp();
    testRecoveredBackupRemainsWritable();
    std::cout << "All AppContext tests passed!\n";
    return 0;
}
