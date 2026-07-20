#include <iostream>
#include <string>

#include "../TestUtils.h"
#include "core/GameRegistry.h"
#include "core/Recommendations.h"
#include "core/Statistics.h"

namespace {

constexpr std::int64_t kNow = 1000000;
constexpr std::int64_t kDay = 86400;

void testNeverPlayedGameScoresHighest() {
    GameRegistry registry;
    Statistics stats;
    const GameInfo* numberMemory = registry.findById("number_memory");
    TEST_CHECK(numberMemory != nullptr);

    const float beforePlaying = Recommendations::priorityScore(*numberMemory, stats, kNow);
    TEST_CHECK(beforePlaying > 90.0f);  // never played + untouched category: max urgency

    GameResult strong;
    strong.score = 95;
    stats.recordResult("number_memory", numberMemory->category, strong, kNow);
    const float afterStrongRecentSession =
        Recommendations::priorityScore(*numberMemory, stats, kNow);
    TEST_CHECK(afterStrongRecentSession < beforePlaying);
}

void testUnimplementedGameAlwaysScoresLowest() {
    GameRegistry registry;
    Statistics stats;
    const GameInfo* crosswords = registry.findById("crosswords");
    TEST_CHECK(crosswords != nullptr);
    TEST_CHECK(!crosswords->isImplemented());
    TEST_CHECK(Recommendations::priorityScore(*crosswords, stats, kNow) < 0.0f);
}

void testStaleSessionScoresHigherThanRecentOne() {
    GameRegistry registry;
    const GameInfo* mentalMath = registry.findById("mental_math");
    TEST_CHECK(mentalMath != nullptr);

    GameResult result;
    result.score = 70;

    Statistics staleStats;
    staleStats.recordResult("mental_math", mentalMath->category, result, kNow - 30 * kDay);
    const float staleScore = Recommendations::priorityScore(*mentalMath, staleStats, kNow);

    Statistics recentStats;
    recentStats.recordResult("mental_math", mentalMath->category, result, kNow - kDay);
    const float recentScore = Recommendations::priorityScore(*mentalMath, recentStats, kNow);

    TEST_CHECK(staleScore > recentScore);
}

void testDailyTrainingSpansDistinctCategoriesAndRespectsMaxCount() {
    GameRegistry registry;
    Statistics stats;
    const auto picks = Recommendations::dailyTraining(registry, stats, kNow, 3);
    TEST_CHECK(picks.size() == 3);

    for (std::size_t i = 0; i < picks.size(); ++i) {
        TEST_CHECK(picks[i].game != nullptr);
        TEST_CHECK(picks[i].game->isImplemented());
        TEST_CHECK(picks[i].reason == "Experimente pela primeira vez");
        for (std::size_t j = i + 1; j < picks.size(); ++j) {
            TEST_CHECK(picks[i].game->category != picks[j].game->category);
        }
    }
}

void testDailyTrainingCapsAtImplementedGameCount() {
    GameRegistry registry;
    Statistics stats;
    std::size_t implementedCount = 0;
    for (const GameInfo& info : registry.games()) {
        if (info.isImplemented()) ++implementedCount;
    }
    const auto picks = Recommendations::dailyTraining(registry, stats, kNow, 10);
    TEST_CHECK(picks.size() == implementedCount);
}

// A weak-but-already-tried category surfaces with a "strengthen it" reason
// once it is no longer competing against the stronger "never played" pull of
// every other game.
void testReasonReflectsWeakCategoryOnceOthersAreNoLongerNovel() {
    GameRegistry registry;
    Statistics stats;
    GameResult strong;
    strong.score = 95;
    for (const GameInfo& info : registry.games()) {
        if (!info.isImplemented() || info.id == "mental_math") continue;
        stats.recordResult(info.id, info.category, strong, kNow);
    }
    const GameInfo* mentalMath = registry.findById("mental_math");
    GameResult weak;
    weak.score = 20;
    stats.recordResult("mental_math", mentalMath->category, weak, kNow - kDay);

    const auto picks = Recommendations::dailyTraining(registry, stats, kNow, 3);
    bool found = false;
    for (const auto& entry : picks) {
        if (entry.game->id != "mental_math") continue;
        found = true;
        TEST_CHECK(entry.reason == std::string("Fortaleça ") + categoryName(mentalMath->category));
    }
    TEST_CHECK(found);
}

// Once the category skill is already solid, staleness alone should be the
// reason shown, not a "strengthen it" message.
void testReasonReflectsStalenessWhenSkillIsAlreadyGood() {
    GameRegistry registry;
    Statistics stats;
    const GameInfo* mentalMath = registry.findById("mental_math");
    GameResult strong;
    strong.score = 90;
    stats.recordResult("mental_math", mentalMath->category, strong, kNow - 5 * kDay);
    TEST_CHECK(stats.forCategory(mentalMath->category).skill >= 60.0f);

    for (const GameInfo& info : registry.games()) {
        if (!info.isImplemented() || info.id == "mental_math") continue;
        stats.recordResult(info.id, info.category, strong, kNow);
    }

    const auto picks = Recommendations::dailyTraining(registry, stats, kNow, 3);
    bool found = false;
    for (const auto& entry : picks) {
        if (entry.game->id != "mental_math") continue;
        found = true;
        TEST_CHECK(entry.reason == "Há 5 dias sem treinar");
    }
    TEST_CHECK(found);
}

}  // namespace

int main() {
    std::cout << "Running RecommendationsTest...\n";
    testNeverPlayedGameScoresHighest();
    testUnimplementedGameAlwaysScoresLowest();
    testStaleSessionScoresHigherThanRecentOne();
    testDailyTrainingSpansDistinctCategoriesAndRespectsMaxCount();
    testDailyTrainingCapsAtImplementedGameCount();
    testReasonReflectsWeakCategoryOnceOthersAreNoLongerNovel();
    testReasonReflectsStalenessWhenSkillIsAlreadyGood();
    std::cout << "All Recommendations tests passed!\n";
    return 0;
}
