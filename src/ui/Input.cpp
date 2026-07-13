#include "ui/Input.h"

void Input::beginFrame() {
    mousePressed_ = false;
    backspacePressed_ = false;
    enterPressed_ = false;
    escapePressed_ = false;
    textTyped_.clear();
    scrollDeltaY_ = 0.0f;
}

void Input::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEWHEEL:
            scrollDeltaY_ += static_cast<float>(event.wheel.y);
            break;
        case SDL_MOUSEMOTION:
            mouseX_ = static_cast<float>(event.motion.x);
            mouseY_ = static_cast<float>(event.motion.y);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                mouseDown_ = true;
                mousePressed_ = true;
                mouseX_ = static_cast<float>(event.button.x);
                mouseY_ = static_cast<float>(event.button.y);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) mouseDown_ = false;
            break;
        case SDL_TEXTINPUT:
            textTyped_ += event.text.text;
            break;
        case SDL_KEYDOWN:  // key repeat included, so holding backspace works
            if (event.key.keysym.sym == SDLK_BACKSPACE) backspacePressed_ = true;
            if (event.key.keysym.sym == SDLK_ESCAPE) escapePressed_ = true;
            if (event.key.keysym.sym == SDLK_RETURN ||
                event.key.keysym.sym == SDLK_KP_ENTER) {
                enterPressed_ = true;
            }
            break;
        default:
            break;
    }
}
