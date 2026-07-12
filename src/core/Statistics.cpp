#include "core/Statistics.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_set>

void Statistics::recordResult(const std::string& gameId, GameCategory category, const GameResult& result, std::int64_t timestamp) {
    CategoryStats& catStats = categories_[static_cast<int>(category)];
    if (catStats.gamesPlayed == 0) {
        catStats.skill = static_cast<float>(result.score);
    } else {
        catStats.skill = catStats.skill * 0.7f + static_cast<float>(result.score) * 0.3f;
    }
    ++catStats.gamesPlayed;
    ++totalGames_;

    GameStats& gStats = gameStats_[gameId];
    if (gStats.sessions == 0) {
        gStats.bestScore = result.score;
        gStats.averageScore = static_cast<float>(result.score);
    } else {
        gStats.bestScore = std::max(gStats.bestScore, result.score);
        gStats.averageScore = (gStats.averageScore * gStats.sessions + result.score) / (gStats.sessions + 1);
    }
    gStats.mostRecentScore = result.score;
    gStats.totalCorrect += result.correct;
    gStats.totalAttempts += result.total;
    ++gStats.sessions;

    SessionRecord record;
    record.gameId = gameId;
    record.category = category;
    record.timestamp = timestamp;
    record.score = result.score;
    record.correct = result.correct;
    record.total = result.total;
    record.difficulty = 0;
    record.durationSeconds = 0;

    history_.push_back(record);
    if (history_.size() > kMaxHistory) history_.erase(history_.begin());
}

SummaryStats Statistics::summaryForPeriod(std::int64_t startTimestamp, std::int64_t endTimestamp) const {
    SummaryStats s;
    std::unordered_set<int> cats;
    std::unordered_set<std::int64_t> days;
    for (const auto& rec : history_) {
        if (rec.timestamp >= startTimestamp && rec.timestamp <= endTimestamp) {
            s.sessionsCompleted++;
            s.averageScore += rec.score;
            s.totalTrainingTimeSeconds += rec.durationSeconds;
            cats.insert(static_cast<int>(rec.category));
            // Using a simple rough grouping for days (GMT-based is enough for summaries).
            days.insert(rec.timestamp / 86400); 
        }
    }
    if (s.sessionsCompleted > 0) {
        s.averageScore /= s.sessionsCompleted;
    }
    s.categoriesTrained = cats.size();
    s.activeDays = days.size();
    return s;
}

SummaryStats Statistics::dailySummary(std::int64_t now) const {
    return summaryForPeriod(now - 86400, now);
}

SummaryStats Statistics::weeklySummary(std::int64_t now) const {
    return summaryForPeriod(now - 7 * 86400, now);
}

SummaryStats Statistics::monthlySummary(std::int64_t now) const {
    return summaryForPeriod(now - 30 * 86400, now);
}

ChartSeries Statistics::prepareChartSeries(FilterType filter, const std::string& filterValue, std::int64_t now) const {
    ChartSeries series;
    for (const auto& rec : history_) {
        bool match = true;
        if (filter == FilterType::Game && rec.gameId != filterValue) match = false;
        if (filter == FilterType::Category && std::to_string(static_cast<int>(rec.category)) != filterValue) match = false;
        if (filter == FilterType::Recent && (now - rec.timestamp > 30 * 86400)) match = false;
        if (match) {
            ChartPoint p;
            p.timestamp = rec.timestamp;
            p.score = static_cast<float>(rec.score);
            p.gameId = rec.gameId;
            p.category = rec.category;
            series.push_back(p);
        }
    }
    return series;
}

void Statistics::toMap(KeyValueMap& out) const {
    out["stats.total_games"] = std::to_string(totalGames_);

    for (int i = 0; i < kCategoryCount; ++i) {
        const std::string prefix = "stats.category." + std::to_string(i);
        out[prefix + ".skill"] = std::to_string(categories_[i].skill);
        out[prefix + ".games"] = std::to_string(categories_[i].gamesPlayed);
    }
    
    for (const auto& [id, gs] : gameStats_) {
        const std::string prefix = "stats.game." + id;
        out[prefix + ".sessions"] = std::to_string(gs.sessions);
        out[prefix + ".bestScore"] = std::to_string(gs.bestScore);
        out[prefix + ".averageScore"] = std::to_string(gs.averageScore);
        out[prefix + ".mostRecentScore"] = std::to_string(gs.mostRecentScore);
        out[prefix + ".totalCorrect"] = std::to_string(gs.totalCorrect);
        out[prefix + ".totalAttempts"] = std::to_string(gs.totalAttempts);
        out[prefix + ".bestDifficulty"] = std::to_string(gs.bestDifficulty);
    }

    std::ostringstream joined;
    for (size_t i = 0; i < history_.size(); ++i) {
        if (i > 0) joined << '|';
        const auto& r = history_[i];
        joined << r.gameId << ',' 
               << static_cast<int>(r.category) << ','
               << r.timestamp << ','
               << r.score << ','
               << r.correct << ','
               << r.total << ','
               << r.difficulty << ','
               << r.durationSeconds;
    }
    out["stats.history"] = joined.str();
}

void Statistics::fromMap(const KeyValueMap& in) {
    resetAll();
    
    if (auto it = in.find("stats.total_games"); it != in.end()) {
        try { totalGames_ = std::max(0, std::stoi(it->second)); } catch (...) {}
    }

    for (int i = 0; i < kCategoryCount; ++i) {
        const std::string prefix = "stats.category." + std::to_string(i);
        if (auto it = in.find(prefix + ".skill"); it != in.end()) {
            try { categories_[i].skill = std::clamp(std::stof(it->second), 0.0f, 100.0f); } catch (...) {}
        }
        if (auto it = in.find(prefix + ".games"); it != in.end()) {
            try { categories_[i].gamesPlayed = std::max(0, std::stoi(it->second)); } catch (...) {}
        }
    }
    
    for (const auto& [k, v] : in) {
        if (k.find("stats.game.") == 0) {
            size_t dotPos = k.find('.', 11);
            if (dotPos != std::string::npos) {
                std::string gameId = k.substr(11, dotPos - 11);
                std::string field = k.substr(dotPos + 1);
                auto& gs = gameStats_[gameId];
                try {
                    if (field == "sessions") gs.sessions = std::max(0, std::stoi(v));
                    else if (field == "bestScore") gs.bestScore = std::max(0, std::stoi(v));
                    else if (field == "averageScore") gs.averageScore = std::clamp(std::stof(v), 0.0f, 100.0f);
                    else if (field == "mostRecentScore") gs.mostRecentScore = std::max(0, std::stoi(v));
                    else if (field == "totalCorrect") gs.totalCorrect = std::max(0, std::stoi(v));
                    else if (field == "totalAttempts") gs.totalAttempts = std::max(0, std::stoi(v));
                    else if (field == "bestDifficulty") gs.bestDifficulty = std::max(0, std::stoi(v));
                } catch (...) {}
            }
        }
    }

    if (auto it = in.find("stats.history"); it != in.end()) {
        if (it->second.find('|') == std::string::npos && it->second.find(',') != std::string::npos && it->second.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") == std::string::npos) {
            std::istringstream stream(it->second);
            std::string value;
            std::int64_t dummyTs = 0; 
            while (std::getline(stream, value, ',')) {
                if (!value.empty()) {
                    try {
                        float v = std::stof(value);
                        if (!std::isnan(v) && !std::isinf(v)) {
                            SessionRecord r;
                            r.gameId = "legacy";
                            r.score = static_cast<int>(v);
                            r.timestamp = dummyTs++;
                            history_.push_back(r);
                        }
                    } catch (...) {}
                }
            }
        } else {
            std::istringstream stream(it->second);
            std::string recordStr;
            while (std::getline(stream, recordStr, '|')) {
                if (recordStr.empty()) continue;
                std::istringstream rs(recordStr);
                std::string token;
                SessionRecord r;
                try {
                    if (std::getline(rs, token, ',')) r.gameId = token;
                    if (std::getline(rs, token, ',')) r.category = static_cast<GameCategory>(std::stoi(token));
                    if (std::getline(rs, token, ',')) r.timestamp = std::stol(token);
                    if (std::getline(rs, token, ',')) r.score = std::stoi(token);
                    if (std::getline(rs, token, ',')) r.correct = std::stoi(token);
                    if (std::getline(rs, token, ',')) r.total = std::stoi(token);
                    if (std::getline(rs, token, ',')) r.difficulty = std::stoi(token);
                    if (std::getline(rs, token, ',')) r.durationSeconds = std::stoi(token);
                    history_.push_back(r);
                } catch (...) {}
            }
        }
        if (history_.size() > kMaxHistory) {
            history_.erase(history_.begin(), history_.begin() + (history_.size() - kMaxHistory));
        }
    }
}

void Statistics::resetAll() {
    categories_.fill(CategoryStats{});
    gameStats_.clear();
    history_.clear();
    totalGames_ = 0;
}

void Statistics::resetHistoryOnly() {
    history_.clear();
}

void Statistics::resetStatisticsOnly() {
    categories_.fill(CategoryStats{});
    gameStats_.clear();
    totalGames_ = 0;
}
