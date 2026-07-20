#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "core/GameCategory.h"
#include "core/GameResult.h"
#include "core/SessionRecord.h"

using KeyValueMap = std::map<std::string, std::string>;

struct CategoryStats {
    float skill = 0.0f;
    int gamesPlayed = 0;

    // Running average of the difficulty level played in this category
    // (see AdaptiveDifficulty). Lets recordResult() weigh a session played
    // above your own typical difficulty a bit more than one played below
    // it — a per-player, per-category reference point, not a fixed scale.
    float averageDifficulty = 0.0f;
};

struct GameStats {
    int sessions = 0;
    int bestScore = 0;
    float averageScore = 0.0f;
    int mostRecentScore = 0;
    int totalCorrect = 0;
    int totalAttempts = 0;
    int bestDifficulty = 0;

    // Current adaptive difficulty level for this game: where the *next*
    // session should start. Unlike bestDifficulty (a personal record), this
    // rises and falls with recent performance — see AdaptiveDifficulty.
    float difficultyLevel = 0.0f;

    // Unix timestamp of the most recent completed session; 0 = never played.
    // Feeds the "how long since you trained this" half of Recommendations.
    std::int64_t lastPlayedTimestamp = 0;
};

struct SummaryStats {
    int sessionsCompleted = 0;
    float averageScore = 0.0f;
    int totalTrainingTimeSeconds = 0;
    int categoriesTrained = 0;
    int activeDays = 0;
};

struct ChartPoint {
    std::int64_t timestamp = 0;
    float score = 0.0f;
    std::string gameId;
    GameCategory category = GameCategory::Memory;
};
using ChartSeries = std::vector<ChartPoint>;

class Statistics {
public:
    void recordResult(const std::string& gameId, GameCategory category,
                      const GameResult& result, std::int64_t timestamp);

    const CategoryStats& forCategory(GameCategory category) const {
        const int index = static_cast<int>(category);
        if (index >= 0 && index < kCategoryCount) return categories_[index];
        static const CategoryStats empty;
        return empty;
    }

    const GameStats& forGame(const std::string& gameId) const {
        const auto it = gameStats_.find(gameId);
        if (it != gameStats_.end()) return it->second;
        static const GameStats empty;
        return empty;
    }

    int totalGamesPlayed() const { return totalGames_; }
    const std::vector<SessionRecord>& history() const { return history_; }

    SummaryStats summaryForPeriod(std::int64_t startTimestamp,
                                  std::int64_t endTimestamp) const;
    SummaryStats dailySummary(std::int64_t now) const;
    SummaryStats weeklySummary(std::int64_t now) const;
    SummaryStats monthlySummary(std::int64_t now) const;

    enum class FilterType { All, Game, Category, Recent };
    ChartSeries prepareChartSeries(FilterType filter,
                                   const std::string& filterValue = "",
                                   std::int64_t now = 0) const;

    void toMap(KeyValueMap& out) const;
    void fromMap(const KeyValueMap& in);

    void resetAll();
    void resetHistoryOnly();
    void resetStatisticsOnly();

private:
    static constexpr std::size_t kMaxHistory = 1000;

    std::array<CategoryStats, kCategoryCount> categories_{};
    std::map<std::string, GameStats> gameStats_;
    std::vector<SessionRecord> history_;
    int totalGames_ = 0;
};
