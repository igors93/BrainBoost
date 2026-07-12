#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "games/Game.h"

// "Reação Rápida": wait for the panel to turn green and click as fast as
// possible. Clicking too early consumes the current trial. Score is based on
// the average reaction time across valid trials.
class ReactionTimeGame : public Game {
public:
    ReactionTimeGame();
    explicit ReactionTimeGame(std::uint32_t seed);

    void update(float deltaSeconds, const GameInput& input,
                const Rect& area) override;
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

    void startTrial();
    void finishPauseOrGame();

    std::mt19937 rng_;
    Phase phase_ = Phase::Instructions;
    int trial_ = 0;
    int falseStarts_ = 0;

    float waitTimer_ = 0.0f;
    float goElapsedSeconds_ = 0.0f;
    float pauseTimer_ = 0.0f;
    std::vector<float> reactionTimesMs_;
};
