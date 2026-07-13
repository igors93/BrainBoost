#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include "../TestUtils.h"
#include "games/MentalMathGame.h"
#include "games/NumberMemoryGame.h"
#include "games/SequenceLogicGame.h"

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
    std::cout << "All game model tests passed!\n";
    return 0;
}
