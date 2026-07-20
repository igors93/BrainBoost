#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include "../TestUtils.h"
#include "games/MentalMathGame.h"
#include "games/NumberMemoryGame.h"
#include "games/SequenceLogicGame.h"
#include "games/SpatialMemoryGame.h"

namespace {

int solveMentalQuestion(const std::string& question) {
    std::istringstream stream(question);
    int a = 0, b = 0;
    char operation = '+', equals = '=', questionMark = '?';
    stream >> a >> operation >> b >> equals >> questionMark;
    TEST_CHECK(equals == '=');
    TEST_CHECK(questionMark == '?');
    if (operation == '+') return a + b;
    if (operation == '-') return a - b;
    TEST_CHECK(operation == 'x');
    return a * b;
}

int solveSequence(const std::string& sequence) {
    std::istringstream stream(sequence);
    int a = 0, b = 0, c = 0, d = 0;
    char questionMark = '?';
    stream >> a >> b >> c >> d >> questionMark;
    TEST_CHECK(questionMark == '?');
    const int first = b - a;
    const int second = c - b;
    const int third = d - c;
    if (b == a * 2 && c == b * 2 && d == c * 2) return d * 2;
    if (first == second && second == third) return d + third;
    TEST_CHECK(first == third);
    return d + second;
}

void answerMentalQuestion(MentalMathGame& game) {
    GameInput input;
    input.text = std::to_string(solveMentalQuestion(game.currentQuestion()));
    input.confirmPressed = true;
    game.update(0.0f, input);
}

void testMentalMathCompletion() {
    MentalMathGame first(42), second(42);
    TEST_CHECK(first.currentQuestion() == second.currentQuestion());
    for (int question = 0; question < 10; ++question) {
        answerMentalQuestion(first);
        TEST_CHECK(first.correctCount() == question + 1);
        first.update(1.0f, GameInput{});
    }
    TEST_CHECK(first.isFinished());
    TEST_CHECK(first.result().score == 100);
}

void testNumberMemoryProcessesRecall() {
    NumberMemoryGame game(100);
    game.update(10.0f, GameInput{});
    GameInput answer;
    answer.text = game.currentSequence();
    answer.confirmPressed = true;
    game.update(0.0f, answer);
    TEST_CHECK(game.successes() == 1);
    game.update(2.0f, GameInput{});
    TEST_CHECK(game.sequenceLength() == 4);
}

void testNumberMemoryZeroSuccessAwardsNoXp() {
    NumberMemoryGame game(101);
    for (int round = 0; round < 5; ++round) {
        game.update(10.0f, GameInput{});
        GameInput wrong;
        wrong.text = "99999999999999999999";
        wrong.confirmPressed = true;
        game.update(0.0f, wrong);
        game.update(2.0f, GameInput{});
    }
    TEST_CHECK(game.isFinished());
    TEST_CHECK(game.result().correct == 0);
    TEST_CHECK(game.result().score == 0);
    TEST_CHECK(game.result().xpEarned == 0);
}

void testNumberMemoryStartsAtPersistedDifficulty() {
    NumberMemoryGame baseline(100, 0);
    TEST_CHECK(baseline.sequenceLength() == 3);
    NumberMemoryGame game(100, 4);
    TEST_CHECK(game.sequenceLength() == 7);  // kStartLength (3) + 4
    TEST_CHECK(static_cast<int>(game.currentSequence().size()) == 7);
    NumberMemoryGame clamped(100, 999);
    TEST_CHECK(clamped.sequenceLength() == 15);  // 3 + kMaxDifficulty (12)
}

void testSpatialMemoryStartsAtPersistedDifficulty() {
    SpatialMemoryGame baseline(200, 0);
    TEST_CHECK(baseline.sequenceLength() == 3);
    SpatialMemoryGame game(200, 4);
    TEST_CHECK(game.sequenceLength() == 7);  // kStartLength (3) + 4
    SpatialMemoryGame clamped(200, 999);
    TEST_CHECK(clamped.sequenceLength() == 13);  // 3 + kMaxDifficulty (10)
}

void advanceSpatialMemoryToRecall(SpatialMemoryGame& game) {
    while (!game.isAwaitingRecall()) game.update(1.0f, GameInput{});
}

// Bidirectional intra-session staircase: a hit grows the sequence right
// away, but only two consecutive misses shrink it back (see update()).
void testNumberMemoryRetreatsAfterConsecutiveMisses() {
    NumberMemoryGame game(100, 5);
    TEST_CHECK(game.sequenceLength() == 8);  // kStartLength (3) + 5
    for (int miss = 0; miss < 2; ++miss) {
        game.update(10.0f, GameInput{});
        GameInput wrong;
        wrong.text = "99999999999999999999";
        wrong.confirmPressed = true;
        game.update(0.0f, wrong);
        game.update(2.0f, GameInput{});
    }
    TEST_CHECK(game.sequenceLength() == 7);  // one retreat after 2 consecutive misses
}

void testNumberMemoryRetreatFloorsAtMinimum() {
    NumberMemoryGame game(102, 0);  // sequenceLength_ starts at kStartLength (3)
    for (int round = 0; round < 5; ++round) {
        game.update(10.0f, GameInput{});
        GameInput wrong;
        wrong.text = "99999999999999999999";
        wrong.confirmPressed = true;
        game.update(0.0f, wrong);
        game.update(2.0f, GameInput{});
    }
    TEST_CHECK(game.isFinished());
    TEST_CHECK(game.sequenceLength() == 1);  // two retreats floored at kMinLength
}

void testSpatialMemoryRetreatsAfterConsecutiveMisses() {
    SpatialMemoryGame game(300, 5);
    TEST_CHECK(game.sequenceLength() == 8);  // kStartLength (3) + 5
    for (int miss = 0; miss < 2; ++miss) {
        advanceSpatialMemoryToRecall(game);
        GameInput wrong;
        wrong.optionIndex = (game.expectedCell() + 1) % 9;
        game.update(0.0f, wrong);
        game.update(2.0f, GameInput{});
    }
    TEST_CHECK(game.sequenceLength() == 7);  // one retreat after 2 consecutive misses
}

void testMentalMathStartsAtPersistedDifficulty() {
    MentalMathGame game(33, 5);
    TEST_CHECK(game.result().difficulty == 5);  // no streak yet: just the offset
    // Ten correct answers in a row complete three 3-correct streaks, each
    // bumping the intra-session level up by one.
    for (int question = 0; question < 10; ++question) {
        answerMentalQuestion(game);
        game.update(1.0f, GameInput{});
    }
    TEST_CHECK(game.isFinished());
    TEST_CHECK(game.result().difficulty == 5 + 3);
}

void testSequenceLogicStartsAtPersistedDifficulty() {
    SequenceLogicGame game(77, 6);
    TEST_CHECK(game.result().difficulty == 6);  // no streak yet: just the offset
}

// Bidirectional intra-session staircase: a 3-correct streak raises the level
// right away, a shorter 2-wrong streak gives it back (see chooseOption()).
void testSequenceLogicIntraSessionBumpAdapts() {
    SequenceLogicGame game(77, 0);
    TEST_CHECK(game.result().difficulty == 0);

    auto answerCorrectly = [&]() {
        const int expected = solveSequence(game.currentSequence());
        const auto& options = game.currentOptions();
        const auto it = std::find(options.begin(), options.end(), expected);
        TEST_CHECK(it != options.end());
        GameInput input;
        input.optionIndex = static_cast<int>(it - options.begin());
        game.update(0.0f, input);
        game.update(1.0f, GameInput{});
    };
    auto answerWrongly = [&]() {
        const int expected = solveSequence(game.currentSequence());
        const auto& options = game.currentOptions();
        const auto it = std::find(options.begin(), options.end(), expected);
        const int correctIndex = static_cast<int>(it - options.begin());
        GameInput input;
        input.optionIndex = (correctIndex + 1) % 4;
        game.update(0.0f, input);
        game.update(1.0f, GameInput{});
    };

    answerCorrectly();
    answerCorrectly();
    TEST_CHECK(game.result().difficulty == 0);  // streak of 2: not yet advanced
    answerCorrectly();
    TEST_CHECK(game.result().difficulty == 1);  // 3rd consecutive correct

    answerWrongly();
    TEST_CHECK(game.result().difficulty == 1);  // streak of 1 miss: unchanged
    answerWrongly();
    TEST_CHECK(game.result().difficulty == 0);  // 2nd consecutive miss: retreats
}

void testSequenceLogicActions() {
    SequenceLogicGame game(55);
    const int expected = solveSequence(game.currentSequence());
    const auto& options = game.currentOptions();
    const auto it = std::find(options.begin(), options.end(), expected);
    TEST_CHECK(it != options.end());
    GameInput input;
    input.optionIndex = static_cast<int>(it - options.begin());
    game.update(0.0f, input);
    TEST_CHECK(game.correctCount() == 1);
}

}  // namespace

int main() {
    std::cout << "Running game model tests...\n";
    testMentalMathCompletion();
    testNumberMemoryProcessesRecall();
    testNumberMemoryZeroSuccessAwardsNoXp();
    testSequenceLogicActions();
    testNumberMemoryStartsAtPersistedDifficulty();
    testNumberMemoryRetreatsAfterConsecutiveMisses();
    testNumberMemoryRetreatFloorsAtMinimum();
    testSpatialMemoryStartsAtPersistedDifficulty();
    testSpatialMemoryRetreatsAfterConsecutiveMisses();
    testMentalMathStartsAtPersistedDifficulty();
    testSequenceLogicStartsAtPersistedDifficulty();
    testSequenceLogicIntraSessionBumpAdapts();
    std::cout << "All game model tests passed!\n";
    return 0;
}
