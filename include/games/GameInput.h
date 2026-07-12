#pragma once

#include <string>

// SDL-independent input snapshot consumed by game logic. The UI layer fills
// the raw pointer/keyboard fields, while tests and future accessibility code
// may use the semantic fields directly.
struct GameInput {
    float pointerX = 0.0f;
    float pointerY = 0.0f;

    bool primaryPressed = false;
    bool confirmPressed = false;
    bool cancelPressed = false;
    bool backspacePressed = false;

    std::string text;

    // Optional semantic actions. They make model tests deterministic and can
    // later be populated by keyboard/controller adapters.
    bool startPressed = false;
    bool submitPressed = false;
    int optionIndex = -1;
};
