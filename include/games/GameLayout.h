#pragma once

#include "ui/Rect.h"

namespace GameLayout {

inline Rect mentalMathAnswerField(const Rect& area) {
    return {area.centerX() - 110.0f, area.y + 150.0f, 220.0f, 46.0f};
}

inline Rect mentalMathSubmitButton(const Rect& area) {
    const Rect field = mentalMathAnswerField(area);
    return {area.centerX() - 110.0f, field.bottom() + 16.0f, 220.0f, 44.0f};
}

inline Rect numberMemoryRecallField(const Rect& area) {
    return {area.centerX() - 130.0f, area.y + 100.0f, 260.0f, 46.0f};
}

inline Rect numberMemoryConfirmButton(const Rect& area) {
    const Rect field = numberMemoryRecallField(area);
    return {area.centerX() - 110.0f, field.bottom() + 16.0f, 220.0f, 44.0f};
}

inline Rect reactionStartButton(const Rect& area) {
    return {area.centerX() - 110.0f, area.y + 150.0f, 220.0f, 46.0f};
}

inline Rect reactionPanel(const Rect& area) {
    return {area.x + 16.0f, area.y + 48.0f, area.w - 32.0f, area.h - 64.0f};
}

inline Rect sequenceOptionButton(const Rect& area, int index) {
    constexpr int kCount = 4;
    constexpr float kWidth = 110.0f;
    constexpr float kHeight = 52.0f;
    constexpr float kSpacing = 14.0f;
    const float rowWidth = kCount * kWidth + (kCount - 1) * kSpacing;
    const float firstX = area.centerX() - rowWidth * 0.5f;
    return {firstX + index * (kWidth + kSpacing), area.y + 165.0f,
            kWidth, kHeight};
}

}  // namespace GameLayout
