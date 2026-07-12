#pragma once

#include "ui/Rect.h"

struct AppContext;
class Renderer;
class Input;

// Lists every achievement with its unlock state and XP reward.
class AchievementsScreen {
public:
    void render(AppContext& context, Renderer& renderer, const Input& input,
                const Rect& area);
};
