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
    testRoundTrip();
    std::cout << "All Statistics tests passed!\n";
    return 0;
}
