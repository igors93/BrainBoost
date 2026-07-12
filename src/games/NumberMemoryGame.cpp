#include "games/NumberMemoryGame.h"

#include <cstdio>

#include "ui/Input.h"
#include "ui/Renderer.h"

NumberMemoryGame::NumberMemoryGame() : rng_(std::random_device{}()) {
    startRound();
}

float NumberMemoryGame::memorizeSeconds() const {
    // Longer sequences stay on screen a bit longer.
    return 1.2f + 0.45f * static_cast<float>(sequenceLength_);
}

void NumberMemoryGame::startRound() {
    std::uniform_int_distribution<int> digit(0, 9);
    sequence_.clear();
    for (int i = 0; i < sequenceLength_; ++i) {
        sequence_ += static_cast<char>('0' + digit(rng_));
    }

    recallField_.text.clear();
    phase_ = Phase::Memorize;
    phaseTimer_ = memorizeSeconds();
}

void NumberMemoryGame::frame(float deltaSeconds, Renderer& renderer,
                             const Input& input, const Rect& area) {
    // --- State updates -----------------------------------------------------
    if (phase_ == Phase::Memorize) {
        phaseTimer_ -= deltaSeconds;
        if (phaseTimer_ <= 0.0f) phase_ = Phase::Recall;
    } else if (phase_ == Phase::Feedback) {
        phaseTimer_ -= deltaSeconds;
        if (phaseTimer_ <= 0.0f) {
            ++round_;
            if (round_ >= kTotalRounds) {
                phase_ = Phase::Done;
            } else {
                if (lastRoundCorrect_) ++sequenceLength_;
                startRound();
            }
        }
    }

    // --- Drawing -----------------------------------------------------------
    const float cx = area.centerX();

    char progress[32];
    std::snprintf(progress, sizeof(progress), "Rodada %d de %d", round_ + 1,
                  kTotalRounds);
    renderer.drawTextCentered(phase_ == Phase::Done ? "Fim de jogo!" : progress, cx,
                              area.y + 16, 15, Theme::kTextMuted);

    switch (phase_) {
        case Phase::Memorize: {
            renderer.drawTextCentered("Memorize a sequência:", cx, area.y + 60, 16,
                                      Theme::kTextMuted);
            renderer.drawTextCentered(sequence_, cx, area.y + 95, 42, Theme::kAccent,
                                      true);
            Widgets::progressBar(renderer,
                                 Rect{cx - 160, area.y + 170, 320, 8},
                                 phaseTimer_ / memorizeSeconds());
            break;
        }
        case Phase::Recall: {
            renderer.drawTextCentered("Digite a sequência que você viu:", cx,
                                      area.y + 60, 16, Theme::kTextMuted);
            const Rect field{cx - 130, area.y + 100, 260, 46};
            const bool submitted =
                Widgets::textField(renderer, input, recallField_, field, 22, true, 20);

            const Rect confirm{cx - 110, field.bottom() + 16, 220, 44};
            if (Widgets::button(renderer, input, confirm, "Confirmar", Theme::kButton,
                                17) ||
                submitted) {
                lastRoundCorrect_ = (sequence_ == recallField_.text);
                if (lastRoundCorrect_) ++successes_;
                phase_ = Phase::Feedback;
                phaseTimer_ = 1.1f;
            }
            break;
        }
        case Phase::Feedback: {
            if (lastRoundCorrect_) {
                renderer.drawTextCentered("Correto!", cx, area.y + 110, 24,
                                          Theme::kSuccess, true);
            } else {
                char text[64];
                std::snprintf(text, sizeof(text), "Errado! Era: %s", sequence_.c_str());
                renderer.drawTextCentered(text, cx, area.y + 110, 24, Theme::kDanger,
                                          true);
            }
            break;
        }
        case Phase::Done:
            break;
    }
}

bool NumberMemoryGame::isFinished() const { return phase_ == Phase::Done; }

GameResult NumberMemoryGame::result() const {
    GameResult result;
    result.correct = successes_;
    result.total = kTotalRounds;
    result.score = successes_ * 100 / kTotalRounds;
    result.xpEarned = successes_ * 15 + sequenceLength_ * 5;
    return result;
}
