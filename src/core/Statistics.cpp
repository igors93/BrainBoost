#include "core/Statistics.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cmath>
#include <ctime>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace {

bool isValidCategory(GameCategory category) {
    const int index = static_cast<int>(category);
    return index >= 0 && index < kCategoryCount;
}

bool isSafeGameId(const std::string& gameId) {
    if (gameId.empty() || gameId.size() > 80) return false;
    for (const unsigned char character : gameId) {
        const bool allowed = std::isalnum(character) != 0 || character == '_' ||
                             character == '-';
        if (!allowed) return false;
    }
    return true;
}

int saturatingAdd(int left, int right) {
    if (right <= 0) return left;
    if (left > std::numeric_limits<int>::max() - right) {
        return std::numeric_limits<int>::max();
    }
    return left + right;
}

bool parseInt(const std::string& text, int& output) {
    try {
        std::size_t parsed = 0;
        const long long value = std::stoll(text, &parsed, 10);
        if (parsed != text.size() || value < std::numeric_limits<int>::min() ||
            value > std::numeric_limits<int>::max()) {
            return false;
        }
        output = static_cast<int>(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseInt64(const std::string& text, std::int64_t& output) {
    try {
        std::size_t parsed = 0;
        const long long value = std::stoll(text, &parsed, 10);
        if (parsed != text.size()) return false;
        output = static_cast<std::int64_t>(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseFiniteFloat(const std::string& text, float& output) {
    try {
        std::size_t parsed = 0;
        const float value = std::stof(text, &parsed);
        if (parsed != text.size() || !std::isfinite(value)) return false;
        output = value;
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> parts;
    std::istringstream stream(value);
    std::string part;
    while (std::getline(stream, part, delimiter)) parts.push_back(part);
    return parts;
}

bool localTime(std::int64_t timestamp, std::tm& output) {
    if (timestamp < 0) return false;
    const std::time_t raw = static_cast<std::time_t>(timestamp);
    if (static_cast<std::int64_t>(raw) != timestamp) return false;
    const std::tm* converted = std::localtime(&raw);
    if (converted == nullptr) return false;
    output = *converted;
    return true;
}

std::int64_t startOfLocalDay(std::int64_t timestamp) {
    std::tm local{};
    if (!localTime(timestamp, local)) return timestamp - timestamp % 86400;
    local.tm_hour = 0;
    local.tm_min = 0;
    local.tm_sec = 0;
    local.tm_isdst = -1;
    return static_cast<std::int64_t>(std::mktime(&local));
}

std::int64_t shiftLocalDays(std::int64_t localDayStart, int days) {
    std::tm local{};
    if (!localTime(localDayStart, local)) {
        return localDayStart + static_cast<std::int64_t>(days) * 86400;
    }
    local.tm_mday += days;
    local.tm_isdst = -1;
    return static_cast<std::int64_t>(std::mktime(&local));
}

std::int64_t localDayKey(std::int64_t timestamp) {
    std::tm local{};
    if (!localTime(timestamp, local)) return timestamp / 86400;
    return static_cast<std::int64_t>(local.tm_year + 1900) * 10000 +
           static_cast<std::int64_t>(local.tm_mon + 1) * 100 + local.tm_mday;
}

SessionRecord sanitizedRecord(const std::string& gameId, GameCategory category,
                              const GameResult& result, std::int64_t timestamp) {
    SessionRecord record;
    record.gameId = gameId;
    record.category = category;
    record.timestamp = std::max<std::int64_t>(0, timestamp);
    record.score = std::clamp(result.score, 0, 100);
    record.total = std::max(0, result.total);
    record.correct = std::clamp(result.correct, 0, record.total);
    record.difficulty = std::max(0, result.difficulty);
    record.durationSeconds = std::max(0, result.durationSeconds);
    return record;
}

bool parseSessionRecord(const std::string& serialized, SessionRecord& record) {
    const std::vector<std::string> fields = split(serialized, ',');
    if (fields.size() != 8 || !isSafeGameId(fields[0])) return false;

    int categoryValue = 0;
    int score = 0;
    int correct = 0;
    int total = 0;
    int difficulty = 0;
    int duration = 0;
    std::int64_t timestamp = 0;

    if (!parseInt(fields[1], categoryValue) || categoryValue < 0 ||
        categoryValue >= kCategoryCount || !parseInt64(fields[2], timestamp) ||
        timestamp < 0 || !parseInt(fields[3], score) || !parseInt(fields[4], correct) ||
        !parseInt(fields[5], total) || !parseInt(fields[6], difficulty) ||
        !parseInt(fields[7], duration)) {
        return false;
    }

    record.gameId = fields[0];
    record.category = static_cast<GameCategory>(categoryValue);
    record.timestamp = timestamp;
    record.score = std::clamp(score, 0, 100);
    record.total = std::max(0, total);
    record.correct = std::clamp(correct, 0, record.total);
    record.difficulty = std::max(0, difficulty);
    record.durationSeconds = std::max(0, duration);
    return true;
}

}  // namespace

void Statistics::recordResult(const std::string& gameId, GameCategory category,
                              const GameResult& result, std::int64_t timestamp) {
    if (!isSafeGameId(gameId) || !isValidCategory(category)) return;

    const SessionRecord record = sanitizedRecord(gameId, category, result, timestamp);
    CategoryStats& categoryStats = categories_[static_cast<int>(category)];
    if (categoryStats.gamesPlayed == 0 || !std::isfinite(categoryStats.skill)) {
        categoryStats.skill = static_cast<float>(record.score);
    } else {
        categoryStats.skill = std::clamp(
            categoryStats.skill * 0.7f + static_cast<float>(record.score) * 0.3f,
            0.0f, 100.0f);
    }
    categoryStats.gamesPlayed = saturatingAdd(categoryStats.gamesPlayed, 1);
    totalGames_ = saturatingAdd(totalGames_, 1);

    GameStats& gameStats = gameStats_[gameId];
    if (gameStats.sessions == 0 || !std::isfinite(gameStats.averageScore)) {
        gameStats.bestScore = record.score;
        gameStats.averageScore = static_cast<float>(record.score);
    } else {
        gameStats.bestScore = std::max(gameStats.bestScore, record.score);
        const double accumulated =
            static_cast<double>(gameStats.averageScore) * gameStats.sessions;
        gameStats.averageScore = static_cast<float>(
            (accumulated + record.score) / (static_cast<double>(gameStats.sessions) + 1.0));
    }
    gameStats.mostRecentScore = record.score;
    gameStats.totalCorrect = saturatingAdd(gameStats.totalCorrect, record.correct);
    gameStats.totalAttempts = saturatingAdd(gameStats.totalAttempts, record.total);
    gameStats.bestDifficulty = std::max(gameStats.bestDifficulty, record.difficulty);
    gameStats.sessions = saturatingAdd(gameStats.sessions, 1);

    history_.push_back(record);
    if (history_.size() > kMaxHistory) {
        history_.erase(history_.begin(), history_.begin() +
                                             static_cast<std::ptrdiff_t>(history_.size() -
                                                                         kMaxHistory));
    }
}

SummaryStats Statistics::summaryForPeriod(std::int64_t startTimestamp,
                                          std::int64_t endTimestamp) const {
    SummaryStats summary;
    if (startTimestamp > endTimestamp) return summary;

    std::unordered_set<int> categories;
    std::unordered_set<std::int64_t> activeDays;
    double scoreTotal = 0.0;

    for (const SessionRecord& record : history_) {
        if (record.timestamp < startTimestamp || record.timestamp > endTimestamp) continue;

        summary.sessionsCompleted = saturatingAdd(summary.sessionsCompleted, 1);
        scoreTotal += record.score;
        summary.totalTrainingTimeSeconds =
            saturatingAdd(summary.totalTrainingTimeSeconds, record.durationSeconds);
        categories.insert(static_cast<int>(record.category));
        activeDays.insert(localDayKey(record.timestamp));
    }

    if (summary.sessionsCompleted > 0) {
        summary.averageScore =
            static_cast<float>(scoreTotal / static_cast<double>(summary.sessionsCompleted));
    }
    summary.categoriesTrained = static_cast<int>(categories.size());
    summary.activeDays = static_cast<int>(activeDays.size());
    return summary;
}

SummaryStats Statistics::dailySummary(std::int64_t now) const {
    const std::int64_t start = startOfLocalDay(now);
    return summaryForPeriod(start, shiftLocalDays(start, 1) - 1);
}

SummaryStats Statistics::weeklySummary(std::int64_t now) const {
    const std::int64_t today = startOfLocalDay(now);
    return summaryForPeriod(shiftLocalDays(today, -6), shiftLocalDays(today, 1) - 1);
}

SummaryStats Statistics::monthlySummary(std::int64_t now) const {
    const std::int64_t today = startOfLocalDay(now);
    return summaryForPeriod(shiftLocalDays(today, -29), shiftLocalDays(today, 1) - 1);
}

ChartSeries Statistics::prepareChartSeries(FilterType filter,
                                           const std::string& filterValue,
                                           std::int64_t now) const {
    ChartSeries series;
    const std::int64_t recentStart =
        filter == FilterType::Recent ? shiftLocalDays(startOfLocalDay(now), -29) : 0;

    for (const SessionRecord& record : history_) {
        bool matches = true;
        if (filter == FilterType::Game) matches = record.gameId == filterValue;
        if (filter == FilterType::Category) {
            matches = std::to_string(static_cast<int>(record.category)) == filterValue;
        }
        if (filter == FilterType::Recent) matches = record.timestamp >= recentStart;

        if (matches) {
            series.push_back({record.timestamp, static_cast<float>(record.score),
                              record.gameId, record.category});
        }
    }
    return series;
}

void Statistics::toMap(KeyValueMap& out) const {
    out["stats.total_games"] = std::to_string(totalGames_);

    for (int index = 0; index < kCategoryCount; ++index) {
        const std::string prefix = "stats.category." + std::to_string(index);
        out[prefix + ".skill"] = std::to_string(categories_[index].skill);
        out[prefix + ".games"] = std::to_string(categories_[index].gamesPlayed);
    }

    for (const auto& [gameId, stats] : gameStats_) {
        if (!isSafeGameId(gameId)) continue;
        const std::string prefix = "stats.game." + gameId;
        out[prefix + ".sessions"] = std::to_string(stats.sessions);
        out[prefix + ".bestScore"] = std::to_string(stats.bestScore);
        out[prefix + ".averageScore"] = std::to_string(stats.averageScore);
        out[prefix + ".mostRecentScore"] = std::to_string(stats.mostRecentScore);
        out[prefix + ".totalCorrect"] = std::to_string(stats.totalCorrect);
        out[prefix + ".totalAttempts"] = std::to_string(stats.totalAttempts);
        out[prefix + ".bestDifficulty"] = std::to_string(stats.bestDifficulty);
    }

    std::ostringstream joined;
    bool first = true;
    for (const SessionRecord& record : history_) {
        if (!isSafeGameId(record.gameId) || !isValidCategory(record.category)) continue;
        if (!first) joined << '|';
        first = false;
        joined << record.gameId << ',' << static_cast<int>(record.category) << ','
               << record.timestamp << ',' << record.score << ',' << record.correct << ','
               << record.total << ',' << record.difficulty << ',' << record.durationSeconds;
    }
    out["stats.history"] = joined.str();
}

void Statistics::fromMap(const KeyValueMap& in) {
    resetAll();

    if (const auto it = in.find("stats.total_games"); it != in.end()) {
        int value = 0;
        if (parseInt(it->second, value)) totalGames_ = std::max(0, value);
    }

    for (int index = 0; index < kCategoryCount; ++index) {
        const std::string prefix = "stats.category." + std::to_string(index);
        if (const auto it = in.find(prefix + ".skill"); it != in.end()) {
            float skill = 0.0f;
            if (parseFiniteFloat(it->second, skill)) {
                categories_[index].skill = std::clamp(skill, 0.0f, 100.0f);
            }
        }
        if (const auto it = in.find(prefix + ".games"); it != in.end()) {
            int games = 0;
            if (parseInt(it->second, games)) categories_[index].gamesPlayed = std::max(0, games);
        }
    }

    constexpr const char* kGamePrefix = "stats.game.";
    constexpr std::size_t kGamePrefixLength = 11;
    for (const auto& [key, value] : in) {
        if (key.rfind(kGamePrefix, 0) != 0) continue;
        const std::size_t fieldSeparator = key.find('.', kGamePrefixLength);
        if (fieldSeparator == std::string::npos) continue;

        const std::string gameId = key.substr(kGamePrefixLength,
                                              fieldSeparator - kGamePrefixLength);
        if (!isSafeGameId(gameId)) continue;
        const std::string field = key.substr(fieldSeparator + 1);
        GameStats& stats = gameStats_[gameId];

        int integerValue = 0;
        float floatValue = 0.0f;
        if (field == "averageScore") {
            if (parseFiniteFloat(value, floatValue)) {
                stats.averageScore = std::clamp(floatValue, 0.0f, 100.0f);
            }
        } else if (parseInt(value, integerValue)) {
            integerValue = std::max(0, integerValue);
            if (field == "sessions") stats.sessions = integerValue;
            else if (field == "bestScore") stats.bestScore = std::clamp(integerValue, 0, 100);
            else if (field == "mostRecentScore") {
                stats.mostRecentScore = std::clamp(integerValue, 0, 100);
            } else if (field == "totalCorrect") stats.totalCorrect = integerValue;
            else if (field == "totalAttempts") stats.totalAttempts = integerValue;
            else if (field == "bestDifficulty") stats.bestDifficulty = integerValue;
        }
    }

    for (auto& [gameId, stats] : gameStats_) {
        (void)gameId;
        stats.totalCorrect = std::min(stats.totalCorrect, stats.totalAttempts);
        if (stats.sessions == 0) stats = GameStats{};
    }

    if (const auto historyIt = in.find("stats.history"); historyIt != in.end()) {
        const std::string& serialized = historyIt->second;
        if (serialized.find('|') == std::string::npos) {
            SessionRecord singleRecord;
            if (parseSessionRecord(serialized, singleRecord)) {
                history_.push_back(singleRecord);
            } else {
                // Legacy versions stored only comma-separated score values.
                std::int64_t legacyTimestamp = 1;
                for (const std::string& value : split(serialized, ',')) {
                    float score = 0.0f;
                    if (!value.empty() && parseFiniteFloat(value, score)) {
                        SessionRecord record;
                        record.gameId = "legacy";
                        record.category = GameCategory::Memory;
                        record.timestamp = legacyTimestamp++;
                        record.score = std::clamp(static_cast<int>(score), 0, 100);
                        history_.push_back(record);
                    }
                }
            }
        } else {
            for (const std::string& serializedRecord : split(serialized, '|')) {
                SessionRecord record;
                if (parseSessionRecord(serializedRecord, record)) history_.push_back(record);
            }
        }

        if (history_.size() > kMaxHistory) {
            history_.erase(history_.begin(), history_.begin() +
                                                 static_cast<std::ptrdiff_t>(history_.size() -
                                                                             kMaxHistory));
        }
    }
}

void Statistics::resetAll() {
    categories_.fill(CategoryStats{});
    gameStats_.clear();
    history_.clear();
    totalGames_ = 0;
}

void Statistics::resetHistoryOnly() { history_.clear(); }

void Statistics::resetStatisticsOnly() {
    categories_.fill(CategoryStats{});
    gameStats_.clear();
    totalGames_ = 0;
}
