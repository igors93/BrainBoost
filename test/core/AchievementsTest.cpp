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
void finishOneGame(UserProfile& profile, Statistics& stats, int gameXp) {
    GameResult result;
    result.score = 60;
    result.xpEarned = gameXp;
    profile.addXp(result.xpEarned);
    stats.recordResult("number_memory", GameCategory::Memory, result, 1000);
    Achievements::evaluate(profile, stats);
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
    {
        std::ofstream file(savePath, std::ios::trunc);
        file << "save.version=1\n"
             << "profile.name=Igor\n"
             << "profile.xp=150\n"
             << "profile.streak=0\n"
             << "profile.last_played_day=0\n"
             << "profile.achievements=first_steps\n"
             << "stats.total_games=5\n"
             << "stats.history=\n";
        for (int index = 0; index < kCategoryCount; ++index) {
            file << "stats.category." << index << ".skill=50.000000\n"
                 << "stats.category." << index << ".games=1\n";
        }
    }

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
    testFullResetStartsAFreshRewardLifetime();
    std::cout << "All Achievements tests passed!\n";
    return 0;
}
