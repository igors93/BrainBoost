#pragma once

#include "ui/Rect.h"
#include "ui/Widgets.h"

struct AppContext;
class Renderer;
class Input;

// Profile name editing and progress reset (with confirmation).
class SettingsScreen {
public:
    void render(AppContext& context, Renderer& renderer, const Input& input,
                const Rect& area);

private:
    Widgets::TextFieldState nameField_;
    bool nameLoaded_ = false;
    bool confirmingReset_ = false;
};
