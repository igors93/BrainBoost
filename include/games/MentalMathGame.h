#pragma once

#include <cstdint>
#include <random>
#include <string>

#include "games/Game.h"

class MentalMathGame : public Game {
public:
    MentalMathGame();
    explicit MentalMathGame(std::uint32_t seed);
    // startingDifficulty is the persisted adaptive level (see
    // GameStats::difficultyLevel); it is clamped to [0, kMaxDifficulty] and
    // added on top of the level questionIndex_ would otherwise reach.
    MentalMathGame(std::uint32_t seed, int startingDifficulty);

    void update(float deltaSeconds, const GameInput& input) override;
    void render(Renderer& renderer, const Rect& area) const override;
    bool isFinished() const override;
    GameResult result() const override;

    const std::string& currentQuestion() const { return questionText_; }
    const std::string& currentAnswerText() const { return answerText_; }
    int currentQuestionIndex() const { return questionIndex_; }
    int correctCount() const { return correctCount_; }
    bool isAwaitingAnswer() const { return phase_ == Phase::Question; }
    bool isShowingFeedback() const { return phase_ == Phase::Feedback; }

private:
    enum class Phase { Question, Feedback, Done };
    static constexpr int kTotalQuestions = 10;
    static constexpr int kMaxDifficulty = 10;
    static constexpr int kCorrectStreakToAdvance = 3;
    static constexpr int kWrongStreakToRetreat = 2;

    void nextQuestion();
    void submitAnswer();
    void applyTextInput(const GameInput& input);

    std::mt19937 rng_;
    Phase phase_ = Phase::Question;
    int levelOffset_ = 0;
    int sessionBump_ = 0;
    int consecutiveCorrect_ = 0;
    int consecutiveWrong_ = 0;
    int questionIndex_ = 0;
    int correctCount_ = 0;
    std::string questionText_;
    int expectedAnswer_ = 0;
    std::string answerText_;
    bool answerFocused_ = true;
    bool lastAnswerCorrect_ = false;
    float feedbackTimer_ = 0.0f;
};
