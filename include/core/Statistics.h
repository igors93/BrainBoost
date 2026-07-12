#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

#include "core/GameCategory.h"
#include "core/GameResult.h"

using KeyValueMap = std::map<std::string, std::string>;

// Skill level (0..100) for one category plus how many sessions trained it.
struct CategoryStats {
    float skill = 0.0f;
    int gamesPlayed = 0;
};

// Aggregates game results into per-category skills and a score history
// used by the evolution chart. Serialized through toMap()/fromMap().
class Statistics {
public:
    void recordResult(GameCategory category, const GameResult& result);

    const CategoryStats& forCategory(GameCategory category) const {
        return categories_[static_cast<int>(category)];
    }
    int totalGamesPlayed() const { return totalGames_; }

    // Scores of the most recent sessions, oldest first (capped).
    const std::vector<float>& history() const { return history_; }

    void toMap(KeyValueMap& out) const;
    void fromMap(const KeyValueMap& in);

    void reset();

private:
    static constexpr size_t kMaxHistory = 60;

    std::array<CategoryStats, kCategoryCount> categories_{};
    std::vector<float> history_;
    int totalGames_ = 0;
};
