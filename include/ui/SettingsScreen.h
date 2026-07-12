#pragma once

#include "ui/Rect.h"
#include "ui/Widgets.h"

struct AppContext;
class Renderer;
class Input;

class SettingsScreen {
public:
    void render(AppContext& context, Renderer& renderer, const Input& input,
                const Rect& area);

private:
    Widgets::TextFieldState nameField_;
    bool nameLoaded_ = false;

    enum class ResetScope {
        None,
        All,
        Statistics,
        History,
        Achievements,
        ProfileName
    };
    ResetScope confirmingReset_ = ResetScope::None;
};
