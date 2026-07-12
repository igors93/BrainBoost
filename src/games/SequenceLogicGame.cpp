#include "games/SequenceLogicGame.h"

#include <algorithm>
#include <cstdio>
#include <set>

#include "ui/Input.h"
#include "ui/Renderer.h"
#include "ui/Widgets.h"

SequenceLogicGame::SequenceLogicGame() : rng_(std::random_device{}()) {
    nextRound();
}

void SequenceLogicGame::nextRound() {
    std::uniform_int_distribution<int> ruleDist(0, 2);
    std::uniform_int_distribution<int> startDist(1, 12);

    constexpr int kShownTerms = 4;
    int terms[kShownTerms + 1];
    int value = startDist(rng_);

    switch (ruleDist(rng_)) {
        case 0: {  // arithmetic: +k
            std::uniform_int_distribution<int> step(2, 9);
            const int k = step(rng_);
            for (int& term : terms) {
                term = value;
                value += k;
            }
            break;
        }
        case 1: {  // geometric: *2
            for (int& term : terms) {
                term = value;
                value *= 2;
            }
            break;
        }
        default: {  // alternating steps: +a, +b, +a, ...
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
    std::snprintf(buffer, sizeof(buffer), "%d   %d   %d   %d   ?", terms[0], terms[1],
                  terms[2], terms[3]);
    sequenceText_ = buffer;

    // Correct answer plus three unique nearby distractors.
    const int answer = terms[kShownTerms];
    std::set<int> values{answer};
    std::uniform_int_distribution<int> offset(-6, 6);
    while (values.size() < kOptionCount) {
        const int candidate = answer + offset(rng_);
        if (candidate > 0) values.insert(candidate);
    }

    int index = 0;
    for (int v : values) options_[index++] = v;
    std::shuffle(options_.begin(), options_.end(), rng_);
    correctOption_ = static_cast<int>(
        std::find(options_.begin(), options_.end(), answer) - options_.begin());
    chosenOption_ = -1;
}

void SequenceLogicGame::frame(float deltaSeconds, Renderer& renderer,
                              const Input& input, const Rect& area) {
    if (phase_ == Phase::Feedback) {
        feedbackTimer_ -= deltaSeconds;
        if (feedbackTimer_ <= 0.0f) {
            ++round_;
            if (round_ >= kTotalRounds) {
                phase_ = Phase::Done;
            } else {
                phase_ = Phase::Question;
                nextRound();
            }
        }
    }

    const float cx = area.centerX();

    char progress[32];
    std::snprintf(progress, sizeof(progress), "Rodada %d de %d", round_ + 1,
                  kTotalRounds);
    renderer.drawTextCentered(phase_ == Phase::Done ? "Fim de jogo!" : progress, cx,
                              area.y + 16, 15, Theme::kTextMuted);

    renderer.drawTextCentered("Qual é o próximo número?", cx, area.y + 55, 16,
                              Theme::kTextMuted);
    renderer.drawTextCentered(sequenceText_, cx, area.y + 85, 40, Theme::kAccent, true);

    if (phase_ == Phase::Done) return;

    // Option buttons in a centered row. During feedback the correct answer
    // turns green and a wrong pick turns red.
    const float buttonWidth = 110.0f;
    const float buttonHeight = 52.0f;
    const float spacing = 14.0f;
    const float rowWidth =
        kOptionCount * buttonWidth + (kOptionCount - 1) * spacing;
    float x = cx - rowWidth * 0.5f;
    const float y = area.y + 165;

    for (int i = 0; i < kOptionCount; ++i) {
        Color background = Theme::kButton;
        if (phase_ == Phase::Feedback) {
            if (i == correctOption_) background = rgb(0x15803D);
            else if (i == chosenOption_) background = rgb(0x991B1B);
        }

        const Rect rect{x, y, buttonWidth, buttonHeight};
        const bool clicked = Widgets::button(renderer, input, rect,
                                             std::to_string(options_[i]), background, 22);
        if (clicked && phase_ == Phase::Question) {
            chosenOption_ = i;
            if (chosenOption_ == correctOption_) ++correctCount_;
            phase_ = Phase::Feedback;
            feedbackTimer_ = 0.9f;
        }
        x += buttonWidth + spacing;
    }
}

bool SequenceLogicGame::isFinished() const { return phase_ == Phase::Done; }

GameResult SequenceLogicGame::result() const {
    GameResult result;
    result.correct = correctCount_;
    result.total = kTotalRounds;
    result.score = correctCount_ * 100 / kTotalRounds;
    result.xpEarned = correctCount_ * 18;
    return result;
}
