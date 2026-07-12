#include "ui/SettingsScreen.h"

#include "app/AppContext.h"
#include "ui/Input.h"
#include "ui/Renderer.h"

void SettingsScreen::render(AppContext& context, Renderer& renderer,
                            const Input& input, const Rect& area) {
    float y = area.y;
    renderer.drawText("Configurações", area.x, y, 26, Theme::kText, true);
    y += 50;

    if (!nameLoaded_) {
        nameField_.text = context.profile.name;
        nameLoaded_ = true;
    }

    // --- Profile -------------------------------------------------------------
    const Rect profilePanel{area.x, y, area.w, 140};
    renderer.fillRect(profilePanel, Theme::kPanel);
    renderer.drawText("Perfil", profilePanel.x + 16, profilePanel.y + 12, 16,
                      Theme::kText, true);
    renderer.drawText("Seu nome:", profilePanel.x + 16, profilePanel.y + 44, 14,
                      Theme::kTextMuted);

    const Rect field{profilePanel.x + 16, profilePanel.y + 70, 320, 44};
    const bool submitted =
        Widgets::textField(renderer, input, nameField_, field, 18, false, 24);
    const bool saveClicked = Widgets::button(
        renderer, input, Rect{field.right() + 10, field.y, 110, 44}, "Salvar");
    if ((submitted || saveClicked) && !nameField_.text.empty()) {
        context.profile.name = nameField_.text;
        context.saveProgress();
    }
    y += 152;

    // --- Data ------------------------------------------------------------------
    const Rect dataPanel{area.x, y, area.w, 150};
    renderer.fillRect(dataPanel, Theme::kPanel);
    renderer.drawText("Dados", dataPanel.x + 16, dataPanel.y + 12, 16, Theme::kText,
                      true);
    renderer.drawText("Progresso salvo em: " + context.saveManager.filePath(),
                      dataPanel.x + 16, dataPanel.y + 42, 13, Theme::kTextMuted);

    const Rect resetButton{dataPanel.x + 16, dataPanel.y + 76, 220, 44};
    if (!confirmingReset_) {
        if (Widgets::button(renderer, input, resetButton, "Zerar progresso",
                            Theme::kDangerButton, 15)) {
            confirmingReset_ = true;
        }
    } else {
        if (Widgets::button(renderer, input, resetButton, "Confirmar exclusão",
                            rgb(0x991B1B), 15)) {
            context.profile.reset();
            context.stats.reset();
            context.saveProgress();
            confirmingReset_ = false;
        }
        if (Widgets::button(renderer, input,
                            Rect{resetButton.right() + 10, resetButton.y, 130, 44},
                            "Cancelar")) {
            confirmingReset_ = false;
        }
        renderer.drawText("Isso apaga XP, estatísticas e conquistas. Não pode ser desfeito.",
                          resetButton.x, resetButton.bottom() + 8, 13, Theme::kDanger);
    }
}
