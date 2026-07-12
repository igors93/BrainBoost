#include "games/NumberMemoryGame.h"

#include <algorithm>
#include <cctype>

namespace {

Rect recallFieldRect(const Rect& area) {
    return Rect{area.centerX() - 130.0f, area.y + 100.0f, 260.0f, 46.0f};
}

Rect confirmButtonRect(const Rect& area) {
    const Rect field = recallFieldRect(area);
    return Rect{area.centerX() - 110.0f, field.bottom() + 16.0f, 220.0f, 44.0f};
}

}  // namespace

NumberMemoryGame::NumberMemoryGame() : rng_(std::random_device{}()) {
    startRound();
}

NumberMemoryGame::NumberMemoryGame(std::uint32_t seed) : rng_(seed) {
    startRound();
}

float NumberMemoryGame::memorizeSeconds() const {
    return 1.2f + 0.45f * static_cast<float>(sequenceLength_);
}

void NumberMemoryGame::startRound() {
    std::uniform_int_distribution<int> digit(0, 9);
    sequence_.clear();
    for (int i = 0; i < sequenceLength_; ++i) {
        sequence_ += static_cast<char>('0' + digit(rng_));
    }

    recallText_.clear();
    recallFocused_ = false;
    phase_ = Phase::Memorize;
    phaseTimer_ = memorizeSeconds();
}

void NumberMemoryGame::applyTextInput(const GameInput& input) {
    if (!recallFocused_) return;

    for (char character : input.text) {
        if (recallText_.size() >= 20) break;
        if (std::isdigit(static_cast<unsigned char>(character)) != 0) {
            recallText_ += character;
        }
    }
    if (input.backspacePressed && !recallText_.empty()) recallText_.pop_back();
}

void NumberMemoryGame::submitRecall() {
    if (recallText_.empty()) return;

    lastRoundCorrect_ = (sequence_ == recallText_);
    if (lastRoundCorrect_) ++successes_;
    recallFocused_ = false;
    phase_ = Phase::Feedback;
    phaseTimer_ = 1.1f;
}

void NumberMemoryGame::update(float deltaSeconds, const GameInput& input,
                              const Rect& area) {
    if (phase_ == Phase::Done) return;

    const float safeDelta = std::max(0.0f, deltaSeconds);
    if (phase_ == Phase::Memorize) {
        phaseTimer_ -= safeDelta;
        if (phaseTimer_ <= 0.0f) {
            phase_ = Phase::Recall;
            recallFocused_ = true;
        }
        return;
    }

    if (phase_ == Phase::Feedback) {
        phaseTimer_ -= safeDelta;
        if (phaseTimer_ <= 0.0f) {
            ++round_;
            if (round_ >= kTotalRounds) {
                phase_ = Phase::Done;
            } else {
                if (lastRoundCorrect_) ++sequenceLength_;
                startRound();
            }
        }
        return;
    }

    const Rect field = recallFieldRect(area);
    const Rect confirm = confirmButtonRect(area);
    if (input.primaryPressed) {
        recallFocused_ = field.contains(input.pointerX, input.pointerY);
    }
    if (input.cancelPressed) recallFocused_ = false;

    applyTextInput(input);

    const bool clickedConfirm =
        input.primaryPressed && confirm.contains(input.pointerX, input.pointerY);
    const bool keyboardConfirm = recallFocused_ && input.confirmPressed;
    if (input.submitPressed || clickedConfirm || keyboardConfirm) {
        submitRecall();
    }
}

bool NumberMemoryGame::isFinished() const { return phase_ == Phase::Done; }

GameResult NumberMemoryGame::result() const {
    GameResult result;
    result.correct = successes_;
    result.total = kTotalRounds;
    result.score = successes_ * 100 / kTotalRounds;
    result.xpEarned = successes_ * 15 + sequenceLength_ * 5;
    result.difficulty = std::max(0, sequenceLength_ - kStartLength);
    return result;
}
