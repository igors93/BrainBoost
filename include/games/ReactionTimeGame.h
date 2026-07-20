#pragma once

#include <cstdint>
#include <memory>
#include <random>
#include <vector>

#include "games/Game.h"
#include "games/GameClock.h"

class ReactionTimeGame : public Game {
public:
    ReactionTimeGame();
    explicit ReactionTimeGame(std::uint32_t seed);
    ReactionTimeGame(std::uint32_t seed,
                     std::shared_ptr<const GameClock> clock);
    // startingDifficulty is the persisted adaptive level (see
    // GameStats::difficultyLevel); it is clamped to [0, kMaxDifficulty] and
    // shortens the minimum wait before "go", raising anticipation demand.
    ReactionTimeGame(std::uint32_t seed, int startingDifficulty);
    ReactionTimeGame(std::uint32_t seed, std::shared_ptr<const GameClock> clock,
                     int startingDifficulty);

    void update(float deltaSeconds, const GameInput& input) override;
    void render(Renderer& renderer, const Rect& area) const override;
    bool isFinished() const override;
    GameResult result() const override;

    bool isWaiting() const { return phase_ == Phase::Waiting; }
    bool isReady() const { return phase_ == Phase::Go; }
    bool isShowingResult() const { return phase_ == Phase::TrialResult; }
    bool isShowingTooEarly() const { return phase_ == Phase::TooEarly; }
    int completedTrials() const { return trial_; }
    int falseStarts() const { return falseStarts_; }
    float waitingSecondsRemaining() const { return waitTimer_; }
    float averageReactionMs() const;

private:
    enum class Phase { Instructions, Waiting, Go, TrialResult, TooEarly, Done };
    static constexpr int kTotalTrials = 5;
    static constexpr int kMaxDifficulty = 10;
    static constexpr int kCleanStreakToAdvance = 2;

    void startTrial();
    void finishPauseOrGame();

    std::mt19937 rng_;
    std::shared_ptr<const GameClock> clock_;
    int startingDifficulty_ = 0;
    int sessionBump_ = 0;
    int consecutiveClean_ = 0;
    Phase phase_ = Phase::Instructions;
    int trial_ = 0;
    int falseStarts_ = 0;
    float waitTimer_ = 0.0f;
    double goStartedAtSeconds_ = 0.0;
    float pauseTimer_ = 0.0f;
    std::vector<float> reactionTimesMs_;
};
