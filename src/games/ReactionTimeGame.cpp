#include "games/ReactionTimeGame.h"

#include <algorithm>
#include <cstdio>
#include <numeric>

#include "ui/Input.h"
#include "ui/Renderer.h"
#include "ui/Widgets.h"

ReactionTimeGame::ReactionTimeGame() : rng_(std::random_device{}()) {}

void ReactionTimeGame::startTrial() {
    std::uniform_real_distribution<float> delay(1.5f, 3.5f);
    waitTimer_ = delay(rng_);
    reactionTimer_ = 0.0f;
    phase_ = Phase::Waiting;
}

float ReactionTimeGame::averageMs() const {
    if (reactionTimesMs_.empty()) return 0.0f;
    const float sum =
        std::accumulate(reactionTimesMs_.begin(), reactionTimesMs_.end(), 0.0f);
    return sum / static_cast<float>(reactionTimesMs_.size());
}

void ReactionTimeGame::frame(float deltaSeconds, Renderer& renderer,
                             const Input& input, const Rect& area) {
    // --- State updates -----------------------------------------------------
    switch (phase_) {
        case Phase::Waiting:
            waitTimer_ -= deltaSeconds;
            if (waitTimer_ <= 0.0f) phase_ = Phase::Go;
            break;
        case Phase::Go:
            reactionTimer_ += deltaSeconds;
            break;
        case Phase::TrialResult:
        case Phase::TooEarly:
            pauseTimer_ -= deltaSeconds;
            if (pauseTimer_ <= 0.0f) {
                if (trial_ >= kTotalTrials) {
                    phase_ = Phase::Done;
                } else {
                    startTrial();
                }
            }
            break;
        default:
            break;
    }

    // --- Drawing -----------------------------------------------------------
    const float cx = area.centerX();

    char progress[48];
    std::snprintf(progress, sizeof(progress), "Tentativa %d de %d",
                  std::min(trial_ + 1, kTotalTrials), kTotalTrials);
    renderer.drawTextCentered(phase_ == Phase::Done ? "Fim de jogo!" : progress, cx,
                              area.y + 16, 15, Theme::kTextMuted);

    if (phase_ == Phase::Instructions) {
        renderer.drawTextCentered("Clique no painel assim que ele ficar VERDE.", cx,
                                  area.y + 70, 22, Theme::kText, true);
        renderer.drawTextCentered("Cuidado: clicar cedo demais cancela a tentativa!",
                                  cx, area.y + 105, 15, Theme::kTextMuted);
        const Rect start{cx - 110, area.y + 150, 220, 46};
        if (Widgets::button(renderer, input, start, "Começar", Theme::kButton, 17)) {
            startTrial();
        }
        return;
    }
    if (phase_ == Phase::Done) return;

    // Big clickable panel filling the rest of the area.
    Color panelColor = rgb(0x7F1D1D);  // waiting: red
    std::string panelText = "Aguarde...";
    if (phase_ == Phase::Go) {
        panelColor = rgb(0x15803D);  // go: green
        panelText = "CLIQUE AGORA!";
    } else if (phase_ == Phase::TooEarly) {
        panelColor = rgb(0x92400E);
        panelText = "Muito cedo!";
    } else if (phase_ == Phase::TrialResult) {
        panelColor = rgb(0x1E3A8A);
        char time[32];
        std::snprintf(time, sizeof(time), "%.0f ms", reactionTimesMs_.back());
        panelText = time;
    }

    const Rect panel{area.x + 16, area.y + 48, area.w - 32, area.h - 64};
    renderer.fillRect(panel, panelColor);
    renderer.drawTextCentered(panelText, panel.centerX(),
                              panel.centerY() - renderer.lineHeight(36) * 0.5f, 36,
                              Theme::kText, true);

    const bool clicked =
        input.mousePressed() && panel.contains(input.mouseX(), input.mouseY());
    if (!clicked) return;

    if (phase_ == Phase::Waiting) {
        phase_ = Phase::TooEarly;
        pauseTimer_ = 1.2f;
    } else if (phase_ == Phase::Go) {
        reactionTimesMs_.push_back(reactionTimer_ * 1000.0f);
        ++trial_;
        phase_ = Phase::TrialResult;
        pauseTimer_ = 1.2f;
    }
}

bool ReactionTimeGame::isFinished() const { return phase_ == Phase::Done; }

GameResult ReactionTimeGame::result() const {
    GameResult result;
    result.correct = static_cast<int>(reactionTimesMs_.size());
    result.total = kTotalTrials;

    // ~200 ms averages score near 100; ~600 ms approaches the minimum.
    const float avg = averageMs();
    result.score = static_cast<int>(std::clamp((650.0f - avg) / 4.5f, 5.0f, 100.0f));
    result.xpEarned = result.score / 2 + 20;
    return result;
}
