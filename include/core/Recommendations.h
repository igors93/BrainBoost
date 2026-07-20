#pragma once

#include <cstdint>
#include <string>
#include <vector>

class GameRegistry;
class Statistics;
struct GameInfo;

// Turns raw stats into "what should you play next": the category skill and
// per-game staleness already tracked by Statistics, but never used to guide
// the player before now (see HomeScreen's "Treino de hoje").
namespace Recommendations {

// Higher means more worth playing next. Weak categories and long-neglected
// games score higher; a never-implemented game ("Em breve") always scores
// below every playable one. Exposed mainly so HomeScreen's full catalog grid
// can sort by the same signal used to build dailyTraining().
float priorityScore(const GameInfo& info, const Statistics& stats, std::int64_t now);

struct Entry {
    const GameInfo* game = nullptr;
    std::string reason;  // short pt-BR hint shown under the card
};

// Up to maxCount picks, favoring category diversity: the single
// highest-priority game from each distinct category first, then the next
// highest-priority games overall to fill any remaining slots. Only
// implemented (playable) games are considered.
std::vector<Entry> dailyTraining(const GameRegistry& registry, const Statistics& stats,
                                 std::int64_t now, std::size_t maxCount = 3);

}  // namespace Recommendations
