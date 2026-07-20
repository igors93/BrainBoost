#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../TestUtils.h"
#include "games/FillInPuzzleGame.h"

namespace {

// Selects slotIndex (clicking its first cell, toggling orientation if the
// first click landed on the crossing slot instead) and types its true
// solution into whichever of its cells are not already locked by a solved
// crossing slot — exactly what a player would type, left to right.
void typeSlotSolution(FillInPuzzleGame& game, int slotIndex) {
    const int firstRow = game.slotRow(slotIndex);
    const int firstCol = game.slotCol(slotIndex);
    const int cols = game.cols();

    GameInput click;
    click.optionIndex = firstRow * cols + firstCol;
    game.update(0.0f, click);
    if (game.selectedSlot() != slotIndex) {
        game.update(0.0f, click);  // toggle orientation at an intersection
    }
    TEST_CHECK(game.selectedSlot() == slotIndex);

    const bool across = game.slotOrientation(slotIndex) == FillInPuzzleGame::Orientation::Across;
    const std::string solution = game.debugSolutionFor(slotIndex);
    for (int i = 0; i < static_cast<int>(solution.size()); ++i) {
        const int row = across ? firstRow : firstRow + i;
        const int col = across ? firstCol + i : firstCol;
        if (game.isLocked(row, col)) continue;
        GameInput type;
        type.text = std::string(1, solution[static_cast<std::size_t>(i)]);
        game.update(0.0f, type);
    }
}

void testEveryOpenCellBelongsToASlot() {
    for (std::uint32_t seed : {1u, 2u, 3u, 42u}) {
        FillInPuzzleGame game(seed, 0);
        std::vector<bool> covered(static_cast<std::size_t>(game.rows() * game.cols()), false);
        for (int i = 0; i < game.slotCount(); ++i) {
            const bool across = game.slotOrientation(i) == FillInPuzzleGame::Orientation::Across;
            for (int k = 0; k < game.slotLength(i); ++k) {
                const int row = across ? game.slotRow(i) : game.slotRow(i) + k;
                const int col = across ? game.slotCol(i) + k : game.slotCol(i);
                covered[static_cast<std::size_t>(row * game.cols() + col)] = true;
            }
        }
        for (int row = 0; row < game.rows(); ++row) {
            for (int col = 0; col < game.cols(); ++col) {
                if (game.isBlocked(row, col)) continue;
                TEST_CHECK(covered[static_cast<std::size_t>(row * game.cols() + col)]);
            }
        }
        TEST_CHECK(game.slotCount() > 0);
        TEST_CHECK(static_cast<int>(game.bank().size()) == game.slotCount());
    }
}

void testDifficultySizingIsMonotonicAndClamped() {
    FillInPuzzleGame low(7, 0);
    FillInPuzzleGame mid(7, 3);
    FillInPuzzleGame high(7, 6);
    FillInPuzzleGame clamped(7, 999);

    TEST_CHECK(low.rows() == 9);   // kBaseSize
    TEST_CHECK(mid.rows() == 12);  // kBaseSize + 3
    TEST_CHECK(high.rows() == 15);  // kBaseSize + kMaxDifficulty
    TEST_CHECK(clamped.rows() == high.rows());
    TEST_CHECK(low.cols() == low.rows());  // square grid
}

void testGivenClueCountDecreasesWithDifficulty() {
    FillInPuzzleGame easy(11, 0);
    FillInPuzzleGame hard(11, 6);

    auto usedCount = [](const FillInPuzzleGame& game) {
        int used = 0;
        for (const auto& entry : game.bank()) used += entry.used ? 1 : 0;
        return used;
    };

    TEST_CHECK(usedCount(easy) == 3);
    TEST_CHECK(usedCount(hard) == 1);
}

void testGivenSlotsAreLockedAndMatchTheirSolution() {
    FillInPuzzleGame game(21, 0);
    int solvedAtStart = 0;
    for (int i = 0; i < game.slotCount(); ++i) {
        if (!game.slotSolved(i)) continue;
        ++solvedAtStart;
        const bool across = game.slotOrientation(i) == FillInPuzzleGame::Orientation::Across;
        const std::string solution = game.debugSolutionFor(i);
        for (int k = 0; k < game.slotLength(i); ++k) {
            const int row = across ? game.slotRow(i) : game.slotRow(i) + k;
            const int col = across ? game.slotCol(i) + k : game.slotCol(i);
            TEST_CHECK(game.isLocked(row, col));
            TEST_CHECK(game.displayedChar(row, col) == solution[static_cast<std::size_t>(k)]);
        }
    }
    TEST_CHECK(solvedAtStart == 3);
}

void testCorrectSubmissionSolvesSlotAndMarksBankUsed() {
    FillInPuzzleGame game(5, 0);
    int target = -1;
    for (int i = 0; i < game.slotCount(); ++i) {
        if (!game.slotSolved(i)) { target = i; break; }
    }
    TEST_CHECK(target >= 0);

    const int usedBefore = static_cast<int>(
        std::count_if(game.bank().begin(), game.bank().end(),
                      [](const FillInPuzzleGame::BankEntry& e) { return e.used; }));

    typeSlotSolution(game, target);

    TEST_CHECK(game.slotSolved(target));
    const int usedAfter = static_cast<int>(
        std::count_if(game.bank().begin(), game.bank().end(),
                      [](const FillInPuzzleGame::BankEntry& e) { return e.used; }));
    TEST_CHECK(usedAfter == usedBefore + 1);
    TEST_CHECK(game.mistakes() == 0);
}

void testWrongSubmissionIncrementsMistakesAndClearsSlot() {
    FillInPuzzleGame game(6, 0);
    int target = -1;
    for (int i = 0; i < game.slotCount(); ++i) {
        if (!game.slotSolved(i)) { target = i; break; }
    }
    TEST_CHECK(target >= 0);

    const int firstRow = game.slotRow(target);
    const int firstCol = game.slotCol(target);
    const bool across = game.slotOrientation(target) == FillInPuzzleGame::Orientation::Across;
    const int length = game.slotLength(target);

    GameInput click;
    click.optionIndex = firstRow * game.cols() + firstCol;
    game.update(0.0f, click);
    if (game.selectedSlot() != target) game.update(0.0f, click);
    TEST_CHECK(game.selectedSlot() == target);

    // Deliberately wrong: every open digit is one off from the true
    // solution, so the assembled string is guaranteed to differ from it
    // (rather than relying on a fixed digit being statistically unlikely
    // to match).
    const std::string solution = game.debugSolutionFor(target);
    for (int i = 0; i < length; ++i) {
        const int row = across ? firstRow : firstRow + i;
        const int col = across ? firstCol + i : firstCol;
        if (game.isLocked(row, col)) continue;
        GameInput type;
        const char correct = solution[static_cast<std::size_t>(i)];
        const char wrong = static_cast<char>('0' + (correct - '0' + 1) % 10);
        type.text = std::string(1, wrong);
        game.update(0.0f, type);
    }

    TEST_CHECK(!game.slotSolved(target));
    TEST_CHECK(game.mistakes() == 1);
    for (int i = 0; i < length; ++i) {
        const int row = across ? firstRow : firstRow + i;
        const int col = across ? firstCol + i : firstCol;
        if (game.isLocked(row, col)) continue;
        TEST_CHECK(game.displayedChar(row, col) == '\0');
    }
}

void testSolvingEverySlotFinishesTheGameWithAPositiveResult() {
    FillInPuzzleGame game(9, 0);
    for (int i = 0; i < game.slotCount(); ++i) {
        if (!game.slotSolved(i)) typeSlotSolution(game, i);
    }

    TEST_CHECK(game.isFinished());
    TEST_CHECK(game.isSolved());
    const GameResult result = game.result();
    TEST_CHECK(result.correct == result.total);
    TEST_CHECK(result.total == game.slotCount());
    TEST_CHECK(result.score >= 30 && result.score <= 100);
    TEST_CHECK(result.xpEarned > 0);
    TEST_CHECK(result.difficulty == 0);
}

void testDeterministicGenerationForSameSeedAndDifficulty() {
    FillInPuzzleGame first(123, 2);
    FillInPuzzleGame second(123, 2);
    TEST_CHECK(first.rows() == second.rows());
    TEST_CHECK(first.slotCount() == second.slotCount());
    TEST_CHECK(first.bank().size() == second.bank().size());
    for (std::size_t i = 0; i < first.bank().size(); ++i) {
        TEST_CHECK(first.bank()[i].number == second.bank()[i].number);
    }
    for (int i = 0; i < first.slotCount(); ++i) {
        TEST_CHECK(first.debugSolutionFor(i) == second.debugSolutionFor(i));
    }
}

}  // namespace

int main() {
    std::cout << "Running FillInPuzzleGameTest...\n";
    testEveryOpenCellBelongsToASlot();
    testDifficultySizingIsMonotonicAndClamped();
    testGivenClueCountDecreasesWithDifficulty();
    testGivenSlotsAreLockedAndMatchTheirSolution();
    testCorrectSubmissionSolvesSlotAndMarksBankUsed();
    testWrongSubmissionIncrementsMistakesAndClearsSlot();
    testSolvingEverySlotFinishesTheGameWithAPositiveResult();
    testDeterministicGenerationForSameSeedAndDifficulty();
    std::cout << "All FillInPuzzleGame tests passed!\n";
    return 0;
}
