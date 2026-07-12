#include "ui/Sidebar.h"

#include <cstdio>

#include "app/AppContext.h"
#include "ui/Input.h"
#include "ui/Renderer.h"
#include "ui/Widgets.h"

void Sidebar::render(AppContext& context, Renderer& renderer, const Input& input) {
    const Rect column{0, 0, kWidth, renderer.height()};
    renderer.fillRect(column, Theme::kSidebar);

    renderer.drawText("BrainBoost", 16, 22, 28, Theme::kAccent, true);
    renderer.drawText("Treinamento Cognitivo", 16, 58, 13, Theme::kTextMuted);

    struct NavEntry {
        const char* label;
        ScreenId target;
    };
    constexpr NavEntry kEntries[] = {
        {"Início", ScreenId::Home},
        {"Jogos", ScreenId::Games},
        {"Estatísticas", ScreenId::Stats},
        {"Conquistas", ScreenId::Achievements},
        {"Configurações", ScreenId::Settings},
        {"Sobre", ScreenId::About},
    };

    float y = 96;
    for (const NavEntry& entry : kEntries) {
        const bool active = context.screen == entry.target;
        const Rect rect{14, y, kWidth - 28, 40};
        if (Widgets::button(renderer, input, rect, entry.label,
                            active ? Theme::kAccent : Theme::kSidebar, 16)) {
            if (context.screen == ScreenId::Playing) context.closeGame(entry.target);
            context.screen = entry.target;
        }
        y += 46;
    }

    if (Widgets::button(renderer, input, Rect{14, y + 6, kWidth - 28, 40}, "Sair",
                        Theme::kDangerButton, 16)) {
        context.quitRequested = true;
    }

    // Streak box pinned to the bottom.
    const Rect box{14, renderer.height() - 110, kWidth - 28, 92};
    renderer.fillRect(box, Theme::kPanel);
    renderer.drawText("Sequência Atual", box.x + 14, box.y + 10, 13, Theme::kTextMuted);
    char streak[32];
    std::snprintf(streak, sizeof(streak), "%d dias", context.profile.streakDays());
    renderer.drawText(streak, box.x + 14, box.y + 30, 22, Theme::kWarning, true);
    renderer.drawText(context.profile.streakDays() > 0 ? "Continue assim!"
                                                       : "Jogue hoje para começar!",
                      box.x + 14, box.y + 62, 13, Theme::kTextMuted);
}
