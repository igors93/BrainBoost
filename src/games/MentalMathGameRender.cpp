#include "games/MentalMathGame.h"

#include <cstdio>

#include "games/GameLayout.h"
#include "ui/Renderer.h"
#include "ui/Widgets.h"

void MentalMathGame::render(Renderer& renderer, const Rect& area) const {
    const float cx = area.centerX();
    char progress[32];
    std::snprintf(progress, sizeof(progress), "Questão %d de %d",
                  questionIndex_ + 1, kTotalQuestions);
    renderer.drawTextCentered(phase_ == Phase::Done ? "Fim de jogo!" : progress,
                              cx, area.y + 16, 15, Theme::kTextMuted);
    renderer.drawTextCentered(questionText_, cx, area.y + 70, 42, Theme::kText,
                              true);

    if (phase_ == Phase::Question) {
        Widgets::drawTextField(renderer, GameLayout::mentalMathAnswerField(area),
                               answerText_, answerFocused_, 22);
        Widgets::drawButton(renderer, GameLayout::mentalMathSubmitButton(area),
                            "Responder", Theme::kButton, 17);
    } else if (phase_ == Phase::Feedback) {
        if (lastAnswerCorrect_) {
            renderer.drawTextCentered("Correto!", cx, area.y + 165, 24,
                                      Theme::kSuccess, true);
        } else {
            char text[64];
            std::snprintf(text, sizeof(text), "Errado! Resposta: %d",
                          expectedAnswer_);
            renderer.drawTextCentered(text, cx, area.y + 165, 24,
                                      Theme::kDanger, true);
        }
    }
}
