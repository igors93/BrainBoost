#pragma once

#include "ui/Rect.h"

struct AppContext;
class Renderer;
class Input;

// Static information about the application.
class AboutScreen {
public:
    float render(AppContext& context, Renderer& renderer, const Input& input,
                 const Rect& area);
};
