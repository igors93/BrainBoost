#pragma once

#include <array>
#include <random>
#include <string>

#include "games/Game.h"

// "Sequência Lógica": a numeric sequence follows a hidden rule; pick the
// next term among four options.
class SequenceLogicGame : public Game {
public:
    SequenceLogicGame();
    explicit SequenceLogicGame(std::uint32_t seed);

    void frame(float deltaSeconds, Renderer& renderer, const Input& input,
               const Rect& area) override;
    bool isFinished() const override;
    GameResult result() const override;

private:
    enum class Phase { Question, Feedback, Done };
    static constexpr int kTotalRounds = 8;
    static constexpr int kOptionCount = 4;

    void nextRound();

    std::mt19937 rng_;
    Phase phase_ = Phase::Question;
    int round_ = 0;
    int correctCount_ = 0;

    std::string sequenceText_;
    std::array<int, kOptionCount> options_{};
    int correctOption_ = 0;
    int chosenOption_ = -1;
    float feedbackTimer_ = 0.0f;
};
