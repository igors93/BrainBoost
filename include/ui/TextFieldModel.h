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

inline std::size_t utf8SequenceLength(unsigned char first) {
    if (first < 0x80U) return 1;
    if ((first & 0xE0U) == 0xC0U) return 2;
    if ((first & 0xF0U) == 0xE0U) return 3;
    if ((first & 0xF8U) == 0xF0U) return 4;
    return 0;
}

inline bool isValidUtf8Sequence(const std::string& text, std::size_t offset,
                                std::size_t length) {
    if (length == 0 || offset + length > text.size()) return false;
    for (std::size_t index = 1; index < length; ++index) {
        const unsigned char byte =
            static_cast<unsigned char>(text[offset + index]);
        if ((byte & 0xC0U) != 0x80U) return false;
    }
    return true;
}

inline std::size_t utf8CodepointCount(const std::string& text) {
    std::size_t count = 0;
    for (std::size_t offset = 0; offset < text.size();) {
        const std::size_t length = utf8SequenceLength(
            static_cast<unsigned char>(text[offset]));
        if (!isValidUtf8Sequence(text, offset, length)) {
            ++offset;
            continue;
        }
        offset += length;
        ++count;
    }
    return count;
}

inline void eraseLastUtf8Codepoint(std::string& text) {
    if (text.empty()) return;
    std::size_t erase = text.size() - 1;
    while (erase > 0 &&
           (static_cast<unsigned char>(text[erase]) & 0xC0U) == 0x80U) {
        --erase;
    }
    text.erase(erase);
}

inline void appendUtf8Text(std::string& destination, const std::string& input,
                           bool numericOnly, std::size_t maxCharacters) {
    std::size_t count = utf8CodepointCount(destination);
    for (std::size_t offset = 0; offset < input.size() && count < maxCharacters;) {
        const unsigned char first = static_cast<unsigned char>(input[offset]);
        const std::size_t length = utf8SequenceLength(first);
        if (!isValidUtf8Sequence(input, offset, length)) {
            ++offset;
            continue;
        }

        const bool accepted = !numericOnly ||
            (length == 1 && std::isdigit(first) != 0);
        if (accepted) {
            destination.append(input, offset, length);
            ++count;
        }
        offset += length;
    }
}

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
        appendUtf8Text(state.text, input.text, numericOnly, maxLength);
        if (input.backspacePressed) eraseLastUtf8Codepoint(state.text);
    }
    return state.focused && input.confirmPressed;
}
