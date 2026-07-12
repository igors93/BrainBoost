#pragma once

#include <random>
#include <string>

#include "games/Game.h"
#include "ui/Widgets.h"

// "Cálculo Mental": solve a series of arithmetic operations as fast as
// possible. Difficulty ramps up with each question.
class MentalMathGame : public Game {
public:
    MentalMathGame();
    explicit MentalMathGame(std::uint32_t seed);

    void frame(float deltaSeconds, Renderer& renderer, const Input& input,
               const Rect& area) override;
    bool isFinished() const override;
    GameResult result() const override;

private:
    enum class Phase { Question, Feedback, Done };
    static constexpr int kTotalQuestions = 10;

    void nextQuestion();
    void submitAnswer();

    std::mt19937 rng_;
    Phase phase_ = Phase::Question;
    int questionIndex_ = 0;
    int correctCount_ = 0;

    std::string questionText_;
    int expectedAnswer_ = 0;
    Widgets::TextFieldState answerField_;
    bool lastAnswerCorrect_ = false;
    float feedbackTimer_ = 0.0f;
};
