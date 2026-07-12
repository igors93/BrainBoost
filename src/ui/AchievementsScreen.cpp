#include "ui/AchievementsScreen.h"

#include <cstdio>

#include "app/AppContext.h"
#include "core/Achievements.h"
#include "ui/Input.h"
#include "ui/Renderer.h"

void AchievementsScreen::render(AppContext& context, Renderer& renderer,
                                const Input& input, const Rect& area) {
    (void)input;
    float y = area.y;
    renderer.drawText("Conquistas", area.x, y, 26, Theme::kText, true);
    y += 42;

    int unlockedCount = 0;
    for (const AchievementDef& def : Achievements::all()) {
        if (context.profile.hasAchievement(def.id)) ++unlockedCount;
    }
    char summary[48];
    std::snprintf(summary, sizeof(summary), "%d de %d desbloqueadas", unlockedCount,
                  static_cast<int>(Achievements::all().size()));
    renderer.drawText(summary, area.x, y, 14, Theme::kTextMuted);
    y += 32;

    for (const AchievementDef& def : Achievements::all()) {
        const bool unlocked = context.profile.hasAchievement(def.id);
        const Rect row{area.x, y, area.w, 64};

        Color background = Theme::kPanel;
        if (!unlocked) background.a = 150;
        renderer.fillRect(row, background);
        if (unlocked) renderer.outlineRect(row, rgb(0x4ADE80, 120), 1);

        renderer.drawText(def.title, row.x + 18, row.y + 10, 16,
                          unlocked ? Theme::kSuccess : Theme::kTextMuted, true);
        renderer.drawText(def.description, row.x + 18, row.y + 36, 13,
                          Theme::kTextMuted);

        char reward[24];
        std::snprintf(reward, sizeof(reward), "+%d XP", def.xpReward);
        const float rewardWidth = renderer.textWidth(reward, 15);
        renderer.drawText(reward, row.right() - rewardWidth - 18, row.y + 22, 15,
                          unlocked ? Theme::kWarning : Theme::kTextMuted);

        y += 72;
    }
}
