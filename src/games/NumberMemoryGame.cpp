#include "games/NumberMemoryGame.h"

#include <algorithm>
#include <cctype>

NumberMemoryGame::NumberMemoryGame() : rng_(std::random_device{}()) { startRound(); }
NumberMemoryGame::NumberMemoryGame(std::uint32_t seed) : rng_(seed) { startRound(); }

NumberMemoryGame::NumberMemoryGame(std::uint32_t seed, int startingDifficulty)
    : rng_(seed),
      sequenceLength_(kStartLength +
                      std::clamp(startingDifficulty, 0, kMaxDifficulty)) {
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

void NumberMemoryGame::update(float deltaSeconds, const GameInput& input) {
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
            if (round_ >= kTotalRounds) phase_ = Phase::Done;
            else {
                // Bidirectional staircase: a hit grows the sequence right
                // away, but only sustained misses shrink it back, so one
                // unlucky round does not immediately punish the player.
                if (lastRoundCorrect_) {
                    ++sequenceLength_;
                    consecutiveMisses_ = 0;
                } else if (++consecutiveMisses_ >= kMissesToRetreat) {
                    sequenceLength_ = std::max(kMinLength, sequenceLength_ - 1);
                    consecutiveMisses_ = 0;
                }
                startRound();
            }
        }
        return;
    }

    if (input.focusTextField) recallFocused_ = true;
    if (input.blurTextField) recallFocused_ = false;
    applyTextInput(input);
    if (input.submitPressed || (recallFocused_ && input.confirmPressed)) {
        submitRecall();
    }
}

bool NumberMemoryGame::isFinished() const { return phase_ == Phase::Done; }

GameResult NumberMemoryGame::result() const {
    GameResult result;
    result.correct = successes_;
    result.total = kTotalRounds;
    result.score = successes_ * 100 / kTotalRounds;
    result.difficulty = std::max(0, sequenceLength_ - kStartLength);
    if (successes_ > 0) {
        result.xpEarned = successes_ * 15 + result.difficulty * 5;
    }
    return result;
}
