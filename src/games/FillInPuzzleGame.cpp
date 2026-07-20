#include "games/FillInPuzzleGame.h"

#include <algorithm>
#include <numeric>

FillInPuzzleGame::FillInPuzzleGame() : rng_(std::random_device{}()) { generate(0); }
FillInPuzzleGame::FillInPuzzleGame(std::uint32_t seed) : rng_(seed) { generate(0); }

FillInPuzzleGame::FillInPuzzleGame(std::uint32_t seed, int startingDifficulty) : rng_(seed) {
    generate(startingDifficulty);
}

bool FillInPuzzleGame::isBlocked(int row, int col) const {
    return blocked_[cellIndex(row, col)];
}

char FillInPuzzleGame::displayedChar(int row, int col) const {
    return playerGrid_[cellIndex(row, col)];
}

bool FillInPuzzleGame::isLocked(int row, int col) const { return locked_[cellIndex(row, col)]; }

std::pair<int, int> FillInPuzzleGame::cellAt(const Slot& slot, int offset) const {
    if (slot.orientation == Orientation::Across) return {slot.row, slot.col + offset};
    return {slot.row + offset, slot.col};
}

int FillInPuzzleGame::horizontalRunLength(int row, int col) const {
    int start = col;
    while (start > 0 && !isBlocked(row, start - 1)) --start;
    int end = col;
    while (end < cols_ - 1 && !isBlocked(row, end + 1)) ++end;
    return end - start + 1;
}

int FillInPuzzleGame::verticalRunLength(int row, int col) const {
    int start = row;
    while (start > 0 && !isBlocked(start - 1, col)) --start;
    int end = row;
    while (end < rows_ - 1 && !isBlocked(end + 1, col)) ++end;
    return end - start + 1;
}

bool FillInPuzzleGame::generateAttempt() {
    blocked_.assign(cellIndex(rows_ - 1, cols_ - 1) + 1, false);

    // 180-degree rotational symmetry, like a standard crossword grid.
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            const std::size_t idx = cellIndex(r, c);
            const std::size_t mirror = cellIndex(rows_ - 1 - r, cols_ - 1 - c);
            if (blocked_[idx] || blocked_[mirror]) continue;
            if (chance(rng_) < kBlockDensity) {
                blocked_[idx] = true;
                blocked_[mirror] = true;
            }
        }
    }

    // Self-heal: block any open cell that would not belong to a real slot in
    // either direction, repeating until nothing changes.
    bool changed = true;
    while (changed) {
        changed = false;
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                if (isBlocked(r, c)) continue;
                if (horizontalRunLength(r, c) >= kMinSlotLength) continue;
                if (verticalRunLength(r, c) >= kMinSlotLength) continue;
                blocked_[cellIndex(r, c)] = true;
                changed = true;
            }
        }
    }

    slots_.clear();
    for (int r = 0; r < rows_; ++r) {
        int c = 0;
        while (c < cols_) {
            if (isBlocked(r, c)) {
                ++c;
                continue;
            }
            const int start = c;
            while (c < cols_ && !isBlocked(r, c)) ++c;
            const int length = c - start;
            if (length >= kMinSlotLength) {
                slots_.push_back({r, start, length, Orientation::Across, "", false});
            }
        }
    }
    for (int c = 0; c < cols_; ++c) {
        int r = 0;
        while (r < rows_) {
            if (isBlocked(r, c)) {
                ++r;
                continue;
            }
            const int start = r;
            while (r < rows_ && !isBlocked(r, c)) ++r;
            const int length = r - start;
            if (length >= kMinSlotLength) {
                slots_.push_back({start, c, length, Orientation::Down, "", false});
            }
        }
    }

    return static_cast<int>(slots_.size()) >= kMinSlotCount;
}

void FillInPuzzleGame::generate(int startingDifficulty) {
    startingDifficulty_ = std::clamp(startingDifficulty, 0, kMaxDifficulty);
    rows_ = kBaseSize + startingDifficulty_;
    cols_ = kBaseSize + startingDifficulty_;

    for (int attempt = 0; attempt < kMaxGenerationAttempts; ++attempt) {
        if (generateAttempt() || attempt == kMaxGenerationAttempts - 1) break;
    }

    // Fill every open cell with a random digit, then read each slot's target
    // directly off that grid: crossings are consistent by construction, so
    // no dedicated solver is needed to guarantee the puzzle is solvable.
    std::vector<char> solutionGrid(cellIndex(rows_ - 1, cols_ - 1) + 1, '\0');
    std::uniform_int_distribution<int> digit(0, 9);
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (isBlocked(r, c)) continue;
            solutionGrid[cellIndex(r, c)] = static_cast<char>('0' + digit(rng_));
        }
    }
    for (Slot& slot : slots_) {
        std::string value;
        value.reserve(static_cast<std::size_t>(slot.length));
        for (int i = 0; i < slot.length; ++i) {
            const auto [r, c] = cellAt(slot, i);
            value += solutionGrid[cellIndex(r, c)];
        }
        slot.solution = value;
    }

    bank_.clear();
    for (const Slot& slot : slots_) bank_.push_back({slot.solution, false});
    std::stable_sort(bank_.begin(), bank_.end(), [](const BankEntry& a, const BankEntry& b) {
        if (a.number.size() != b.number.size()) return a.number.size() < b.number.size();
        return a.number < b.number;
    });

    playerGrid_.assign(cellIndex(rows_ - 1, cols_ - 1) + 1, '\0');
    locked_.assign(cellIndex(rows_ - 1, cols_ - 1) + 1, false);
    selectedSlot_ = -1;
    cursorOffset_ = 0;
    lastClickedRow_ = -1;
    lastClickedCol_ = -1;
    mistakes_ = 0;
    solved_ = false;
    mistakeFlashTimer_ = 0.0f;

    // Fewer starting clues at higher difficulty: 3 slots given at the
    // baseline, down to 1 at the top of the range.
    const int givenSlotCount = std::max(1, 3 - startingDifficulty_ / 2);
    std::vector<int> order(slots_.size());
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), rng_);
    for (int i = 0; i < givenSlotCount && i < static_cast<int>(order.size()); ++i) {
        revealSlot(order[static_cast<std::size_t>(i)]);
    }
    settleIncidentallyCompletedSlots();
}

void FillInPuzzleGame::revealSlot(int slotIndex) {
    Slot& slot = slots_[static_cast<std::size_t>(slotIndex)];
    if (slot.solved) return;
    for (int i = 0; i < slot.length; ++i) {
        const auto [r, c] = cellAt(slot, i);
        const std::size_t idx = cellIndex(r, c);
        playerGrid_[idx] = slot.solution[static_cast<std::size_t>(i)];
        locked_[idx] = true;
    }
    slot.solved = true;
    markBankEntryUsed(slot.solution);
}

void FillInPuzzleGame::markBankEntryUsed(const std::string& value) {
    for (BankEntry& entry : bank_) {
        if (!entry.used && entry.number == value) {
            entry.used = true;
            return;
        }
    }
}

bool FillInPuzzleGame::slotCovers(const Slot& slot, int row, int col) const {
    if (slot.orientation == Orientation::Across) {
        return slot.row == row && col >= slot.col && col < slot.col + slot.length;
    }
    return slot.col == col && row >= slot.row && row < slot.row + slot.length;
}

int FillInPuzzleGame::slotAt(int row, int col, Orientation orientation) const {
    for (int i = 0; i < static_cast<int>(slots_.size()); ++i) {
        const Slot& slot = slots_[static_cast<std::size_t>(i)];
        if (slot.orientation == orientation && slotCovers(slot, row, col)) return i;
    }
    return -1;
}

void FillInPuzzleGame::selectCellAt(int row, int col) {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) return;
    if (isBlocked(row, col)) return;

    const int across = slotAt(row, col, Orientation::Across);
    const int down = slotAt(row, col, Orientation::Down);
    const bool sameCellAsLastClick = row == lastClickedRow_ && col == lastClickedCol_;
    lastClickedRow_ = row;
    lastClickedCol_ = col;

    Orientation orientation = across >= 0 ? Orientation::Across : Orientation::Down;
    if (selectedSlot_ >= 0) {
        const Slot& current = slots_[static_cast<std::size_t>(selectedSlot_)];
        if (sameCellAsLastClick) {
            // Re-clicking the exact same cell toggles orientation when the
            // crossing slot exists here, otherwise keeps it as is.
            if (current.orientation == Orientation::Across && down >= 0) {
                orientation = Orientation::Down;
            } else if (current.orientation == Orientation::Down && across >= 0) {
                orientation = Orientation::Across;
            } else {
                orientation = current.orientation;
            }
        } else if (slotCovers(current, row, col)) {
            // A different cell but still within the slot already being
            // typed: just move the cursor there, don't toggle orientation
            // out from under the player mid-entry.
            orientation = current.orientation;
        }
    }

    const int chosen = orientation == Orientation::Across ? across : down;
    if (chosen < 0) return;  // every open cell belongs to at least one slot
    selectedSlot_ = chosen;
    const Slot& slot = slots_[static_cast<std::size_t>(chosen)];
    cursorOffset_ = slot.orientation == Orientation::Across ? col - slot.col : row - slot.row;
}

void FillInPuzzleGame::typeDigit(char digit) {
    if (selectedSlot_ < 0 || solved_) return;
    Slot& slot = slots_[static_cast<std::size_t>(selectedSlot_)];
    if (slot.solved) return;

    while (cursorOffset_ < slot.length) {
        const auto [r, c] = cellAt(slot, cursorOffset_);
        if (!locked_[cellIndex(r, c)]) break;
        ++cursorOffset_;
    }
    if (cursorOffset_ >= slot.length) return;

    const auto [r, c] = cellAt(slot, cursorOffset_);
    playerGrid_[cellIndex(r, c)] = digit;
    ++cursorOffset_;

    tryCommitSelectedSlot();
}

void FillInPuzzleGame::backspace() {
    if (selectedSlot_ < 0) return;
    const Slot& slot = slots_[static_cast<std::size_t>(selectedSlot_)];
    if (slot.solved) return;

    int pos = cursorOffset_ - 1;
    while (pos >= 0) {
        const auto [r, c] = cellAt(slot, pos);
        if (!locked_[cellIndex(r, c)]) break;
        --pos;
    }
    if (pos < 0) return;
    const auto [r, c] = cellAt(slot, pos);
    playerGrid_[cellIndex(r, c)] = '\0';
    cursorOffset_ = pos;
}

void FillInPuzzleGame::tryCommitSelectedSlot() {
    Slot& slot = slots_[static_cast<std::size_t>(selectedSlot_)];
    for (int i = 0; i < slot.length; ++i) {
        const auto [r, c] = cellAt(slot, i);
        if (playerGrid_[cellIndex(r, c)] == '\0') return;  // not fully typed yet
    }

    std::string typed;
    typed.reserve(static_cast<std::size_t>(slot.length));
    for (int i = 0; i < slot.length; ++i) {
        const auto [r, c] = cellAt(slot, i);
        typed += playerGrid_[cellIndex(r, c)];
    }

    // Validated against this slot's own generated target, not "any unused
    // bank entry": with crossings already fixed by earlier solves, a
    // different-but-coincidentally-unused number could otherwise be
    // accepted here and silently strand the number this slot actually
    // needs, making the rest of the grid unsolvable.
    if (typed == slot.solution) {
        for (int i = 0; i < slot.length; ++i) {
            const auto [r, c] = cellAt(slot, i);
            locked_[cellIndex(r, c)] = true;
        }
        slot.solved = true;
        markBankEntryUsed(typed);
        cursorOffset_ = 0;
        // Locking this slot's cells can complete a crossing slot too — every
        // one of its own cells might now be filled purely as a side effect,
        // without ever being individually typed and validated. Without this,
        // that slot's bank number never gets struck and the puzzle never
        // registers as finished even once the grid looks full.
        settleIncidentallyCompletedSlots();
        solved_ = std::all_of(slots_.begin(), slots_.end(),
                              [](const Slot& s) { return s.solved; });
    } else {
        ++mistakes_;
        mistakeFlashTimer_ = 0.6f;
        for (int i = 0; i < slot.length; ++i) {
            const auto [r, c] = cellAt(slot, i);
            if (!locked_[cellIndex(r, c)]) playerGrid_[cellIndex(r, c)] = '\0';
        }
        cursorOffset_ = 0;
    }
}

void FillInPuzzleGame::settleIncidentallyCompletedSlots() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (Slot& slot : slots_) {
            if (slot.solved) continue;

            bool fullyFilled = true;
            for (int i = 0; i < slot.length && fullyFilled; ++i) {
                const auto [r, c] = cellAt(slot, i);
                if (playerGrid_[cellIndex(r, c)] == '\0') fullyFilled = false;
            }
            if (!fullyFilled) continue;

            std::string typed;
            typed.reserve(static_cast<std::size_t>(slot.length));
            for (int i = 0; i < slot.length; ++i) {
                const auto [r, c] = cellAt(slot, i);
                typed += playerGrid_[cellIndex(r, c)];
            }
            if (typed != slot.solution) continue;  // leftover guess, not a mistake to flag

            for (int i = 0; i < slot.length; ++i) {
                const auto [r, c] = cellAt(slot, i);
                locked_[cellIndex(r, c)] = true;
            }
            slot.solved = true;
            markBankEntryUsed(typed);
            changed = true;
        }
    }
}

void FillInPuzzleGame::update(float deltaSeconds, const GameInput& input) {
    if (solved_) return;
    if (mistakeFlashTimer_ > 0.0f) {
        mistakeFlashTimer_ = std::max(0.0f, mistakeFlashTimer_ - std::max(0.0f, deltaSeconds));
    }

    if (input.optionIndex >= 0 && cols_ > 0) {
        selectCellAt(input.optionIndex / cols_, input.optionIndex % cols_);
    }
    for (const char character : input.text) {
        if (character >= '0' && character <= '9') typeDigit(character);
    }
    if (input.backspacePressed) backspace();
}

bool FillInPuzzleGame::isFinished() const { return solved_; }

GameResult FillInPuzzleGame::result() const {
    GameResult result;
    result.total = slotCount();
    result.correct = static_cast<int>(
        std::count_if(slots_.begin(), slots_.end(), [](const Slot& s) { return s.solved; }));
    result.difficulty = startingDifficulty_;

    if (solved_) {
        result.score = std::clamp(100 - mistakes_ * 8, 30, 100);
        result.xpEarned = std::max(10, 40 + startingDifficulty_ * 6 - mistakes_ * 3);
    } else if (result.total > 0) {
        result.score = result.correct * 100 / result.total;
    }
    return result;
}
