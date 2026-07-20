#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "games/Game.h"

// "Quase Nada": a crossword-shaped grid where every run of open cells (an
// across or down "slot") must be filled with one of the numbers listed in
// the side bank, each used exactly once, so that every crossing digit
// agrees. A handful of slots start already revealed as clues. A fresh grid
// is procedurally generated every time the game is started.
class FillInPuzzleGame : public Game {
public:
    enum class Orientation { Across, Down };

    struct BankEntry {
        std::string number;
        bool used = false;
    };

    FillInPuzzleGame();
    explicit FillInPuzzleGame(std::uint32_t seed);
    // startingDifficulty is the persisted adaptive level (see
    // GameStats::difficultyLevel); it is clamped to [0, kMaxDifficulty] and
    // grows the grid while shrinking how many slots start revealed.
    FillInPuzzleGame(std::uint32_t seed, int startingDifficulty);

    void update(float deltaSeconds, const GameInput& input) override;
    void render(Renderer& renderer, const Rect& area) const override;
    bool isFinished() const override;
    GameResult result() const override;

    int rows() const { return rows_; }
    int cols() const { return cols_; }
    bool isBlocked(int row, int col) const;
    // '\0' when the cell has nothing displayed yet.
    char displayedChar(int row, int col) const;
    bool isLocked(int row, int col) const;  // given, or part of an already-solved slot

    const std::vector<BankEntry>& bank() const { return bank_; }
    int mistakes() const { return mistakes_; }
    bool isSolved() const { return solved_; }

    int slotCount() const { return static_cast<int>(slots_.size()); }
    int slotRow(int index) const { return slots_[static_cast<std::size_t>(index)].row; }
    int slotCol(int index) const { return slots_[static_cast<std::size_t>(index)].col; }
    int slotLength(int index) const { return slots_[static_cast<std::size_t>(index)].length; }
    Orientation slotOrientation(int index) const {
        return slots_[static_cast<std::size_t>(index)].orientation;
    }
    bool slotSolved(int index) const { return slots_[static_cast<std::size_t>(index)].solved; }

    int selectedSlot() const { return selectedSlot_; }
    int cursorOffset() const { return cursorOffset_; }
    int startingDifficulty() const { return startingDifficulty_; }

    // Test/dev support only: reveals the hidden target for a slot without
    // affecting play state. Never used by rendering or update() — the whole
    // point of the puzzle is that this string is not otherwise knowable.
    const std::string& debugSolutionFor(int index) const {
        return slots_[static_cast<std::size_t>(index)].solution;
    }

private:
    struct Slot {
        int row = 0;
        int col = 0;
        int length = 0;
        Orientation orientation = Orientation::Across;
        std::string solution;
        bool solved = false;
    };

    static constexpr int kMinSlotLength = 3;
    static constexpr int kBaseSize = 9;
    static constexpr int kMaxDifficulty = 6;
    static constexpr float kBlockDensity = 0.16f;
    static constexpr int kMinSlotCount = 8;
    static constexpr int kMaxGenerationAttempts = 6;

    void generate(int startingDifficulty);
    bool generateAttempt();
    int horizontalRunLength(int row, int col) const;
    int verticalRunLength(int row, int col) const;
    void revealSlot(int slotIndex);
    void markBankEntryUsed(const std::string& value);
    std::pair<int, int> cellAt(const Slot& slot, int offset) const;
    std::size_t cellIndex(int row, int col) const {
        return static_cast<std::size_t>(row) * static_cast<std::size_t>(cols_) +
               static_cast<std::size_t>(col);
    }

    void selectCellAt(int row, int col);
    void typeDigit(char digit);
    void backspace();
    void tryCommitSelectedSlot();
    // Marks solved any not-yet-solved slot whose cells all happen to already
    // hold its correct target (e.g. filled in purely by crossing slots),
    // repeating since one such settle can complete another in turn. Never
    // flags a mismatch as a mistake — it only ever gives credit that is
    // already earned.
    void settleIncidentallyCompletedSlots();
    // Slots covering (row, col); -1 when there is none in that orientation.
    int slotAt(int row, int col, Orientation orientation) const;
    bool slotCovers(const Slot& slot, int row, int col) const;

    std::mt19937 rng_;
    int startingDifficulty_ = 0;
    int rows_ = 0;
    int cols_ = 0;
    std::vector<bool> blocked_;
    std::vector<char> playerGrid_;
    std::vector<bool> locked_;
    std::vector<Slot> slots_;
    std::vector<BankEntry> bank_;

    int selectedSlot_ = -1;
    int cursorOffset_ = 0;
    int lastClickedRow_ = -1;
    int lastClickedCol_ = -1;
    int mistakes_ = 0;
    bool solved_ = false;
    float mistakeFlashTimer_ = 0.0f;
};
