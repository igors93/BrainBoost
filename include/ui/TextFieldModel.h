#pragma once

#include <cctype>
#include <cstddef>
#include <string>

#include "ui/Rect.h"

struct TextFieldState {
    std::string text;
    bool focused = false;
};

struct TextFieldInput {
    float pointerX = 0.0f;
    float pointerY = 0.0f;
    bool primaryPressed = false;
    bool confirmPressed = false;
    bool cancelPressed = false;
    bool backspacePressed = false;
    std::string text;
};

inline void eraseLastUtf8Codepoint(std::string& text) {
    if (text.empty()) return;
    std::size_t erase = text.size() - 1;
    while (erase > 0 &&
           (static_cast<unsigned char>(text[erase]) & 0xC0U) == 0x80U) {
        --erase;
    }
    text.erase(erase);
}

// Pure text-field state transition. It has no Renderer or SDL dependency and
// can therefore be unit-tested independently from the graphical widget.
inline bool updateTextFieldState(TextFieldState& state,
                                 const TextFieldInput& input,
                                 const Rect& rect,
                                 bool numericOnly,
                                 std::size_t maxLength) {
    if (input.primaryPressed) {
        state.focused = rect.contains(input.pointerX, input.pointerY);
    }
    if (input.cancelPressed) state.focused = false;

    if (state.focused) {
        for (char character : input.text) {
            if (state.text.size() >= maxLength) break;
            if (numericOnly &&
                std::isdigit(static_cast<unsigned char>(character)) == 0) {
                continue;
            }
            state.text += character;
        }
        if (input.backspacePressed) eraseLastUtf8Codepoint(state.text);
    }

    return state.focused && input.confirmPressed;
}
