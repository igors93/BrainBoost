#pragma once

#include <SDL.h>

#include <string>

// Per-frame input snapshot. Application feeds SDL events into handleEvent();
// screens and widgets only read the accessors.
class Input {
public:
    // Clears the "happened this frame" state; call once before polling events.
    void beginFrame();
    void handleEvent(const SDL_Event& event);

    float mouseX() const { return mouseX_; }
    float mouseY() const { return mouseY_; }
    bool mouseDown() const { return mouseDown_; }
    bool mousePressed() const { return mousePressed_; }

    // UTF-8 text typed this frame (already handles keyboard layout/accents).
    const std::string& textTyped() const { return textTyped_; }
    bool backspacePressed() const { return backspacePressed_; }
    bool enterPressed() const { return enterPressed_; }
    bool escapePressed() const { return escapePressed_; }

private:
    float mouseX_ = 0.0f;
    float mouseY_ = 0.0f;
    bool mouseDown_ = false;
    bool mousePressed_ = false;
    std::string textTyped_;
    bool backspacePressed_ = false;
    bool enterPressed_ = false;
    bool escapePressed_ = false;
};
