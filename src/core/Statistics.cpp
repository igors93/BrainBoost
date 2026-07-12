#include "core/Statistics.h"

#include <algorithm>
#include <sstream>

void Statistics::recordResult(GameCategory category, const GameResult& result) {
    CategoryStats& stats = categories_[static_cast<int>(category)];

    // Exponential moving average: recent sessions weigh more, but a single
    // bad run does not wipe out accumulated skill.
    if (stats.gamesPlayed == 0) {
        stats.skill = static_cast<float>(result.score);
    } else {
        stats.skill = stats.skill * 0.7f + static_cast<float>(result.score) * 0.3f;
    }
    ++stats.gamesPlayed;
    ++totalGames_;

    history_.push_back(static_cast<float>(result.score));
    if (history_.size() > kMaxHistory) history_.erase(history_.begin());
}

void Statistics::toMap(KeyValueMap& out) const {
    out["stats.total_games"] = std::to_string(totalGames_);

    for (int i = 0; i < kCategoryCount; ++i) {
        const std::string prefix = "stats.category." + std::to_string(i);
        out[prefix + ".skill"] = std::to_string(categories_[i].skill);
        out[prefix + ".games"] = std::to_string(categories_[i].gamesPlayed);
    }

    std::ostringstream joined;
    for (size_t i = 0; i < history_.size(); ++i) {
        if (i > 0) joined << ',';
        joined << history_[i];
    }
    out["stats.history"] = joined.str();
}

void Statistics::fromMap(const KeyValueMap& in) {
    reset();
    if (auto it = in.find("stats.total_games"); it != in.end())
        totalGames_ = std::stoi(it->second);

    for (int i = 0; i < kCategoryCount; ++i) {
        const std::string prefix = "stats.category." + std::to_string(i);
        if (auto it = in.find(prefix + ".skill"); it != in.end())
            categories_[i].skill = std::stof(it->second);
        if (auto it = in.find(prefix + ".games"); it != in.end())
            categories_[i].gamesPlayed = std::stoi(it->second);
    }

    if (auto it = in.find("stats.history"); it != in.end()) {
        std::istringstream stream(it->second);
        std::string value;
        while (std::getline(stream, value, ',')) {
            if (!value.empty()) history_.push_back(std::stof(value));
        }
    }
}

void Statistics::reset() {
    categories_.fill(CategoryStats{});
    history_.clear();
    totalGames_ = 0;
}
