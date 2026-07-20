#pragma once

#include "ui/Rect.h"

struct AppContext;
class Renderer;
class Input;

// Dashboard: welcome header with stat chips, the game card grid, and the
// performance / evolution / achievements panels.
class HomeScreen {
public:
    float render(AppContext& context, Renderer& renderer, const Input& input,
                 const Rect& area);

    // Card grid alone; also used by the "Jogos" page. Returns the height used.
    float renderGameGrid(AppContext& context, Renderer& renderer, const Input& input,
                         const Rect& area);

private:
    float renderHeader(AppContext& context, Renderer& renderer, const Rect& area);
    // "Treino de hoje": a short, category-diverse set of recommended games
    // (see core/Recommendations.h). Returns the height used.
    float renderDailyTraining(AppContext& context, Renderer& renderer,
                              const Input& input, const Rect& area);
    void renderBottomPanels(AppContext& context, Renderer& renderer,
                            const Input& input, const Rect& area);
};
