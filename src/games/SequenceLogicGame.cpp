#include "games/SequenceLogicGame.h"

#include <algorithm>
#include <cstdio>
#include <set>

namespace {

constexpr float kButtonWidth = 110.0f;
constexpr float kButtonHeight = 52.0f;
constexpr float kButtonSpacing = 14.0f;

Rect optionRect(const Rect& area, int index) {
    constexpr int kCount = 4;
    const float rowWidth =
        kCount * kButtonWidth + (kCount - 1) * kButtonSpacing;
    const float firstX = area.centerX() - rowWidth * 0.5f;
    return Rect{firstX + index * (kButtonWidth + kButtonSpacing),
                area.y + 165.0f, kButtonWidth, kButtonHeight};
}

}  // namespace

SequenceLogicGame::SequenceLogicGame() : rng_(std::random_device{}()) {
    nextRound();
}

SequenceLogicGame::SequenceLogicGame(std::uint32_t seed) : rng_(seed) {
    nextRound();
}

void SequenceLogicGame::nextRound() {
    std::uniform_int_distribution<int> ruleDist(0, 2);
    std::uniform_int_distribution<int> startDist(1, 12);

    constexpr int kShownTerms = 4;
    int terms[kShownTerms + 1];
    int value = startDist(rng_);

    switch (ruleDist(rng_)) {
        case 0: {
            std::uniform_int_distribution<int> step(2, 9);
            const int k = step(rng_);
            for (int& term : terms) {
                term = value;
                value += k;
            }
            break;
        }
        case 1:
            for (int& term : terms) {
                term = value;
                value *= 2;
            }
            break;
        default: {
            std::uniform_int_distribution<int> step(1, 6);
            const int a = step(rng_);
            const int b = a + step(rng_);
            for (int i = 0; i <= kShownTerms; ++i) {
                terms[i] = value;
                value += (i % 2 == 0) ? a : b;
            }
            break;
        }
    }

    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "%d   %d   %d   %d   ?", terms[0],
                  terms[1], terms[2], terms[3]);
    sequenceText_ = buffer;

    const int answer = terms[kShownTerms];
    std::set<int> values{answer};
    std::uniform_int_distribution<int> offset(-6, 6);
    int attempts = 0;
    while (values.size() < kOptionCount && attempts < 100) {
        ++attempts;
        const int candidate = answer + offset(rng_);
        if (candidate > 0) values.insert(candidate);
    }
    // Deterministic fallback prevents an accidental infinite loop.
    for (int delta = 1; values.size() < kOptionCount; ++delta) {
        values.insert(answer + delta);
    }

    int index = 0;
    for (int option : values) options_[index++] = option;
    std::shuffle(options_.begin(), options_.end(), rng_);
    correctOption_ = static_cast<int>(
        std::find(options_.begin(), options_.end(), answer) - options_.begin());
    chosenOption_ = -1;
}

void SequenceLogicGame::chooseOption(int index) {
    if (phase_ != Phase::Question || index < 0 || index >= kOptionCount) return;

    chosenOption_ = index;
    if (chosenOption_ == correctOption_) ++correctCount_;
    phase_ = Phase::Feedback;
    feedbackTimer_ = 0.9f;
}

void SequenceLogicGame::update(float deltaSeconds, const GameInput& input,
                               const Rect& area) {
    if (phase_ == Phase::Done) return;

    if (phase_ == Phase::Feedback) {
        feedbackTimer_ -= std::max(0.0f, deltaSeconds);
        if (feedbackTimer_ <= 0.0f) {
            ++round_;
            if (round_ >= kTotalRounds) {
                phase_ = Phase::Done;
            } else {
                phase_ = Phase::Question;
                nextRound();
            }
        }
        return;
    }

    if (input.optionIndex >= 0) {
        chooseOption(input.optionIndex);
        return;
    }

    if (!input.primaryPressed) return;
    for (int i = 0; i < kOptionCount; ++i) {
        if (optionRect(area, i).contains(input.pointerX, input.pointerY)) {
            chooseOption(i);
            return;
        }
    }
}

bool SequenceLogicGame::isFinished() const { return phase_ == Phase::Done; }

GameResult SequenceLogicGame::result() const {
    GameResult result;
    result.correct = correctCount_;
    result.total = kTotalRounds;
    result.score = correctCount_ * 100 / kTotalRounds;
    result.xpEarned = correctCount_ * 18;
    result.difficulty = round_ / 3;
    return result;
}
