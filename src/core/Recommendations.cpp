#include "core/Recommendations.h"

#include <algorithm>
#include <cstdint>

#include "core/GameInfo.h"
#include "core/GameRegistry.h"
#include "core/Statistics.h"

namespace Recommendations {

namespace {

constexpr std::int64_t kSecondsPerDay = 86400;
// Beyond two weeks, every game is treated as equally "very stale": staleness
// alone should nudge a pick, not dominate a session that has been untouched
// for months versus one untouched for weeks.
constexpr double kStalenessCapDays = 14.0;

double staleDays(const GameStats& game, std::int64_t now) {
    if (game.lastPlayedTimestamp <= 0) return kStalenessCapDays;  // never played
    const std::int64_t elapsedSeconds =
        std::max<std::int64_t>(0, now - game.lastPlayedTimestamp);
    const double days = static_cast<double>(elapsedSeconds) / kSecondsPerDay;
    return std::min(kStalenessCapDays, days);
}

std::string reasonFor(const GameInfo& info, const Statistics& stats, std::int64_t now) {
    const GameStats& game = stats.forGame(info.id);
    if (game.lastPlayedTimestamp <= 0) return "Experimente pela primeira vez";

    if (stats.forCategory(info.category).skill < 60.0f) {
        return std::string("Fortaleça ") + categoryName(info.category);
    }

    const std::int64_t rawDays = std::max<std::int64_t>(
        0, (now - game.lastPlayedTimestamp) / kSecondsPerDay);
    if (rawDays >= 1) {
        return "Há " + std::to_string(rawDays) +
               (rawDays == 1 ? " dia sem treinar" : " dias sem treinar");
    }
    return "Continue praticando";
}

}  // namespace

float priorityScore(const GameInfo& info, const Statistics& stats, std::int64_t now) {
    if (!info.isImplemented()) return -1.0f;

    const float skillGap = 100.0f - stats.forCategory(info.category).skill;  // 0..100
    const float stalenessScore =
        static_cast<float>(staleDays(stats.forGame(info.id), now) / kStalenessCapDays * 100.0);
    return skillGap * 0.6f + stalenessScore * 0.4f;
}

std::vector<Entry> dailyTraining(const GameRegistry& registry, const Statistics& stats,
                                 std::int64_t now, std::size_t maxCount) {
    std::vector<const GameInfo*> candidates;
    for (const GameInfo& info : registry.games()) {
        if (info.isImplemented()) candidates.push_back(&info);
    }
    std::stable_sort(candidates.begin(), candidates.end(),
                     [&](const GameInfo* a, const GameInfo* b) {
                         return priorityScore(*a, stats, now) > priorityScore(*b, stats, now);
                     });

    std::vector<Entry> result;
    std::vector<GameCategory> usedCategories;

    // First pass: the top pick from each distinct category, so a short
    // "workout" naturally spans different skills instead of stacking three
    // games from the single weakest one.
    for (const GameInfo* info : candidates) {
        if (result.size() >= maxCount) break;
        if (std::find(usedCategories.begin(), usedCategories.end(), info->category) !=
            usedCategories.end()) {
            continue;
        }
        usedCategories.push_back(info->category);
        result.push_back({info, reasonFor(*info, stats, now)});
    }

    // Second pass: fill any remaining slots with the next-highest-priority
    // games overall, once every category has already contributed one.
    for (const GameInfo* info : candidates) {
        if (result.size() >= maxCount) break;
        const bool alreadyPicked = std::any_of(
            result.begin(), result.end(), [&](const Entry& entry) { return entry.game == info; });
        if (!alreadyPicked) result.push_back({info, reasonFor(*info, stats, now)});
    }

    return result;
}

}  // namespace Recommendations
