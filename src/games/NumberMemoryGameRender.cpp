#include "games/NumberMemoryGame.h"

#include <algorithm>
#include <cstdio>

#include "ui/Renderer.h"
#include "ui/Widgets.h"

namespace {

Rect recallFieldRect(const Rect& area) {
    return Rect{area.centerX() - 130.0f, area.y + 100.0f, 260.0f, 46.0f};
}

Rect confirmButtonRect(const Rect& area) {
    const Rect field = recallFieldRect(area);
    return Rect{area.centerX() - 110.0f, field.bottom() + 16.0f, 220.0f, 44.0f};
}

}  // namespace

void NumberMemoryGame::render(Renderer& renderer, const Rect& area) const {
    const float cx = area.centerX();

    char progress[32];
    std::snprintf(progress, sizeof(progress), "Rodada %d de %d", round_ + 1,
                  kTotalRounds);
    renderer.drawTextCentered(phase_ == Phase::Done ? "Fim de jogo!" : progress,
                              cx, area.y + 16, 15, Theme::kTextMuted);

    switch (phase_) {
        case Phase::Memorize:
            renderer.drawTextCentered("Memorize a sequência:", cx, area.y + 60,
                                      16, Theme::kTextMuted);
            renderer.drawTextCentered(sequence_, cx, area.y + 95, 42,
                                      Theme::kAccent, true);
            Widgets::progressBar(
                renderer, Rect{cx - 160, area.y + 170, 320, 8},
                phaseTimer_ / std::max(0.001f, memorizeSeconds()));
            break;
        case Phase::Recall:
            renderer.drawTextCentered("Digite a sequência que você viu:", cx,
                                      area.y + 60, 16, Theme::kTextMuted);
            Widgets::drawTextField(renderer, recallFieldRect(area), recallText_,
                                   recallFocused_, 22);
            Widgets::drawButton(renderer, confirmButtonRect(area), "Confirmar",
                                Theme::kButton, 17);
            break;
        case Phase::Feedback:
            if (lastRoundCorrect_) {
                renderer.drawTextCentered("Correto!", cx, area.y + 110, 24,
                                          Theme::kSuccess, true);
            } else {
                char text[64];
                std::snprintf(text, sizeof(text), "Errado! Era: %s",
                              sequence_.c_str());
                renderer.drawTextCentered(text, cx, area.y + 110, 24,
                                          Theme::kDanger, true);
            }
            break;
        case Phase::Done:
            break;
    }
}
