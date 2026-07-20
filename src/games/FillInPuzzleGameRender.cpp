#include "games/FillInPuzzleGame.h"

#include <cstdio>
#include <string>

#include "games/GameLayout.h"
#include "ui/Renderer.h"

namespace {
// Clearly distinct from both the blocked-cell and default-open-cell colors
// (see the render loop below), so the active slot always reads at a glance.
constexpr Color kSlotHighlight = rgb(0x24406B);
}  // namespace

void FillInPuzzleGame::render(Renderer& renderer, const Rect& area) const {
    renderer.drawText("Clique numa célula e digite os números do banco ao lado.",
                      area.x, area.y, 13, Theme::kTextMuted);

    // The exact cell the next keystroke will land on, so selection is always
    // visible even right after toggling orientation.
    int cursorRow = -1;
    int cursorCol = -1;
    if (selectedSlot_ >= 0) {
        const Slot& selected = slots_[static_cast<std::size_t>(selectedSlot_)];
        if (!selected.solved && cursorOffset_ < selected.length) {
            const auto [row, col] = cellAt(selected, cursorOffset_);
            cursorRow = row;
            cursorCol = col;
        }
    }

    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            const Rect cell = GameLayout::fillInGridCell(area, r, c, rows_, cols_);
            if (isBlocked(r, c)) {
                // Deliberately near-black: must never be mistaken for a
                // writable cell.
                renderer.fillRect(cell, Theme::kBackground);
                continue;
            }

            bool inSelectedSlot = false;
            if (selectedSlot_ >= 0) {
                inSelectedSlot = slotCovers(slots_[static_cast<std::size_t>(selectedSlot_)], r, c);
            }

            Color background = Theme::kButton;
            if (mistakeFlashTimer_ > 0.0f && inSelectedSlot) {
                background = Theme::kDangerButton;
            } else if (inSelectedSlot) {
                background = kSlotHighlight;
            } else if (isLocked(r, c)) {
                background = Theme::kPanel;
            }

            renderer.fillRect(cell, background);
            renderer.outlineRect(cell, Theme::kGrid, 1);
            if (r == cursorRow && c == cursorCol) {
                renderer.outlineRect(cell, Theme::kWarning, 3);
            }

            const char ch = displayedChar(r, c);
            if (ch != '\0') {
                const std::string text(1, ch);
                const int fontSize = cell.h > 30.0f ? 18 : 14;
                const float textY = cell.y + (cell.h - renderer.lineHeight(fontSize)) * 0.5f;
                const Color textColor = isLocked(r, c) ? Theme::kSuccess : Theme::kText;
                renderer.drawTextCentered(text, cell.centerX(), textY, fontSize, textColor, true);
            }
        }
    }

    const Rect gridArea = GameLayout::fillInGridArea(area);
    const Rect bankArea{gridArea.right() + 20.0f, area.y + 24.0f,
                        area.w - gridArea.w - 20.0f, area.h - 24.0f};

    char status[64];
    std::snprintf(status, sizeof(status), "Erros: %d", mistakes_);
    renderer.drawText(status, bankArea.x, area.y, 14, Theme::kWarning, true);

    std::vector<int> lengths;
    for (const BankEntry& entry : bank_) {
        const int length = static_cast<int>(entry.number.size());
        if (lengths.empty() || lengths.back() != length) lengths.push_back(length);
    }
    if (lengths.empty()) return;

    const float columnWidth = std::max(70.0f, bankArea.w / static_cast<float>(lengths.size()));
    std::size_t entryIndex = 0;
    for (std::size_t column = 0; column < lengths.size(); ++column) {
        const float columnX = bankArea.x + static_cast<float>(column) * columnWidth;
        float y = bankArea.y + 22.0f;
        renderer.drawText(std::to_string(lengths[column]), columnX, bankArea.y, 15, Theme::kText,
                          true);
        while (entryIndex < bank_.size() &&
               static_cast<int>(bank_[entryIndex].number.size()) == lengths[column]) {
            const BankEntry& entry = bank_[entryIndex];
            const Color color = entry.used ? Theme::kTextMuted : Theme::kText;
            renderer.drawText(entry.number, columnX, y, 13, color);
            if (entry.used) {
                const float textWidth = renderer.textWidth(entry.number, 13);
                renderer.drawLine(columnX, y + 8.0f, columnX + textWidth, y + 8.0f,
                                  Theme::kTextMuted);
            }
            y += 18.0f;
            ++entryIndex;
        }
    }
}
