#include "ui/HomeScreen.h"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <string>

#include "app/AppContext.h"
#include "ui/Input.h"
#include "ui/Renderer.h"
#include "ui/Widgets.h"

namespace {

// 4250 -> "4.250" (pt-BR thousands separator).
std::string formatThousands(int value) {
    std::string digits = std::to_string(value);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) {
        digits.insert(static_cast<size_t>(i), ".");
    }
    return digits;
}

const char* tipOfTheDay() {
    static const char* kTips[] = {
        "Treine um pouco todos os dias. A consistência é a chave para o progresso!",
        "Alterne entre categorias diferentes para exercitar todo o cérebro.",
        "Dormir bem melhora a memória e o tempo de reação.",
        "Desafie-se: dificuldade na medida certa acelera o aprendizado.",
        "Faça pausas curtas entre as sessões para manter o foco.",
    };
    const long day = static_cast<long>(std::time(nullptr)) / 86400;
    return kTips[day % (sizeof(kTips) / sizeof(kTips[0]))];
}

}  // namespace

float HomeScreen::renderHeader(AppContext& context, Renderer& renderer,
                               const Rect& area) {
    char welcome[96];
    std::snprintf(welcome, sizeof(welcome), "Bem-vindo de volta, %s!",
                  context.profile.name.c_str());
    renderer.drawText(welcome, area.x, area.y, 26, Theme::kText, true);
    renderer.drawText("Exercite sua mente todos os dias e alcance seu melhor potencial.",
                      area.x, area.y + 38, 14, Theme::kTextMuted);

    // Stat chips, right-aligned.
    const float chipWidth = 150.0f;
    const float spacing = 10.0f;
    float x = area.right() - chipWidth * 3 - spacing * 2;

    char streak[32];
    std::snprintf(streak, sizeof(streak), "%d dias", context.profile.streakDays());
    char level[32];
    std::snprintf(level, sizeof(level), "Nível %d", context.profile.level());

    Widgets::statChip(renderer, Rect{x, area.y, chipWidth, 56}, "Sequência", streak,
                      Theme::kWarning);
    x += chipWidth + spacing;
    Widgets::statChip(renderer, Rect{x, area.y, chipWidth, 56}, "XP Total",
                      formatThousands(context.profile.xp()), Theme::kAccent);
    x += chipWidth + spacing;
    Widgets::statChip(renderer, Rect{x, area.y, chipWidth, 56}, context.profile.name,
                      level, Theme::kSuccess);

    return 66.0f;  // header height
}

float HomeScreen::renderGameGrid(AppContext& context, Renderer& renderer,
                                 const Input& input, const Rect& area) {
    auto games = context.registry.games();
    std::stable_sort(games.begin(), games.end(),
                     [&](const GameInfo& a, const GameInfo& b) {
                         return context.stats.forCategory(a.category).gamesPlayed >
                                context.stats.forCategory(b.category).gamesPlayed;
                     });

    const float spacing = 12.0f;
    const int columns = std::clamp(static_cast<int>(area.w / 290.0f), 1, 4);
    const float cardWidth =
        (area.w - spacing * static_cast<float>(columns - 1)) /
        static_cast<float>(columns);
    const float cardHeight = 140.0f;

    for (size_t i = 0; i < games.size(); ++i) {
        const int column = static_cast<int>(i) % columns;
        const int row = static_cast<int>(i) / columns;
        const Rect card{area.x + static_cast<float>(column) * (cardWidth + spacing),
                        area.y + static_cast<float>(row) * (cardHeight + spacing),
                        cardWidth, cardHeight};
        if (Widgets::gameCard(renderer, input, card, games[i])) {
            context.startGame(games[i]);
        }
    }

    const int rows =
        (static_cast<int>(games.size()) + columns - 1) / columns;
    return static_cast<float>(rows) * (cardHeight + spacing);
}

void HomeScreen::renderBottomPanels(AppContext& context, Renderer& renderer,
                                    const Input& input, const Rect& area) {
    const float spacing = 12.0f;
    const float performanceWidth = area.w * 0.32f;
    const float chartWidth = area.w * 0.37f;
    const float achievementsWidth = area.w - performanceWidth - chartWidth - spacing * 2;

    // --- Skill per category -------------------------------------------------
    const Rect performance{area.x, area.y, performanceWidth, area.h};
    renderer.fillRect(performance, Theme::kPanel);
    renderer.drawText("Seu Desempenho", performance.x + 16, performance.y + 12, 16,
                      Theme::kText, true);
    float rowY = performance.y + 44;
    const float rowHeight =
        std::max(22.0f, (performance.h - 56.0f) / static_cast<float>(kCategoryCount));
    for (int i = 0; i < kCategoryCount; ++i) {
        const auto category = static_cast<GameCategory>(i);
        Widgets::skillBar(renderer,
                          Rect{performance.x + 16, rowY, performance.w - 32, 20},
                          categoryName(category),
                          context.stats.forCategory(category).skill / 100.0f);
        rowY += rowHeight;
    }

    // --- Score evolution ----------------------------------------------------
    const Rect evolution{performance.right() + spacing, area.y, chartWidth, area.h};
    renderer.fillRect(evolution, Theme::kPanel);
    renderer.drawText("Gráfico de Evolução", evolution.x + 16, evolution.y + 12, 16,
                      Theme::kText, true);
    Widgets::scoreLineChart(renderer, input,
                            Rect{evolution.x + 12, evolution.y + 42,
                                 evolution.w - 24, evolution.h - 56},
                            context.stats.history());

    // --- Recent achievements ------------------------------------------------
    const Rect recent{evolution.right() + spacing, area.y, achievementsWidth, area.h};
    renderer.fillRect(recent, Theme::kPanel);
    renderer.drawText("Conquistas Recentes", recent.x + 16, recent.y + 12, 16,
                      Theme::kText, true);

    const auto& unlockedIds = context.profile.achievements();
    float y = recent.y + 44;
    int shown = 0;
    for (auto it = unlockedIds.rbegin(); it != unlockedIds.rend() && shown < 3; ++it) {
        for (const AchievementDef& def : Achievements::all()) {
            if (def.id != *it) continue;
            renderer.drawText(def.title, recent.x + 16, y, 15, Theme::kSuccess, true);
            char detail[128];
            std::snprintf(detail, sizeof(detail), "%s  (+%d XP)",
                          def.description.c_str(), def.xpReward);
            renderer.drawText(detail, recent.x + 16, y + 20, 13, Theme::kTextMuted);
            y += 52;
            ++shown;
            break;
        }
    }
    if (shown == 0) {
        y += renderer.drawTextWrapped("Complete jogos para desbloquear conquistas!",
                                      recent.x + 16, y, recent.w - 32, 14,
                                      Theme::kTextMuted);
        if (!Achievements::all().empty()) {
            const AchievementDef& next = Achievements::all().front();
            y += 8;
            renderer.drawText("Próxima meta: " + next.title, recent.x + 16, y, 14,
                              Theme::kText);
            char detail[128];
            std::snprintf(detail, sizeof(detail), "%s (+%d XP)",
                          next.description.c_str(), next.xpReward);
            renderer.drawText(detail, recent.x + 16, y + 20, 13, Theme::kTextMuted);
        }
    }
}

void HomeScreen::render(AppContext& context, Renderer& renderer, const Input& input,
                        const Rect& area) {
    float y = area.y;
    y += renderHeader(context, renderer, area) + 8;

    renderer.drawLine(area.x, y, area.right(), y, Theme::kGrid);
    y += 14;

    renderer.drawText("Escolha um jogo", area.x, y, 20, Theme::kText, true);
    y += 36;

    y += renderGameGrid(context, renderer, input, Rect{area.x, y, area.w, 0});
    y += 4;

    // Bottom panels fill whatever room is left above the tip line.
    const float tipHeight = 26.0f;
    const float panelHeight = std::max(150.0f, area.bottom() - y - tipHeight - 8.0f);
    renderBottomPanels(context, renderer, input, Rect{area.x, y, area.w, panelHeight});

    char tip[160];
    std::snprintf(tip, sizeof(tip), "Dica do dia: %s", tipOfTheDay());
    renderer.drawText(tip, area.x, area.bottom() - tipHeight + 4, 13, Theme::kTextMuted);
}
