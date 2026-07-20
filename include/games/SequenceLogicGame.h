#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <string>

#include "games/Game.h"

class SequenceLogicGame : public Game {
public:
    SequenceLogicGame();
    explicit SequenceLogicGame(std::uint32_t seed);
    // startingDifficulty is the persisted adaptive level (see
    // GameStats::difficultyLevel); it is clamped to [0, kMaxDifficulty] and
    // widens the generated ranges / narrows the distractor spread.
    SequenceLogicGame(std::uint32_t seed, int startingDifficulty);

    void update(float deltaSeconds, const GameInput& input) override;
    void render(Renderer& renderer, const Rect& area) const override;
    bool isFinished() const override;
    GameResult result() const override;

    const std::string& currentSequence() const { return sequenceText_; }
    const std::array<int, 4>& currentOptions() const { return options_; }
    int currentRound() const { return round_; }
    int correctCount() const { return correctCount_; }
    bool isAwaitingChoice() const { return phase_ == Phase::Question; }
    bool isShowingFeedback() const { return phase_ == Phase::Feedback; }

private:
    enum class Phase { Question, Feedback, Done };
    static constexpr int kTotalRounds = 8;
    static constexpr int kOptionCount = 4;
    static constexpr int kMaxDifficulty = 8;
    static constexpr int kCorrectStreakToAdvance = 3;
    static constexpr int kWrongStreakToRetreat = 2;

    void nextRound();
    void chooseOption(int index);

    std::mt19937 rng_;
    Phase phase_ = Phase::Question;
    int difficultyOffset_ = 0;
    int sessionBump_ = 0;
    int consecutiveCorrect_ = 0;
    int consecutiveWrong_ = 0;
    int round_ = 0;
    int correctCount_ = 0;
    std::string sequenceText_;
    std::array<int, kOptionCount> options_{};
    int correctOption_ = 0;
    int chosenOption_ = -1;
    float feedbackTimer_ = 0.0f;
};
