#include "games/MentalMathGame.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace {

Rect answerFieldRect(const Rect& area) {
    return Rect{area.centerX() - 110.0f, area.y + 150.0f, 220.0f, 46.0f};
}

Rect submitButtonRect(const Rect& area) {
    const Rect field = answerFieldRect(area);
    return Rect{area.centerX() - 110.0f, field.bottom() + 16.0f, 220.0f, 44.0f};
}

void eraseLastCharacter(std::string& text) {
    if (!text.empty()) text.pop_back();
}

}  // namespace

MentalMathGame::MentalMathGame() : rng_(std::random_device{}()) {
    nextQuestion();
}

MentalMathGame::MentalMathGame(std::uint32_t seed) : rng_(seed) {
    nextQuestion();
}

void MentalMathGame::nextQuestion() {
    const int level = questionIndex_ / 3;
    std::uniform_int_distribution<int> operand(1, 10 + level * 15);
    std::uniform_int_distribution<int> operation(0, 2);

    int a = operand(rng_);
    int b = operand(rng_);
    char symbol = '+';

    switch (operation(rng_)) {
        case 0:
            expectedAnswer_ = a + b;
            break;
        case 1:
            symbol = '-';
            if (a < b) std::swap(a, b);
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
    answerText_.clear();
    answerFocused_ = true;
}

void MentalMathGame::applyTextInput(const GameInput& input) {
    if (!answerFocused_) return;

    for (char character : input.text) {
        if (answerText_.size() >= 9) break;
        if (std::isdigit(static_cast<unsigned char>(character)) != 0) {
            answerText_ += character;
        }
    }
    if (input.backspacePressed) eraseLastCharacter(answerText_);
}

void MentalMathGame::submitAnswer() {
    if (answerText_.empty()) return;

    lastAnswerCorrect_ = (std::atoi(answerText_.c_str()) == expectedAnswer_);
    if (lastAnswerCorrect_) ++correctCount_;

    answerFocused_ = false;
    phase_ = Phase::Feedback;
    feedbackTimer_ = 0.8f;
}

void MentalMathGame::update(float deltaSeconds, const GameInput& input,
                            const Rect& area) {
    if (phase_ == Phase::Done) return;

    if (phase_ == Phase::Feedback) {
        feedbackTimer_ -= std::max(0.0f, deltaSeconds);
        if (feedbackTimer_ <= 0.0f) {
            ++questionIndex_;
            if (questionIndex_ >= kTotalQuestions) {
                phase_ = Phase::Done;
            } else {
                phase_ = Phase::Question;
                nextQuestion();
            }
        }
        return;
    }

    const Rect field = answerFieldRect(area);
    const Rect submit = submitButtonRect(area);

    if (input.primaryPressed) {
        answerFocused_ = field.contains(input.pointerX, input.pointerY);
    }
    if (input.cancelPressed) answerFocused_ = false;

    applyTextInput(input);

    const bool clickedSubmit =
        input.primaryPressed && submit.contains(input.pointerX, input.pointerY);
    const bool submittedByKeyboard = answerFocused_ && input.confirmPressed;
    if (input.submitPressed || clickedSubmit || submittedByKeyboard) {
        submitAnswer();
    }
}

bool MentalMathGame::isFinished() const { return phase_ == Phase::Done; }

GameResult MentalMathGame::result() const {
    GameResult result;
    result.correct = correctCount_;
    result.total = kTotalQuestions;
    result.score = correctCount_ * 100 / kTotalQuestions;
    result.xpEarned =
        correctCount_ * 15 + (correctCount_ == kTotalQuestions ? 20 : 0);
    result.difficulty = questionIndex_ / 3;
    return result;
}
