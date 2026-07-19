#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "../TestUtils.h"
#include "core/Achievements.h"
#include "core/GameResult.h"
#include "core/SaveManager.h"
#include "core/Statistics.h"
#include "core/UserProfile.h"

namespace {
namespace fs = std::filesystem;

fs::path uniqueTestDirectory(const char* name) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return fs::temp_directory_path() /
           (std::string("brainboost_ach_") + name + "_" + std::to_string(stamp));
}

// Mirrors what AppContext::applyResultOnce does when a session finishes:
// game XP, statistics and then the achievement evaluation.
std::vector<AchievementUnlockResult> finishOneGame(UserProfile& profile,
                                                   Statistics& stats,
                                                   int gameXp) {
    GameResult result;
    result.score = 60;
    result.xpEarned = gameXp;
    profile.addXp(result.xpEarned);
    stats.recordResult("number_memory", GameCategory::Memory, result, 1000);
    return Achievements::evaluate(profile, stats);
}

// Writes a structurally valid v1 save the way the old version would have
// left it: XP kept, achievements list possibly cleared by a partial reset.
void writeV1SaveFile(const fs::path& path, int xp,
                     const std::string& achievementsCsv, int totalGames,
                     int memoryGames) {
    std::ofstream file(path, std::ios::trunc);
    file << "save.version=1\n"
         << "profile.name=Igor\n"
         << "profile.xp=" << xp << "\n"
         << "profile.streak=0\n"
         << "profile.last_played_day=0\n"
         << "profile.achievements=" << achievementsCsv << "\n"
         << "stats.total_games=" << totalGames << "\n"
         << "stats.history=\n";
    for (int index = 0; index < kCategoryCount; ++index) {
        const int games = index == 0 ? memoryGames : 0;
        file << "stats.category." << index << ".skill=50.000000\n"
             << "stats.category." << index << ".games=" << games << "\n";
    }
}

int rewardOf(const std::string& id) {
    for (const AchievementDef& def : Achievements::all()) {
        if (def.id == id) return def.xpReward;
    }
    return -1;
}

// The original bug: "Zerar apenas conquistas" cleared the unlocked ids while
// XP and statistics stayed, so the next finished game unlocked the same
// achievements again and granted their XP again, forever.
void testResetAchievementsThenPlayNeverRegrantsXp() {
    UserProfile profile;
    Statistics stats;

    // 5 memory games unlock both "first_steps" (5 games) and
    // "memory_in_action" (3 memory sessions).
    for (int i = 0; i < 5; ++i) finishOneGame(profile, stats, 10);
    TEST_CHECK(profile.hasAchievement("first_steps"));
    TEST_CHECK(profile.hasReceivedReward("first_steps"));
    TEST_CHECK(profile.xp() ==
               5 * 10 + rewardOf("first_steps") + rewardOf("memory_in_action"));

    const int xpAfterReward = profile.xp();
    for (int round = 0; round < 4; ++round) {
        profile.resetAchievementsOnly();
        TEST_CHECK(!profile.hasAchievement("first_steps"));

        finishOneGame(profile, stats, 0);  // conditions are still satisfied
        TEST_CHECK(profile.hasAchievement("first_steps"));  // re-unlocked...
        TEST_CHECK(profile.xp() == xpAfterReward);          // ...without XP
    }
}

void testLedgerSurvivesSaveAndReload() {
    const fs::path directory = uniqueTestDirectory("persist");
    const fs::path savePath = directory / "save.ini";
    SaveManager manager(savePath.string());

    UserProfile profile;
    Statistics stats;
    for (int i = 0; i < 5; ++i) finishOneGame(profile, stats, 10);
    profile.resetAchievementsOnly();
    TEST_CHECK(manager.save(profile, stats));

    // Simulate a program restart: reload and finish another game.
    UserProfile reloaded;
    Statistics reloadedStats;
    TEST_CHECK(manager.load(reloaded, reloadedStats));
    TEST_CHECK(reloaded.hasReceivedReward("first_steps"));

    const int xpBefore = reloaded.xp();
    finishOneGame(reloaded, reloadedStats, 0);
    TEST_CHECK(reloaded.hasAchievement("first_steps"));
    TEST_CHECK(reloaded.xp() == xpBefore);
    fs::remove_all(directory);
}

// A v1 save granted XP at unlock time, so migrating it must mark every
// unlocked achievement as already rewarded instead of re-granting them.
void testV1MigrationDoesNotRegrantRewards() {
    const fs::path directory = uniqueTestDirectory("migration");
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    writeV1SaveFile(savePath, 150, "first_steps", 5, 1);

    UserProfile profile;
    Statistics stats;
    SaveManager manager(savePath.string());
    TEST_CHECK(manager.load(profile, stats));
    TEST_CHECK(profile.hasReceivedReward("first_steps"));

    const int xpBefore = profile.xp();
    profile.resetAchievementsOnly();
    finishOneGame(profile, stats, 0);
    TEST_CHECK(profile.xp() == xpBefore);
    fs::remove_all(directory);
}

// The uncovered v1 case: "reset achievements only" cleared the list but the
// XP stayed. The migration must treat satisfied conditions as rewarded even
// with an empty list, otherwise the old XP is granted a second time.
void testV1MigrationCoversClearedAchievementsList() {
    const fs::path directory = uniqueTestDirectory("migration_cleared");
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    // 5 games total and 3 memory sessions: first_steps and memory_in_action
    // were both earned (and rewarded) in v1 before the list was cleared.
    writeV1SaveFile(savePath, 400, "", 5, 3);

    UserProfile profile;
    Statistics stats;
    SaveManager manager(savePath.string());
    TEST_CHECK(manager.load(profile, stats));
    TEST_CHECK(profile.achievements().empty());
    TEST_CHECK(profile.hasReceivedReward("first_steps"));
    TEST_CHECK(profile.hasReceivedReward("memory_in_action"));
    TEST_CHECK(!profile.hasReceivedReward("veteran"));  // never satisfied

    // Next game re-unlocks both achievements without granting XP again.
    const int xpBefore = profile.xp();
    const auto unlocks = finishOneGame(profile, stats, 0);
    TEST_CHECK(unlocks.size() == 2);
    for (const AchievementUnlockResult& unlock : unlocks) {
        TEST_CHECK(!unlock.rewardGranted);
        TEST_CHECK(unlock.xpGranted == 0);
    }
    TEST_CHECK(profile.xp() == xpBefore);

    // Restart after the migration: the ledger persists as v2 and a partial
    // reset still cannot re-grant the old rewards.
    TEST_CHECK(manager.save(profile, stats));
    UserProfile reloaded;
    Statistics reloadedStats;
    TEST_CHECK(manager.load(reloaded, reloadedStats));
    TEST_CHECK(reloaded.hasReceivedReward("first_steps"));
    reloaded.resetAchievementsOnly();
    const int reloadedXp = reloaded.xp();
    finishOneGame(reloaded, reloadedStats, 0);
    TEST_CHECK(reloaded.xp() == reloadedXp);
    fs::remove_all(directory);
}

// A condition that was never satisfied in v1 must NOT be marked as rewarded:
// the player still earns it (with XP) for the first time later.
void testV1MigrationKeepsUnearnedRewardsAvailable() {
    const fs::path directory = uniqueTestDirectory("migration_unearned");
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    writeV1SaveFile(savePath, 100, "", 2, 0);  // only 2 games: nothing earned

    UserProfile profile;
    Statistics stats;
    SaveManager manager(savePath.string());
    TEST_CHECK(manager.load(profile, stats));
    TEST_CHECK(!profile.hasReceivedReward("first_steps"));

    // Three more games reach 5: the reward is granted now, for the first time.
    finishOneGame(profile, stats, 0);
    finishOneGame(profile, stats, 0);
    const int xpBefore = profile.xp();
    const auto unlocks = finishOneGame(profile, stats, 0);
    bool firstStepsGranted = false;
    for (const AchievementUnlockResult& unlock : unlocks) {
        if (unlock.achievement->id == "first_steps") {
            firstStepsGranted = unlock.rewardGranted && unlock.firstUnlock;
            TEST_CHECK(unlock.xpGranted == rewardOf("first_steps"));
        }
    }
    TEST_CHECK(firstStepsGranted);
    TEST_CHECK(profile.xp() >= xpBefore + rewardOf("first_steps"));
    fs::remove_all(directory);
}

// The evaluation result must tell the UI exactly what happened, so it never
// announces XP that was not granted.
void testUnlockResultsDistinguishRewardFromReactivation() {
    UserProfile profile;
    Statistics stats;

    // Games 1-2: nothing unlocks yet.
    TEST_CHECK(finishOneGame(profile, stats, 10).empty());
    TEST_CHECK(finishOneGame(profile, stats, 10).empty());

    // Game 3: memory_in_action (3 memory sessions) — real unlock with XP.
    const auto thirdGame = finishOneGame(profile, stats, 10);
    TEST_CHECK(thirdGame.size() == 1);
    TEST_CHECK(thirdGame[0].achievement->id == "memory_in_action");
    TEST_CHECK(thirdGame[0].firstUnlock);
    TEST_CHECK(thirdGame[0].rewardGranted);
    TEST_CHECK(thirdGame[0].xpGranted == rewardOf("memory_in_action"));

    // Game 4: no new unlocks. Then the visible list is cleared.
    TEST_CHECK(finishOneGame(profile, stats, 10).empty());
    profile.resetAchievementsOnly();

    // Game 5 mixes both cases in one evaluation: memory_in_action reappears
    // without XP while first_steps (5 games) is a genuine first unlock.
    const int xpBefore = profile.xp();
    const auto fifthGame = finishOneGame(profile, stats, 0);
    TEST_CHECK(fifthGame.size() == 2);
    bool sawReactivation = false;
    bool sawFirstUnlock = false;
    for (const AchievementUnlockResult& unlock : fifthGame) {
        if (unlock.achievement->id == "memory_in_action") {
            TEST_CHECK(!unlock.firstUnlock);
            TEST_CHECK(!unlock.rewardGranted);
            TEST_CHECK(unlock.xpGranted == 0);
            sawReactivation = true;
        } else if (unlock.achievement->id == "first_steps") {
            TEST_CHECK(unlock.firstUnlock);
            TEST_CHECK(unlock.rewardGranted);
            TEST_CHECK(unlock.xpGranted == rewardOf("first_steps"));
            sawFirstUnlock = true;
        }
    }
    TEST_CHECK(sawReactivation);
    TEST_CHECK(sawFirstUnlock);
    // The XP delta is exactly the one reward that was actually granted.
    TEST_CHECK(profile.xp() == xpBefore + rewardOf("first_steps"));
}

// Wiping the whole progress starts a new "lifetime": the ledger is cleared
// together with everything else, so a fresh profile earns rewards normally.
void testFullResetStartsAFreshRewardLifetime() {
    UserProfile profile;
    Statistics stats;
    for (int i = 0; i < 5; ++i) finishOneGame(profile, stats, 10);
    TEST_CHECK(profile.hasReceivedReward("first_steps"));

    profile.reset();
    stats.resetAll();
    TEST_CHECK(!profile.hasReceivedReward("first_steps"));

    for (int i = 0; i < 5; ++i) finishOneGame(profile, stats, 10);
    TEST_CHECK(profile.xp() ==
               5 * 10 + rewardOf("first_steps") + rewardOf("memory_in_action"));
}

}  // namespace

int main() {
    std::cout << "Running AchievementsTest...\n";
    testResetAchievementsThenPlayNeverRegrantsXp();
    testLedgerSurvivesSaveAndReload();
    testV1MigrationDoesNotRegrantRewards();
    testV1MigrationCoversClearedAchievementsList();
    testV1MigrationKeepsUnearnedRewardsAvailable();
    testUnlockResultsDistinguishRewardFromReactivation();
    testFullResetStartsAFreshRewardLifetime();
    std::cout << "All Achievements tests passed!\n";
    return 0;
}
