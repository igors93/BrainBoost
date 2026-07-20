#include "games/SpatialMemoryGame.h"

#include <cstdio>

#include "games/GameLayout.h"
#include "ui/Renderer.h"

void SpatialMemoryGame::render(Renderer& renderer, const Rect& area) const {
    const float cx = area.centerX();
    char progress[32];
    std::snprintf(progress, sizeof(progress), "Rodada %d de %d", round_ + 1, kTotalRounds);
    renderer.drawTextCentered(phase_ == Phase::Done ? "Fim de jogo!" : progress,
                              cx, area.y + 16, 15, Theme::kTextMuted);

    if (phase_ == Phase::StartDelay || phase_ == Phase::ShowSequence) {
        renderer.drawTextCentered("Memorize a sequência:", cx, area.y + 60, 16, Theme::kTextMuted);
    } else if (phase_ == Phase::Recall) {
        renderer.drawTextCentered("Repita a sequência clicando nos quadrados:", cx, area.y + 60, 16, Theme::kTextMuted);
    } else if (phase_ == Phase::Feedback) {
        if (lastRoundCorrect_) {
            renderer.drawTextCentered("Correto!", cx, area.y + 60, 24, Theme::kSuccess, true);
        } else {
            renderer.drawTextCentered("Errado!", cx, area.y + 60, 24, Theme::kDanger, true);
        }
    }

    int activeIndex = -1;
    if (phase_ == Phase::ShowSequence && phaseTimer_ > 0.2f) {
        activeIndex = sequence_[sequenceIndex_];
    }

    for (int i = 0; i < 9; ++i) {
        Rect cell = GameLayout::spatialMemoryGridCell(area, i);
        Color color = Theme::kButton;
        Color outlineColor = Theme::kGrid;
        int outlineThickness = 2;
        
        if (i == activeIndex) {
            color = Theme::kAccent;
        } else if (i == clickIndex_ && clickTimer_ > 0.0f) {
            color = Theme::kSuccess;
        } else if (i == hoverIndex_ && phase_ == Phase::Recall) {
            color = lighten(Theme::kButton, 14);
            outlineColor = Theme::kAccent;
            outlineThickness = 3;
        }
        
        renderer.fillRect(cell, color);
        renderer.outlineRect(cell, outlineColor, outlineThickness);
    }
}
