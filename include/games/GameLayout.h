#pragma once

#include <algorithm>

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

inline Rect spatialMemoryGridCell(const Rect& area, int index) {
    constexpr float kSize = 80.0f;
    constexpr float kSpacing = 16.0f;
    const float gridWidth = 3 * kSize + 2 * kSpacing;
    const float startX = area.centerX() - gridWidth * 0.5f;
    const float startY = area.y + 130.0f;
    
    int row = index / 3;
    int col = index % 3;
    
    return {startX + col * (kSize + kSpacing),
            startY + row * (kSize + kSpacing),
            kSize, kSize};
}

// Left portion of the panel reserved for the puzzle grid; the remainder is
// the side number bank. Reserves a header strip above the grid for the
// instructions line so it never sits underneath the first row of cells.
inline Rect fillInGridArea(const Rect& area) {
    constexpr float kHeaderHeight = 26.0f;
    return {area.x, area.y + kHeaderHeight, area.w * 0.55f, area.h - kHeaderHeight};
}

// Same cell geometry is used for hit-testing (GameScreen) and drawing
// (FillInPuzzleGameRender), so clicks always line up with what is drawn.
inline Rect fillInGridCell(const Rect& area, int row, int col, int rows, int cols) {
    const Rect gridArea = fillInGridArea(area);
    const float maxDimension = static_cast<float>(std::max(rows, cols));
    const float cellSize =
        maxDimension > 0.0f ? std::min(gridArea.w, gridArea.h) / maxDimension : 0.0f;
    const float gridWidth = cellSize * static_cast<float>(cols);
    const float gridHeight = cellSize * static_cast<float>(rows);
    const float startX = gridArea.x + (gridArea.w - gridWidth) * 0.5f;
    const float startY = gridArea.y + (gridArea.h - gridHeight) * 0.5f;
    return {startX + static_cast<float>(col) * cellSize, startY + static_cast<float>(row) * cellSize,
            cellSize, cellSize};
}

}  // namespace GameLayout
