#pragma once

#include <cstdint>
#include <random>
#include <string>

#include "games/Game.h"

class NumberMemoryGame : public Game {
public:
    NumberMemoryGame();
    explicit NumberMemoryGame(std::uint32_t seed);

    void update(float deltaSeconds, const GameInput& input) override;
    void render(Renderer& renderer, const Rect& area) const override;
    bool isFinished() const override;
    GameResult result() const override;

    const std::string& currentSequence() const { return sequence_; }
    const std::string& currentRecallText() const { return recallText_; }
    int currentRound() const { return round_; }
    int successes() const { return successes_; }
    int sequenceLength() const { return sequenceLength_; }
    bool isMemorizing() const { return phase_ == Phase::Memorize; }
    bool isAwaitingRecall() const { return phase_ == Phase::Recall; }
    bool isShowingFeedback() const { return phase_ == Phase::Feedback; }

private:
    enum class Phase { Memorize, Recall, Feedback, Done };
    static constexpr int kTotalRounds = 5;
    static constexpr int kStartLength = 3;

    void startRound();
    float memorizeSeconds() const;
    void applyTextInput(const GameInput& input);
    void submitRecall();

    std::mt19937 rng_;
    Phase phase_ = Phase::Memorize;
    int round_ = 0;
    int successes_ = 0;
    int sequenceLength_ = kStartLength;
    std::string sequence_;
    std::string recallText_;
    bool recallFocused_ = false;
    bool lastRoundCorrect_ = false;
    float phaseTimer_ = 0.0f;
};
