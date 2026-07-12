#pragma once

#include <cstdint>
#include <random>
#include <string>

#include "games/Game.h"

// "Cálculo Mental": solve a series of arithmetic operations as fast as
// possible. Difficulty ramps up with each question.
class MentalMathGame : public Game {
public:
    MentalMathGame();
    explicit MentalMathGame(std::uint32_t seed);

    void update(float deltaSeconds, const GameInput& input,
                const Rect& area) override;
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

    void nextQuestion();
    void submitAnswer();
    void applyTextInput(const GameInput& input);

    std::mt19937 rng_;
    Phase phase_ = Phase::Question;
    int questionIndex_ = 0;
    int correctCount_ = 0;

    std::string questionText_;
    int expectedAnswer_ = 0;
    std::string answerText_;
    bool answerFocused_ = true;
    bool lastAnswerCorrect_ = false;
    float feedbackTimer_ = 0.0f;
};
