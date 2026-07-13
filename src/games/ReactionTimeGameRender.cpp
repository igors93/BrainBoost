#include "games/ReactionTimeGame.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include "games/GameLayout.h"
#include "ui/Renderer.h"
#include "ui/Widgets.h"

void ReactionTimeGame::render(Renderer& renderer, const Rect& area) const {
    const float cx = area.centerX();
    char progress[48];
    std::snprintf(progress, sizeof(progress), "Tentativa %d de %d",
                  std::min(trial_ + 1, kTotalTrials), kTotalTrials);
    renderer.drawTextCentered(phase_ == Phase::Done ? "Fim de jogo!" : progress,
                              cx, area.y + 16, 15, Theme::kTextMuted);

    if (phase_ == Phase::Instructions) {
        renderer.drawTextCentered("Clique no painel assim que ele ficar VERDE.",
                                  cx, area.y + 70, 22, Theme::kText, true);
        renderer.drawTextCentered(
            "Clicar cedo demais consome a tentativa atual.", cx, area.y + 105,
            15, Theme::kTextMuted);
        Widgets::drawButton(renderer, GameLayout::reactionStartButton(area),
                            "Começar", Theme::kButton, 17);
        return;
    }
    if (phase_ == Phase::Done) return;

    const Rect panel = GameLayout::reactionPanel(area);
    Color panelColor = rgb(0x7F1D1D);
    std::string panelText = "Aguarde...";
    if (phase_ == Phase::Go) {
        panelColor = rgb(0x15803D);
        panelText = "CLIQUE AGORA!";
    } else if (phase_ == Phase::TooEarly) {
        panelColor = rgb(0x92400E);
        panelText = "Muito cedo! Tentativa consumida.";
    } else if (phase_ == Phase::TrialResult) {
        panelColor = rgb(0x1E3A8A);
        char time[32];
        std::snprintf(time, sizeof(time), "%.0f ms", reactionTimesMs_.back());
        panelText = time;
    }
    renderer.fillRect(panel, panelColor);
    renderer.drawTextCentered(panelText, panel.centerX(),
                              panel.centerY() - renderer.lineHeight(36) * 0.5f,
                              36, Theme::kText, true);
}
