#pragma once

#include <random>
#include <string>

#include "games/Game.h"
#include "ui/Widgets.h"

// "Memória Numérica": a digit sequence is shown briefly, then the player
// must type it back. The sequence grows after every correct round.
class NumberMemoryGame : public Game {
public:
    NumberMemoryGame();
    explicit NumberMemoryGame(std::uint32_t seed);

    void frame(float deltaSeconds, Renderer& renderer, const Input& input,
               const Rect& area) override;
    bool isFinished() const override;
    GameResult result() const override;

private:
    enum class Phase { Memorize, Recall, Feedback, Done };
    static constexpr int kTotalRounds = 5;
    static constexpr int kStartLength = 3;

    void startRound();
    float memorizeSeconds() const;

    std::mt19937 rng_;
    Phase phase_ = Phase::Memorize;
    int round_ = 0;
    int successes_ = 0;
    int sequenceLength_ = kStartLength;

    std::string sequence_;
    Widgets::TextFieldState recallField_;
    bool lastRoundCorrect_ = false;
    float phaseTimer_ = 0.0f;
};
