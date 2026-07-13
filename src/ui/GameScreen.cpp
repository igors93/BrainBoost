#include "ui/GameScreen.h"

#include <cstdio>

#include "app/AppContext.h"
#include "games/GameInput.h"
#include "games/GameLayout.h"
#include "ui/Input.h"
#include "ui/Renderer.h"
#include "ui/Widgets.h"

namespace {

GameInput makeGameInput(const Input& input, const std::string& gameId,
                        const Rect& panel) {
    GameInput gameInput;
    gameInput.confirmPressed = input.enterPressed();
    gameInput.backspacePressed = input.backspacePressed();
    gameInput.text = input.textTyped();

    if (!input.mousePressed()) {
        if (gameId == "quick_reaction" && input.enterPressed()) {
            gameInput.startPressed = true;
        }
        return gameInput;
    }

    const float x = input.mouseX();
    const float y = input.mouseY();

    if (gameId == "mental_math") {
        if (GameLayout::mentalMathAnswerField(panel).contains(x, y)) {
            gameInput.focusTextField = true;
        } else {
            gameInput.blurTextField = true;
            gameInput.submitPressed =
                GameLayout::mentalMathSubmitButton(panel).contains(x, y);
        }
    } else if (gameId == "number_memory") {
        if (GameLayout::numberMemoryRecallField(panel).contains(x, y)) {
            gameInput.focusTextField = true;
        } else {
            gameInput.blurTextField = true;
            gameInput.submitPressed =
                GameLayout::numberMemoryConfirmButton(panel).contains(x, y);
        }
    } else if (gameId == "quick_reaction") {
        gameInput.startPressed =
            GameLayout::reactionStartButton(panel).contains(x, y);
        gameInput.primaryActionPressed =
            GameLayout::reactionPanel(panel).contains(x, y);
    } else if (gameId == "logic_sequence") {
        for (int index = 0; index < 4; ++index) {
            if (GameLayout::sequenceOptionButton(panel, index).contains(x, y)) {
                gameInput.optionIndex = index;
                break;
            }
        }
    }
    return gameInput;
}

}  // namespace

void GameScreen::renderResults(AppContext& context, Renderer& renderer,
                               const Input& input, const Rect& area) {
    context.applyResultOnce();
    const GameResult result = context.activeGame->result();
    const float cx = area.centerX();
    float y = area.y + 30;

    renderer.drawTextCentered("Sessão concluída!", cx, y, 24, Theme::kSuccess,
                              true);
    y += 44;
    if (!context.lastSaveSucceeded) {
        renderer.drawTextCentered(context.lastSaveError, cx, y, 14,
                                  Theme::kDanger, true);
        y += 24;
    }

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
                        {cx - buttonWidth - spacing * 0.5f, y, buttonWidth, 46},
                        "Jogar novamente", Theme::kButton, 16)) {
        const GameInfo* info = context.registry.findById(context.activeGameId);
        if (info) context.startGame(*info);
        return;
    }
    if (Widgets::button(renderer, input,
                        {cx + spacing * 0.5f, y, buttonWidth, 46},
                        "Voltar ao início", Theme::kButton, 16)) {
        context.closeGame(ScreenId::Home);
    }
}

void GameScreen::render(AppContext& context, Renderer& renderer,
                        const Input& input, const Rect& area,
                        float deltaSeconds) {
    const GameInfo* info = context.registry.findById(context.activeGameId);
    if (!context.activeGame || info == nullptr) {
        renderer.drawText("Nenhum jogo ativo.", area.x, area.y, 20,
                          Theme::kText, true);
        if (Widgets::button(renderer, input, {area.x, area.y + 40, 190, 44},
                            "Voltar ao início")) {
            context.screen = ScreenId::Home;
        }
        return;
    }

    // Escape consistently abandons the current game. Text fields lose focus by
    // clicking outside; the game model no longer contains unreachable cancel code.
    if (input.escapePressed()) {
        context.closeGame(ScreenId::Home);
        return;
    }

    renderer.drawText(info->title, area.x, area.y, 26, Theme::kText, true);
    renderer.drawText(categoryName(info->category), area.x, area.y + 38, 14,
                      Theme::categoryColor(info->category));
    if (Widgets::button(renderer, input,
                        {area.right() - 130, area.y, 130, 38},
                        "Sair do jogo", Theme::kButton, 15)) {
        context.closeGame(ScreenId::Home);
        return;
    }

    const Rect panel{area.x, area.y + 66, area.w, area.h - 66};
    renderer.fillRect(panel, Theme::kPanel);

    if (!context.activeGame->isFinished()) {
        context.activeGame->update(
            deltaSeconds, makeGameInput(input, context.activeGameId, panel));
    }

    if (context.activeGame->isFinished()) renderResults(context, renderer, input, panel);
    else context.activeGame->render(renderer, panel);
}
