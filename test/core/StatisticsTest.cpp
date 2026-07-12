#include <iostream>
#include "core/Statistics.h"
#include "../TestUtils.h"

void testGameStats() {
    Statistics stats;
    GameResult r1{80, 10, 8, 10};
    stats.recordResult("math_game", GameCategory::Logic, r1, 1000);
    GameResult r2{90, 15, 9, 10};
    stats.recordResult("math_game", GameCategory::Logic, r2, 2000);

    const GameStats& gs = stats.forGame("math_game");
    TEST_CHECK(gs.sessions == 2);
    TEST_CHECK(gs.bestScore == 90);
    TEST_CHECK(gs.averageScore == 85.0f);
    TEST_CHECK(gs.mostRecentScore == 90);
    TEST_CHECK(gs.totalCorrect == 17);
    TEST_CHECK(gs.totalAttempts == 20);

    GameResult r3{100, 20, 10, 10};
    stats.recordResult("memory_game", GameCategory::Memory, r3, 3000);
    const GameStats& mg = stats.forGame("memory_game");
    TEST_CHECK(mg.sessions == 1);
    TEST_CHECK(mg.bestScore == 100);
}

void testChronologicalOrdering() {
    Statistics stats;
    stats.recordResult("math_game", GameCategory::Logic, {80, 10, 8, 10}, 5000);
    stats.recordResult("math_game", GameCategory::Logic, {90, 10, 9, 10}, 6000);
    stats.recordResult("math_game", GameCategory::Logic, {100, 10, 10, 10}, 7000);

    const auto& hist = stats.history();
    TEST_CHECK(hist.size() == 3);
    TEST_CHECK(hist[0].timestamp == 5000);
    TEST_CHECK(hist[1].timestamp == 6000);
    TEST_CHECK(hist[2].timestamp == 7000);
}

void testSummaries() {
    Statistics stats;
    // Day 1
    stats.recordResult("g1", GameCategory::Logic, {50, 0, 5, 10}, 86400 * 1);
    // Day 2
    stats.recordResult("g2", GameCategory::Memory, {100, 0, 10, 10}, 86400 * 2);
    // Day 10
    stats.recordResult("g1", GameCategory::Logic, {60, 0, 6, 10}, 86400 * 10);

    // Test weekly summary from Day 2 (should include day 1 and day 2)
    SummaryStats w = stats.weeklySummary(86400 * 2);
    TEST_CHECK(w.sessionsCompleted == 2);
    TEST_CHECK(w.averageScore == 75.0f); // (50 + 100) / 2
    TEST_CHECK(w.activeDays == 2);
    TEST_CHECK(w.categoriesTrained == 2);

    // Test daily summary from Day 10
    SummaryStats d = stats.dailySummary(86400 * 10);
    TEST_CHECK(d.sessionsCompleted == 1);
    TEST_CHECK(d.averageScore == 60.0f);
    TEST_CHECK(d.activeDays == 1);
    TEST_CHECK(d.categoriesTrained == 1);

    // Test monthly summary from Day 10
    SummaryStats m = stats.monthlySummary(86400 * 10);
    TEST_CHECK(m.sessionsCompleted == 3);
}

void testChartDataPreparation() {
    Statistics stats;
    stats.recordResult("g1", GameCategory::Logic, {50, 0, 5, 10}, 1000);
    stats.recordResult("g2", GameCategory::Memory, {100, 0, 10, 10}, 2000);

    auto all = stats.prepareChartSeries(Statistics::FilterType::All);
    TEST_CHECK(all.size() == 2);

    auto g1 = stats.prepareChartSeries(Statistics::FilterType::Game, "g1");
    TEST_CHECK(g1.size() == 1);
    TEST_CHECK(g1[0].score == 50.0f);

    auto mem = stats.prepareChartSeries(Statistics::FilterType::Category, std::to_string(static_cast<int>(GameCategory::Memory)));
    TEST_CHECK(mem.size() == 1);
    TEST_CHECK(mem[0].score == 100.0f);
}

void testResets() {
    Statistics stats;
    stats.recordResult("g1", GameCategory::Logic, {50, 0, 5, 10}, 1000);

    stats.resetHistoryOnly();
    TEST_CHECK(stats.history().size() == 0);
    TEST_CHECK(stats.forGame("g1").sessions == 1); // Stats preserved
    TEST_CHECK(stats.totalGamesPlayed() == 1);

    stats.resetStatisticsOnly();
    TEST_CHECK(stats.forGame("g1").sessions == 0);
    TEST_CHECK(stats.totalGamesPlayed() == 0);
}

int main() {
    std::cout << "Running StatisticsTest...\n";
    testGameStats();
    testChronologicalOrdering();
    testSummaries();
    testChartDataPreparation();
    testResets();
    std::cout << "All Statistics tests passed!\n";
    return 0;
}
