#include "games/ReactionTimeGame.h"

#include <algorithm>
#include <numeric>
#include <utility>

ReactionTimeGame::ReactionTimeGame()
    : rng_(std::random_device{}()), clock_(std::make_shared<SteadyGameClock>()) {}

ReactionTimeGame::ReactionTimeGame(std::uint32_t seed)
    : rng_(seed), clock_(std::make_shared<SteadyGameClock>()) {}

ReactionTimeGame::ReactionTimeGame(std::uint32_t seed,
                                   std::shared_ptr<const GameClock> clock)
    : rng_(seed), clock_(std::move(clock)) {
    if (!clock_) clock_ = std::make_shared<SteadyGameClock>();
}

ReactionTimeGame::ReactionTimeGame(std::uint32_t seed, int startingDifficulty)
    : rng_(seed),
      clock_(std::make_shared<SteadyGameClock>()),
      startingDifficulty_(std::clamp(startingDifficulty, 0, kMaxDifficulty)) {}

ReactionTimeGame::ReactionTimeGame(std::uint32_t seed,
                                   std::shared_ptr<const GameClock> clock,
                                   int startingDifficulty)
    : rng_(seed),
      clock_(std::move(clock)),
      startingDifficulty_(std::clamp(startingDifficulty, 0, kMaxDifficulty)) {
    if (!clock_) clock_ = std::make_shared<SteadyGameClock>();
}

void ReactionTimeGame::startTrial() {
    // Higher difficulty shortens the minimum wait, making the "go" moment
    // less predictable and raising the anticipation-inhibition demand.
    // sessionBump_ lets this breathe within the session too: a false start
    // eases it back immediately, a short clean streak tightens it further.
    const int effectiveDifficulty = std::max(0, startingDifficulty_ + sessionBump_);
    const float minDelay =
        std::max(0.5f, 1.5f - 0.08f * static_cast<float>(effectiveDifficulty));
    std::uniform_real_distribution<float> delay(minDelay, 3.5f);
    waitTimer_ = delay(rng_);
    phase_ = Phase::Waiting;
}

float ReactionTimeGame::averageReactionMs() const {
    if (reactionTimesMs_.empty()) return 0.0f;
    const float sum =
        std::accumulate(reactionTimesMs_.begin(), reactionTimesMs_.end(), 0.0f);
    return sum / static_cast<float>(reactionTimesMs_.size());
}

void ReactionTimeGame::finishPauseOrGame() {
    if (trial_ >= kTotalTrials) phase_ = Phase::Done;
    else startTrial();
}

void ReactionTimeGame::update(float deltaSeconds, const GameInput& input) {
    if (phase_ == Phase::Done) return;
    const float safeDelta = std::max(0.0f, deltaSeconds);

    if (phase_ == Phase::Instructions) {
        if (input.startPressed || input.confirmPressed) {
            startTrial();
            return;
        }
    } else if (phase_ == Phase::Waiting && input.primaryActionPressed) {
        ++trial_;
        ++falseStarts_;
        // A false start is an unambiguous "too hard right now" signal: ease
        // off immediately instead of waiting for a streak.
        consecutiveClean_ = 0;
        --sessionBump_;
        phase_ = Phase::TooEarly;
        pauseTimer_ = 1.2f;
        return;
    } else if (phase_ == Phase::Go && input.primaryActionPressed) {
        const double elapsed =
            std::max(0.0, clock_->nowSeconds() - goStartedAtSeconds_);
        reactionTimesMs_.push_back(static_cast<float>(elapsed * 1000.0));
        ++trial_;
        ++consecutiveClean_;
        if (consecutiveClean_ >= kCleanStreakToAdvance) {
            ++sessionBump_;
            consecutiveClean_ = 0;
        }
        phase_ = Phase::TrialResult;
        pauseTimer_ = 1.2f;
        return;
    }

    switch (phase_) {
        case Phase::Waiting:
            waitTimer_ -= safeDelta;
            if (waitTimer_ <= 0.0f) {
                waitTimer_ = 0.0f;
                goStartedAtSeconds_ = clock_->nowSeconds();
                phase_ = Phase::Go;
            }
            break;
        case Phase::TrialResult:
        case Phase::TooEarly:
            pauseTimer_ -= safeDelta;
            if (pauseTimer_ <= 0.0f) finishPauseOrGame();
            break;
        default:
            break;
    }
}

bool ReactionTimeGame::isFinished() const { return phase_ == Phase::Done; }

GameResult ReactionTimeGame::result() const {
    GameResult result;
    result.correct = static_cast<int>(reactionTimesMs_.size());
    result.total = kTotalTrials;
    result.difficulty = std::max(0, startingDifficulty_ + sessionBump_);

    if (reactionTimesMs_.empty()) return result;

    const float avg = averageReactionMs();
    result.score = static_cast<int>(
        std::clamp((650.0f - avg) / 4.5f, 5.0f, 100.0f));
    const int baseXp = result.score / 2 + result.correct * 4;
    result.xpEarned = std::max(0, baseXp - falseStarts_ * 5);
    return result;
}
