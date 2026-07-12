#pragma once

#include <chrono>
#include <random>
#include <vector>

#include "games/Game.h"

// "Reação Rápida": wait for the panel to turn green and click as fast as
// possible. Clicking too early cancels the trial. Score is based on the
// average reaction time across all trials.
class ReactionTimeGame : public Game {
public:
    ReactionTimeGame();

    void frame(float deltaSeconds, Renderer& renderer, const Input& input,
               const Rect& area) override;
    bool isFinished() const override;
    GameResult result() const override;

private:
    enum class Phase { Instructions, Waiting, Go, TrialResult, TooEarly, Done };
    static constexpr int kTotalTrials = 5;

    void startTrial();
    float averageMs() const;

    std::mt19937 rng_;
    Phase phase_ = Phase::Instructions;
    int trial_ = 0;

    float waitTimer_ = 0.0f;      // countdown until the panel turns green
    std::chrono::steady_clock::time_point goTime_;  // time when it turned green
    float pauseTimer_ = 0.0f;     // short pause between trials
    std::vector<float> reactionTimesMs_;
};
