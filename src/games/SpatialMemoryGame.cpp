#include "games/SpatialMemoryGame.h"
#include <algorithm>

SpatialMemoryGame::SpatialMemoryGame() : rng_(std::random_device{}()) { startRound(); }
SpatialMemoryGame::SpatialMemoryGame(std::uint32_t seed) : rng_(seed) { startRound(); }

SpatialMemoryGame::SpatialMemoryGame(std::uint32_t seed, int startingDifficulty)
    : rng_(seed),
      sequenceLength_(kStartLength +
                      std::clamp(startingDifficulty, 0, kMaxDifficulty)) {
    startRound();
}

void SpatialMemoryGame::startRound() {
    std::uniform_int_distribution<int> cell(0, 8);
    sequence_.clear();
    for (int i = 0; i < sequenceLength_; ++i) {
        sequence_.push_back(cell(rng_));
    }
    sequenceIndex_ = 0;
    userIndex_ = 0;
    phase_ = Phase::StartDelay;
    phaseTimer_ = 0.5f;
}

void SpatialMemoryGame::update(float deltaSeconds, const GameInput& input) {
    if (phase_ == Phase::Done) return;
    const float safeDelta = std::max(0.0f, deltaSeconds);

    hoverIndex_ = input.hoverIndex;
    if (clickTimer_ > 0.0f) {
        clickTimer_ -= safeDelta;
    }

    if (phase_ == Phase::StartDelay) {
        phaseTimer_ -= safeDelta;
        if (phaseTimer_ <= 0.0f) {
            phase_ = Phase::ShowSequence;
            phaseTimer_ = 0.8f; // 0.6s show + 0.2s pause
        }
        return;
    }

    if (phase_ == Phase::ShowSequence) {
        phaseTimer_ -= safeDelta;
        if (phaseTimer_ <= 0.0f) {
            ++sequenceIndex_;
            if (sequenceIndex_ >= sequenceLength_) {
                phase_ = Phase::Recall;
            } else {
                phaseTimer_ = 0.8f;
            }
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

    if (phase_ == Phase::Recall && input.optionIndex >= 0) {
        clickIndex_ = input.optionIndex;
        clickTimer_ = 0.2f;

        if (input.optionIndex == sequence_[userIndex_]) {
            ++userIndex_;
            if (userIndex_ >= sequenceLength_) {
                lastRoundCorrect_ = true;
                ++successes_;
                phase_ = Phase::Feedback;
                phaseTimer_ = 1.0f;
            }
        } else {
            lastRoundCorrect_ = false;
            phase_ = Phase::Feedback;
            phaseTimer_ = 1.5f;
        }
    }
}

bool SpatialMemoryGame::isFinished() const { return phase_ == Phase::Done; }

GameResult SpatialMemoryGame::result() const {
    GameResult result;
    result.correct = successes_;
    result.total = kTotalRounds;
    result.score = successes_ * 100 / kTotalRounds;
    result.difficulty = std::max(0, sequenceLength_ - kStartLength);
    if (successes_ > 0) {
        result.xpEarned = successes_ * 20 + result.difficulty * 5;
    }
    return result;
}
