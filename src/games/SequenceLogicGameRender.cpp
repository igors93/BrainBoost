#include "games/SequenceLogicGame.h"

#include <cstdio>

#include "games/GameLayout.h"
#include "ui/Renderer.h"
#include "ui/Widgets.h"

void SequenceLogicGame::render(Renderer& renderer, const Rect& area) const {
    const float cx = area.centerX();
    char progress[32];
    std::snprintf(progress, sizeof(progress), "Rodada %d de %d", round_ + 1,
                  kTotalRounds);
    renderer.drawTextCentered(phase_ == Phase::Done ? "Fim de jogo!" : progress,
                              cx, area.y + 16, 15, Theme::kTextMuted);
    renderer.drawTextCentered("Qual é o próximo número?", cx, area.y + 55, 16,
                              Theme::kTextMuted);
    renderer.drawTextCentered(sequenceText_, cx, area.y + 85, 40,
                              Theme::kAccent, true);
    if (phase_ == Phase::Done) return;

    for (int i = 0; i < kOptionCount; ++i) {
        Color background = Theme::kButton;
        if (phase_ == Phase::Feedback) {
            if (i == correctOption_) background = rgb(0x15803D);
            else if (i == chosenOption_) background = rgb(0x991B1B);
        }
        Widgets::drawButton(renderer, GameLayout::sequenceOptionButton(area, i),
                            std::to_string(options_[i]), background, 22);
    }
}
