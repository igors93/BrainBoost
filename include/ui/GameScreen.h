#pragma once

#include "ui/Rect.h"

struct AppContext;
class Renderer;
class Input;

// Hosts the active game session and, once it finishes, the results panel
// (score, XP, unlocked achievements, replay / back buttons).
class GameScreen {
public:
    void render(AppContext& context, Renderer& renderer, const Input& input,
                const Rect& area, float deltaSeconds);

private:
    void renderResults(AppContext& context, Renderer& renderer, const Input& input,
                       const Rect& area);
};
