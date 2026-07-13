#pragma once

#include <string>

// SDL-independent semantic input consumed by game rules. Coordinates and
// layout hit-testing stay in the UI layer.
struct GameInput {
    bool confirmPressed = false;
    bool backspacePressed = false;
    std::string text;

    bool focusTextField = false;
    bool blurTextField = false;
    bool startPressed = false;
    bool submitPressed = false;
    bool primaryActionPressed = false;
    int optionIndex = -1;
};
