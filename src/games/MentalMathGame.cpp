#include "games/MentalMathGame.h"

#include <cstdio>
#include <cstdlib>

#include "ui/Input.h"
#include "ui/Renderer.h"

MentalMathGame::MentalMathGame() : rng_(std::random_device{}()) {
    nextQuestion();
}

MentalMathGame::MentalMathGame(std::uint32_t seed) : rng_(seed) {
    nextQuestion();
}

void MentalMathGame::nextQuestion() {
    // Ranges grow with the question index to ramp up difficulty.
    const int level = questionIndex_ / 3;
    std::uniform_int_distribution<int> operand(1, 10 + level * 15);
    std::uniform_int_distribution<int> operation(0, 2);

    int a = operand(rng_);
    int b = operand(rng_);
    char symbol = '+';

    switch (operation(rng_)) {
        case 0:
            symbol = '+';
            expectedAnswer_ = a + b;
            break;
        case 1:
            symbol = '-';
            if (a < b) std::swap(a, b);  // keep results non-negative
            expectedAnswer_ = a - b;
            break;
        default: {
            symbol = 'x';
            std::uniform_int_distribution<int> small(2, 9 + level);
            a = small(rng_);
            b = small(rng_);
            expectedAnswer_ = a * b;
            break;
        }
    }

    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%d %c %d = ?", a, symbol, b);
    questionText_ = buffer;
    answerField_.text.clear();
}

void MentalMathGame::submitAnswer() {
    if (answerField_.text.empty()) return;

    lastAnswerCorrect_ = (std::atoi(answerField_.text.c_str()) == expectedAnswer_);
    if (lastAnswerCorrect_) ++correctCount_;

    phase_ = Phase::Feedback;
    feedbackTimer_ = 0.8f;
}

void MentalMathGame::frame(float deltaSeconds, Renderer& renderer, const Input& input,
                           const Rect& area) {
    if (phase_ == Phase::Feedback) {
        feedbackTimer_ -= deltaSeconds;
        if (feedbackTimer_ <= 0.0f) {
            ++questionIndex_;
            if (questionIndex_ >= kTotalQuestions) {
                phase_ = Phase::Done;
            } else {
                phase_ = Phase::Question;
                nextQuestion();
            }
        }
    }

    const float cx = area.centerX();

    char progress[32];
    std::snprintf(progress, sizeof(progress), "Questão %d de %d", questionIndex_ + 1,
                  kTotalQuestions);
    renderer.drawTextCentered(phase_ == Phase::Done ? "Fim de jogo!" : progress, cx,
                              area.y + 16, 15, Theme::kTextMuted);

    renderer.drawTextCentered(questionText_, cx, area.y + 70, 42, Theme::kText, true);

    if (phase_ == Phase::Question) {
        const Rect field{cx - 110, area.y + 150, 220, 46};
        const bool submitted =
            Widgets::textField(renderer, input, answerField_, field, 22, true, 9);

        const Rect submit{cx - 110, field.bottom() + 16, 220, 44};
        if (Widgets::button(renderer, input, submit, "Responder", Theme::kButton, 17) ||
            submitted) {
            submitAnswer();
        }
    } else if (phase_ == Phase::Feedback) {
        if (lastAnswerCorrect_) {
            renderer.drawTextCentered("Correto!", cx, area.y + 165, 24, Theme::kSuccess,
                                      true);
        } else {
            char text[64];
            std::snprintf(text, sizeof(text), "Errado! Resposta: %d", expectedAnswer_);
            renderer.drawTextCentered(text, cx, area.y + 165, 24, Theme::kDanger, true);
        }
    }
}

bool MentalMathGame::isFinished() const { return phase_ == Phase::Done; }

GameResult MentalMathGame::result() const {
    GameResult result;
    result.correct = correctCount_;
    result.total = kTotalQuestions;
    result.score = correctCount_ * 100 / kTotalQuestions;
    result.xpEarned = correctCount_ * 15 + (correctCount_ == kTotalQuestions ? 20 : 0);
    return result;
}
