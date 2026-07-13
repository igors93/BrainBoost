#include "ui/SettingsScreen.h"

#include "app/AppContext.h"
#include "ui/Input.h"
#include "ui/Renderer.h"

float SettingsScreen::render(AppContext& context, Renderer& renderer,
                            const Input& input, const Rect& area) {
    float y = area.y;
    renderer.drawText("Configurações", area.x, y, 26, Theme::kText, true);
    y += 50;

    if (!context.lastSaveSucceeded && !context.lastSaveError.empty()) {
        const float errorHeight = renderer.drawTextWrapped(
            context.lastSaveError, area.x, y, area.w, 14, Theme::kDanger, true);
        y += errorHeight + 12;
    }

    if (!nameLoaded_) {
        nameField_.text = context.profile.name;
        nameLoaded_ = true;
    }

    // --- Profile ---
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

    // --- Data ---
    const float dataHeight = std::max(320.0f, area.bottom() - y);
    const Rect dataPanel{area.x, y, area.w, dataHeight};
    renderer.fillRect(dataPanel, Theme::kPanel);
    renderer.drawText("Zerar Dados", dataPanel.x + 16, dataPanel.y + 12, 16,
                      Theme::kText, true);

    float rowY = dataPanel.y + 42;
    const float buttonWidth = 240.0f;
    const float buttonHeight = 44.0f;
    const float spacing = 10.0f;

    auto drawResetOption = [&](const std::string& label, ResetScope scope,
                               float& currentY) {
        if (confirmingReset_ == ResetScope::None) {
            if (Widgets::button(renderer, input,
                                Rect{dataPanel.x + 16, currentY, buttonWidth,
                                     buttonHeight},
                                label, Theme::kDangerButton, 15)) {
                confirmingReset_ = scope;
            }
        } else if (confirmingReset_ == scope) {
            if (Widgets::button(renderer, input,
                                Rect{dataPanel.x + 16, currentY, buttonWidth,
                                     buttonHeight},
                                "Confirmar", rgb(0x991B1B), 15)) {
                if (scope == ResetScope::All) {
                    context.profile.reset();
                    context.stats.resetAll();
                    nameField_.text = context.profile.name;
                } else if (scope == ResetScope::Statistics) {
                    context.stats.resetStatisticsOnly();
                } else if (scope == ResetScope::History) {
                    context.stats.resetHistoryOnly();
                } else if (scope == ResetScope::Achievements) {
                    context.profile.resetAchievementsOnly();
                } else if (scope == ResetScope::ProfileName) {
                    context.profile.resetNameOnly();
                    nameField_.text = context.profile.name;
                }
                context.saveProgress();
                confirmingReset_ = ResetScope::None;
            }
            if (Widgets::button(renderer, input,
                                Rect{dataPanel.x + 16 + buttonWidth + 10, currentY,
                                     130, buttonHeight},
                                "Cancelar")) {
                confirmingReset_ = ResetScope::None;
            }
            renderer.drawText("Tem certeza? Esta ação não pode ser desfeita.",
                              dataPanel.x + 16, currentY + buttonHeight + 8, 13,
                              Theme::kDanger);
            currentY += 20;
        }
        currentY += buttonHeight + spacing;
    };

    drawResetOption("Zerar tudo", ResetScope::All, rowY);
    drawResetOption("Zerar apenas estatísticas", ResetScope::Statistics, rowY);
    drawResetOption("Zerar apenas histórico", ResetScope::History, rowY);
    drawResetOption("Zerar apenas conquistas", ResetScope::Achievements, rowY);
    drawResetOption("Restaurar nome padrão", ResetScope::ProfileName, rowY);

    return (y + dataHeight) - area.y;
}
