#pragma once

// Axis-aligned rectangle used for layout and hit-testing.
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    float right() const { return x + w; }
    float bottom() const { return y + h; }
    float centerX() const { return x + w * 0.5f; }
    float centerY() const { return y + h * 0.5f; }

    bool contains(float px, float py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    // Rect shrunk by `amount` on every side.
    Rect inset(float amount) const {
        return Rect{x + amount, y + amount, w - amount * 2.0f, h - amount * 2.0f};
    }
};
