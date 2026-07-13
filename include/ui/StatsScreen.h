#pragma once

#include "ui/Rect.h"

struct AppContext;
class Renderer;
class Input;

// Full statistics page: totals, score evolution and per-category details.
class StatsScreen {
public:
    float render(AppContext& context, Renderer& renderer, const Input& input,
                 const Rect& area);
};
