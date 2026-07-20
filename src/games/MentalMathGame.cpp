#include "games/MentalMathGame.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

MentalMathGame::MentalMathGame() : rng_(std::random_device{}()) { nextQuestion(); }
MentalMathGame::MentalMathGame(std::uint32_t seed) : rng_(seed) { nextQuestion(); }

MentalMathGame::MentalMathGame(std::uint32_t seed, int startingDifficulty)
    : rng_(seed), levelOffset_(std::clamp(startingDifficulty, 0, kMaxDifficulty)) {
    nextQuestion();
}

void MentalMathGame::nextQuestion() {
    const int level = std::max(0, levelOffset_ + sessionBump_);
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
    if (input.backspacePressed && !answerText_.empty()) answerText_.pop_back();
}

void MentalMathGame::submitAnswer() {
    if (answerText_.empty()) return;
    lastAnswerCorrect_ = (std::atoi(answerText_.c_str()) == expectedAnswer_);
    // Bidirectional intra-session staircase: a short correct streak raises
    // the level right away, but it takes a shorter wrong streak to give it
    // back, so the session keeps adapting in real time instead of only
    // ramping up on a fixed question schedule.
    if (lastAnswerCorrect_) {
        ++correctCount_;
        ++consecutiveCorrect_;
        consecutiveWrong_ = 0;
        if (consecutiveCorrect_ >= kCorrectStreakToAdvance) {
            ++sessionBump_;
            consecutiveCorrect_ = 0;
        }
    } else {
        consecutiveCorrect_ = 0;
        if (++consecutiveWrong_ >= kWrongStreakToRetreat) {
            --sessionBump_;
            consecutiveWrong_ = 0;
        }
    }
    answerFocused_ = false;
    phase_ = Phase::Feedback;
    feedbackTimer_ = 0.8f;
}

void MentalMathGame::update(float deltaSeconds, const GameInput& input) {
    if (phase_ == Phase::Done) return;
    if (phase_ == Phase::Feedback) {
        feedbackTimer_ -= std::max(0.0f, deltaSeconds);
        if (feedbackTimer_ <= 0.0f) {
            ++questionIndex_;
            if (questionIndex_ >= kTotalQuestions) phase_ = Phase::Done;
            else {
                phase_ = Phase::Question;
                nextQuestion();
            }
        }
        return;
    }

    if (input.focusTextField) answerFocused_ = true;
    if (input.blurTextField) answerFocused_ = false;
    applyTextInput(input);

    if (input.submitPressed || (answerFocused_ && input.confirmPressed)) {
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
    result.difficulty = std::max(0, levelOffset_ + sessionBump_);
    return result;
}
