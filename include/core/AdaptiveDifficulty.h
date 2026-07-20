#pragma once

#include <algorithm>

// Central adaptation rule shared by every game: keeps each game's persisted
// difficulty level inside a "desirable difficulty" band instead of every
// session restarting from the same fixed baseline. GameStats::difficultyLevel
// is the state this advances; Statistics::recordResult() is the only caller.
namespace AdaptiveDifficulty {

// Session score (0..100) bands that trigger a step. Between the two
// thresholds the level is left unchanged: a single average session should
// not nudge the player up or down.
inline constexpr int kRaiseScoreThreshold = 80;
inline constexpr int kLowerScoreThreshold = 50;
inline constexpr float kStep = 1.0f;
inline constexpr float kMinDifficulty = 0.0f;

// currentDifficulty is the level the just-finished session was played at;
// score is that session's normalized 0..100 result. Returns the level the
// *next* session at this game should start from: one step up after a strong
// session, one step down (floored at kMinDifficulty) after a weak one.
inline float nextDifficulty(float currentDifficulty, int score) {
    if (score >= kRaiseScoreThreshold) return currentDifficulty + kStep;
    if (score <= kLowerScoreThreshold) {
        return std::max(kMinDifficulty, currentDifficulty - kStep);
    }
    return currentDifficulty;
}

}  // namespace AdaptiveDifficulty
