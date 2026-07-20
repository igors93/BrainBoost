#include <cmath>
#include <ctime>
#include <iostream>
#include <limits>

#include "../TestUtils.h"
#include "core/Statistics.h"

namespace {

std::int64_t localTimestamp(int year, int month, int day, int hour = 12) {
    std::tm value{};
    value.tm_year = year - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_hour = hour;
    value.tm_isdst = -1;
    return static_cast<std::int64_t>(std::mktime(&value));
}

void testSanitizedRecordingAndGameStats() {
    Statistics stats;
    GameResult first;
    first.score = 150;
    first.correct = 20;
    first.total = 10;
    first.difficulty = 3;
    first.durationSeconds = 45;
    stats.recordResult("mental_math", GameCategory::Reasoning, first, 1000);

    const GameStats& game = stats.forGame("mental_math");
    TEST_CHECK(game.sessions == 1);
    TEST_CHECK(game.bestScore == 100);
    TEST_CHECK(game.totalCorrect == 10);
    TEST_CHECK(game.totalAttempts == 10);
    TEST_CHECK(game.bestDifficulty == 3);
    TEST_CHECK(stats.history()[0].durationSeconds == 45);

    stats.recordResult("", GameCategory::Reasoning, first, 2000);
    stats.recordResult("bad,id", GameCategory::Reasoning, first, 3000);
    stats.recordResult("valid", static_cast<GameCategory>(999), first, 4000);
    TEST_CHECK(stats.totalGamesPlayed() == 1);
}

void testCalendarSummaries() {
    Statistics stats;
    GameResult result;
    result.score = 80;
    result.correct = 8;
    result.total = 10;
    result.durationSeconds = 60;

    stats.recordResult("g1", GameCategory::Logic, result,
                       localTimestamp(2026, 7, 10, 8));
    stats.recordResult("g2", GameCategory::Memory, result,
                       localTimestamp(2026, 7, 9, 23));
    stats.recordResult("g3", GameCategory::Attention, result,
                       localTimestamp(2026, 7, 3, 12));

    const std::int64_t now = localTimestamp(2026, 7, 10, 18);
    const SummaryStats daily = stats.dailySummary(now);
    TEST_CHECK(daily.sessionsCompleted == 1);
    TEST_CHECK(daily.activeDays == 1);
    TEST_CHECK(daily.totalTrainingTimeSeconds == 60);

    const SummaryStats weekly = stats.weeklySummary(now);
    TEST_CHECK(weekly.sessionsCompleted == 2);
    TEST_CHECK(weekly.activeDays == 2);

    const SummaryStats monthly = stats.monthlySummary(now);
    TEST_CHECK(monthly.sessionsCompleted == 3);
}

void testInvalidSerializedDataIsNormalizedOrSkipped() {
    KeyValueMap values;
    values["stats.total_games"] = "-5";
    values["stats.category.0.skill"] = "nan";
    values["stats.category.0.games"] = "-10";
    values["stats.game.mental_math.sessions"] = "2";
    values["stats.game.mental_math.bestScore"] = "900";
    values["stats.game.mental_math.averageScore"] = "inf";
    values["stats.game.mental_math.totalCorrect"] = "50";
    values["stats.game.mental_math.totalAttempts"] = "10";
    values["stats.history"] =
        "mental_math,1,1000,120,20,10,-4,-8|"  // Valid shape, normalized.
        "broken,999,1000,50,1,1,0,0|"          // Invalid category, skipped.
        "missing,1,1000,50";                   // Incomplete, skipped.

    Statistics stats;
    stats.fromMap(values);

    TEST_CHECK(stats.totalGamesPlayed() == 0);
    TEST_CHECK(stats.forCategory(GameCategory::Memory).skill == 0.0f);
    TEST_CHECK(stats.forGame("mental_math").bestScore == 100);
    TEST_CHECK(stats.forGame("mental_math").averageScore == 0.0f);
    TEST_CHECK(stats.forGame("mental_math").totalCorrect == 10);
    TEST_CHECK(stats.history().size() == 1);
    TEST_CHECK(stats.history()[0].score == 100);
    TEST_CHECK(stats.history()[0].correct == 10);
    TEST_CHECK(stats.history()[0].difficulty == 0);
    TEST_CHECK(stats.history()[0].durationSeconds == 0);
}

void testDifficultyLevelAdaptsAcrossSessionsAndPersists() {
    Statistics stats;
    GameResult strong;
    strong.score = 90;
    strong.total = 5;
    strong.correct = 5;

    stats.recordResult("mental_math", GameCategory::Reasoning, strong, 1000);
    TEST_CHECK(stats.forGame("mental_math").difficultyLevel == 1.0f);
    stats.recordResult("mental_math", GameCategory::Reasoning, strong, 1001);
    TEST_CHECK(stats.forGame("mental_math").difficultyLevel == 2.0f);

    GameResult weak;
    weak.score = 10;
    weak.total = 5;
    weak.correct = 1;
    stats.recordResult("mental_math", GameCategory::Reasoning, weak, 1002);
    TEST_CHECK(stats.forGame("mental_math").difficultyLevel == 1.0f);

    // The adaptive level survives a save/load round trip like any other
    // per-game stat.
    KeyValueMap values;
    stats.toMap(values);
    Statistics restored;
    restored.fromMap(values);
    TEST_CHECK(restored.forGame("mental_math").difficultyLevel == 1.0f);

    // A never-played game reports the baseline (no adaptation yet).
    TEST_CHECK(stats.forGame("never_played").difficultyLevel == 0.0f);
}

void testLastPlayedTimestampTracksMostRecentSessionAndPersists() {
    Statistics stats;
    TEST_CHECK(stats.forGame("mental_math").lastPlayedTimestamp == 0);

    GameResult result;
    result.score = 50;
    stats.recordResult("mental_math", GameCategory::Reasoning, result, 1000);
    TEST_CHECK(stats.forGame("mental_math").lastPlayedTimestamp == 1000);
    stats.recordResult("mental_math", GameCategory::Reasoning, result, 2000);
    TEST_CHECK(stats.forGame("mental_math").lastPlayedTimestamp == 2000);

    KeyValueMap values;
    stats.toMap(values);
    Statistics restored;
    restored.fromMap(values);
    TEST_CHECK(restored.forGame("mental_math").lastPlayedTimestamp == 2000);

    // A never-played game reports 0 (never played).
    TEST_CHECK(stats.forGame("never_played").lastPlayedTimestamp == 0);
}

void testFirstSessionSetsAverageDifficultyDirectly() {
    Statistics stats;
    GameResult result;
    result.score = 70;
    result.difficulty = 4;
    stats.recordResult("logic_sequence", GameCategory::Logic, result, 1000);
    TEST_CHECK(stats.forCategory(GameCategory::Logic).averageDifficulty == 4.0f);
    TEST_CHECK(stats.forCategory(GameCategory::Logic).skill == 70.0f);
}

// A session played above your own typical difficulty for a category counts
// for more than the raw score alone would (an IRT-inspired nudge on top of
// the score EMA — see recordResult()).
void testSessionAboveOwnAverageDifficultyBoostsSkill() {
    Statistics stats;
    GameResult first;
    first.score = 70;
    first.difficulty = 2;
    stats.recordResult("logic_sequence", GameCategory::Logic, first, 1000);

    GameResult harder;
    harder.score = 70;  // same score as before...
    harder.difficulty = 10;  // ...but well above the average difficulty (2)
    stats.recordResult("logic_sequence", GameCategory::Logic, harder, 1001);

    const float flatEmaWouldBe = 70.0f * 0.7f + 70.0f * 0.3f;  // == 70, no bonus
    TEST_CHECK(stats.forCategory(GameCategory::Logic).skill > flatEmaWouldBe + 0.01f);
}

// Symmetrically, a session played below your own typical difficulty counts
// for less, even with an identical score.
void testSessionBelowOwnAverageDifficultyLowersSkill() {
    Statistics stats;
    GameResult first;
    first.score = 70;
    first.difficulty = 10;
    stats.recordResult("logic_sequence", GameCategory::Logic, first, 1000);

    GameResult easier;
    easier.score = 70;  // same score as before...
    easier.difficulty = 0;  // ...but well below the average difficulty (10)
    stats.recordResult("logic_sequence", GameCategory::Logic, easier, 1001);

    const float flatEmaWouldBe = 70.0f * 0.7f + 70.0f * 0.3f;  // == 70, no penalty
    TEST_CHECK(stats.forCategory(GameCategory::Logic).skill < flatEmaWouldBe - 0.01f);
}

// However extreme the difficulty swing, the bonus/penalty stays capped —
// two wildly different jumps above the average produce the same skill.
void testDifficultyBonusIsClampedForExtremeSwings() {
    Statistics moderate;
    GameResult first;
    first.score = 70;
    first.difficulty = 2;
    moderate.recordResult("logic_sequence", GameCategory::Logic, first, 1000);
    GameResult moderateJump;
    moderateJump.score = 70;
    moderateJump.difficulty = 10;  // delta 8: already beyond the cap
    moderate.recordResult("logic_sequence", GameCategory::Logic, moderateJump, 1001);

    Statistics extreme;
    extreme.recordResult("logic_sequence", GameCategory::Logic, first, 1000);
    GameResult extremeJump;
    extremeJump.score = 70;
    extremeJump.difficulty = 1000;  // delta 998: absurdly beyond the cap
    extreme.recordResult("logic_sequence", GameCategory::Logic, extremeJump, 1001);

    TEST_CHECK(std::fabs(moderate.forCategory(GameCategory::Logic).skill -
                        extreme.forCategory(GameCategory::Logic).skill) < 0.01f);
}

void testAverageDifficultyPersistsThroughRoundTrip() {
    Statistics stats;
    GameResult result;
    result.score = 60;
    result.difficulty = 3;
    stats.recordResult("logic_sequence", GameCategory::Logic, result, 1000);
    GameResult second;
    second.score = 60;
    second.difficulty = 7;
    stats.recordResult("logic_sequence", GameCategory::Logic, second, 1001);

    KeyValueMap values;
    stats.toMap(values);
    Statistics restored;
    restored.fromMap(values);
    TEST_CHECK(std::fabs(restored.forCategory(GameCategory::Logic).averageDifficulty -
                        stats.forCategory(GameCategory::Logic).averageDifficulty) < 0.001f);
}

void testRoundTrip() {
    Statistics original;
    GameResult result;
    result.score = 73;
    result.correct = 7;
    result.total = 10;
    result.difficulty = 4;
    result.durationSeconds = 31;
    original.recordResult("logic_sequence", GameCategory::Logic, result, 123456789);

    KeyValueMap values;
    original.toMap(values);
    Statistics restored;
    restored.fromMap(values);

    TEST_CHECK(restored.history().size() == 1);
    TEST_CHECK(restored.history()[0].timestamp == 123456789);
    TEST_CHECK(restored.forGame("logic_sequence").bestDifficulty == 4);
}

}  // namespace

int main() {
    std::cout << "Running StatisticsTest...\n";
    testSanitizedRecordingAndGameStats();
    testCalendarSummaries();
    testInvalidSerializedDataIsNormalizedOrSkipped();
    testDifficultyLevelAdaptsAcrossSessionsAndPersists();
    testLastPlayedTimestampTracksMostRecentSessionAndPersists();
    testFirstSessionSetsAverageDifficultyDirectly();
    testSessionAboveOwnAverageDifficultyBoostsSkill();
    testSessionBelowOwnAverageDifficultyLowersSkill();
    testDifficultyBonusIsClampedForExtremeSwings();
    testAverageDifficultyPersistsThroughRoundTrip();
    testRoundTrip();
    std::cout << "All Statistics tests passed!\n";
    return 0;
}
