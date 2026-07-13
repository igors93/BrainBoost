#include "ui/AboutScreen.h"

#include "app/AppContext.h"
#include "ui/Input.h"
#include "ui/Renderer.h"

float AboutScreen::render(AppContext& context, Renderer& renderer, const Input& input,
                          const Rect& area) {
    (void)context;
    (void)input;

    float y = area.y;
    renderer.drawText("Sobre", area.x, y, 26, Theme::kText, true);
    y += 50;

    const Rect panel{area.x, y, area.w, 300};
    renderer.fillRect(panel, Theme::kPanel);

    renderer.drawText("BrainBoost", panel.x + 20, panel.y + 18, 28, Theme::kAccent,
                      true);
    renderer.drawText("Treinamento Cognitivo — versão 0.2.0", panel.x + 20,
                      panel.y + 56, 14, Theme::kTextMuted);

    renderer.drawTextWrapped(
        "O BrainBoost é um aplicativo de treinamento cerebral com jogos que "
        "exercitam memória, raciocínio, atenção, linguagem, percepção espacial "
        "e lógica. Jogue todos os dias para manter sua sequência e acompanhar "
        "sua evolução.",
        panel.x + 20, panel.y + 92, panel.w - 40, 15, Theme::kText);

    renderer.drawText("Tecnologias: C++17, SDL2 e FreeType (interface própria).",
                      panel.x + 20, panel.y + 200, 14, Theme::kTextMuted);
    renderer.drawText(
        "Arquitetura modular: core (dados), games (jogos) e ui (interface).",
        panel.x + 20, panel.y + 226, 14, Theme::kTextMuted);

    return (y + 300.0f) - area.y;
}
