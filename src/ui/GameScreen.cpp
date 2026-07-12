#include "ui/GameScreen.h"

#include <cstdio>

#include "app/AppContext.h"
#include "ui/Input.h"
#include "ui/Renderer.h"
#include "ui/Widgets.h"

void GameScreen::renderResults(AppContext& context, Renderer& renderer,
                               const Input& input, const Rect& area) {
    context.applyResultOnce();
    const GameResult result = context.activeGame->result();
    const float cx = area.centerX();
    float y = area.y + 30;

    renderer.drawTextCentered("Sessão concluída!", cx, y, 24, Theme::kSuccess, true);
    y += 44;

    char score[32];
    std::snprintf(score, sizeof(score), "%d pontos", result.score);
    renderer.drawTextCentered(score, cx, y, 42, Theme::kText, true);
    y += 62;

    char details[64];
    std::snprintf(details, sizeof(details), "Acertos: %d de %d", result.correct,
                  result.total);
    renderer.drawTextCentered(details, cx, y, 15, Theme::kTextMuted);
    y += 30;

    char xp[32];
    std::snprintf(xp, sizeof(xp), "+%d XP", result.xpEarned);
    renderer.drawTextCentered(xp, cx, y, 24, Theme::kAccent, true);
    y += 40;

    for (const AchievementDef* achievement : context.lastUnlocks) {
        char unlocked[128];
        std::snprintf(unlocked, sizeof(unlocked),
                      "Conquista desbloqueada: %s (+%d XP)",
                      achievement->title.c_str(), achievement->xpReward);
        renderer.drawTextCentered(unlocked, cx, y, 15, Theme::kWarning);
        y += 24;
    }

    y += 16;
    const float buttonWidth = 190.0f;
    const float spacing = 12.0f;
    if (Widgets::button(renderer, input,
                        Rect{cx - buttonWidth - spacing * 0.5f, y, buttonWidth, 46},
                        "Jogar novamente", Theme::kButton, 16)) {
        context.startGame(*context.activeGameInfo);
        return;
    }
    if (Widgets::button(renderer, input,
                        Rect{cx + spacing * 0.5f, y, buttonWidth, 46},
                        "Voltar ao início", Theme::kButton, 16)) {
        context.closeGame(ScreenId::Home);
    }
}

void GameScreen::render(AppContext& context, Renderer& renderer, const Input& input,
                        const Rect& area, float deltaSeconds) {
    if (!context.activeGame || context.activeGameInfo == nullptr) {
        renderer.drawText("Nenhum jogo ativo.", area.x, area.y, 20, Theme::kText, true);
        if (Widgets::button(renderer, input, Rect{area.x, area.y + 40, 190, 44},
                            "Voltar ao início")) {
            context.screen = ScreenId::Home;
        }
        return;
    }

    // Header: title + category on the left, exit button on the right.
    renderer.drawText(context.activeGameInfo->title, area.x, area.y, 26, Theme::kText,
                      true);
    renderer.drawText(categoryName(context.activeGameInfo->category), area.x,
                      area.y + 38, 14,
                      Theme::categoryColor(context.activeGameInfo->category));
    if (Widgets::button(renderer, input, Rect{area.right() - 130, area.y, 130, 38},
                        "Sair do jogo", Theme::kButton, 15)) {
        context.closeGame(ScreenId::Home);
        return;
    }

    const Rect panel{area.x, area.y + 66, area.w, area.h - 66};
    renderer.fillRect(panel, Theme::kPanel);

    if (context.activeGame->isFinished()) {
        renderResults(context, renderer, input, panel);
    } else {
        context.activeGame->frame(deltaSeconds, renderer, input, panel);
    }
}
