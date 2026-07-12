#include "ui/StatsScreen.h"

#include <algorithm>
#include <cstdio>

#include "app/AppContext.h"
#include "ui/Input.h"
#include "ui/Renderer.h"
#include "ui/Widgets.h"

void StatsScreen::render(AppContext& context, Renderer& renderer, const Input& input,
                         const Rect& area) {
    float y = area.y;
    renderer.drawText("Estatísticas", area.x, y, 26, Theme::kText, true);
    y += 46;

    char totalGames[16], xp[16], level[16], streak[16];
    std::snprintf(totalGames, sizeof(totalGames), "%d", context.stats.totalGamesPlayed());
    std::snprintf(xp, sizeof(xp), "%d", context.profile.xp());
    std::snprintf(level, sizeof(level), "%d", context.profile.level());
    std::snprintf(streak, sizeof(streak), "%d dias", context.profile.streakDays());

    const float chipWidth = 160.0f;
    const float spacing = 10.0f;
    Widgets::statChip(renderer, Rect{area.x, y, chipWidth, 56}, "Jogos completos",
                      totalGames, Theme::kText);
    Widgets::statChip(renderer, Rect{area.x + (chipWidth + spacing), y, chipWidth, 56},
                      "XP Total", xp, Theme::kAccent);
    Widgets::statChip(renderer,
                      Rect{area.x + (chipWidth + spacing) * 2, y, chipWidth, 56},
                      "Nível", level, Theme::kSuccess);
    Widgets::statChip(renderer,
                      Rect{area.x + (chipWidth + spacing) * 3, y, chipWidth, 56},
                      "Sequência", streak, Theme::kWarning);
    y += 70;

    // Score evolution, larger than the dashboard version.
    const float chartHeight =
        std::max(180.0f, (area.bottom() - y) * 0.5f - 10.0f);
    const Rect chartPanel{area.x, y, area.w, chartHeight};
    renderer.fillRect(chartPanel, Theme::kPanel);
    renderer.drawText("Evolução das pontuações", chartPanel.x + 16, chartPanel.y + 12,
                      16, Theme::kText, true);
    Widgets::scoreLineChart(renderer, input,
                            Rect{chartPanel.x + 12, chartPanel.y + 42,
                                 chartPanel.w - 24, chartPanel.h - 56},
                            context.stats.history());
    y += chartHeight + 12;

    // Per-category breakdown.
    const Rect categoriesPanel{area.x, y, area.w, area.bottom() - y};
    renderer.fillRect(categoriesPanel, Theme::kPanel);
    renderer.drawText("Habilidade por categoria", categoriesPanel.x + 16,
                      categoriesPanel.y + 12, 16, Theme::kText, true);

    float rowY = categoriesPanel.y + 44;
    const float rowHeight = std::max(
        24.0f, (categoriesPanel.h - 56.0f) / static_cast<float>(kCategoryCount));
    for (int i = 0; i < kCategoryCount; ++i) {
        const auto category = static_cast<GameCategory>(i);
        const CategoryStats& stats = context.stats.forCategory(category);

        char label[64];
        std::snprintf(label, sizeof(label), "%s (%d sessões)", categoryName(category),
                      stats.gamesPlayed);
        Widgets::skillBar(renderer,
                          Rect{categoriesPanel.x + 16, rowY, categoriesPanel.w - 32, 20},
                          label, stats.skill / 100.0f, 230.0f);
        rowY += rowHeight;
    }
}
