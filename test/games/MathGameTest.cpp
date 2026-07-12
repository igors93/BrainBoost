#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <string>

#include "../TestUtils.h"
#include "games/MentalMathGame.h"
#include "games/NumberMemoryGame.h"
#include "games/SequenceLogicGame.h"

namespace {

const Rect kArea{0.0f, 0.0f, 800.0f, 600.0f};

int solveMentalQuestion(const std::string& question) {
    std::istringstream stream(question);
    int a = 0;
    int b = 0;
    char operation = '+';
    char equals = '=';
    char questionMark = '?';
    stream >> a >> operation >> b >> equals >> questionMark;
    TEST_CHECK(stream.good() || stream.eof());
    TEST_CHECK(equals == '=');
    TEST_CHECK(questionMark == '?');

    if (operation == '+') return a + b;
    if (operation == '-') return a - b;
    TEST_CHECK(operation == 'x');
    return a * b;
}

int solveSequence(const std::string& sequence) {
    std::istringstream stream(sequence);
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    char questionMark = '?';
    stream >> a >> b >> c >> d >> questionMark;
    TEST_CHECK(questionMark == '?');

    const int firstStep = b - a;
    const int secondStep = c - b;
    const int thirdStep = d - c;

    if (b == a * 2 && c == b * 2 && d == c * 2) return d * 2;
    if (firstStep == secondStep && secondStep == thirdStep) return d + thirdStep;
    TEST_CHECK(firstStep == thirdStep);
    return d + secondStep;
}

void answerMentalQuestion(MentalMathGame& game) {
    GameInput input;
    input.text = std::to_string(solveMentalQuestion(game.currentQuestion()));
    input.confirmPressed = true;
    game.update(0.0f, input, kArea);
}

void testMentalMathDeterminismAndCompletion() {
    MentalMathGame first(42);
    MentalMathGame second(42);
    TEST_CHECK(first.currentQuestion() == second.currentQuestion());

    for (int question = 0; question < 10; ++question) {
        TEST_CHECK(first.isAwaitingAnswer());
        answerMentalQuestion(first);
        TEST_CHECK(first.isShowingFeedback());
        TEST_CHECK(first.correctCount() == question + 1);
        first.update(1.0f, GameInput{}, kArea);
    }

    TEST_CHECK(first.isFinished());
    const GameResult result = first.result();
    TEST_CHECK(result.correct == 10);
    TEST_CHECK(result.total == 10);
    TEST_CHECK(result.score == 100);
    TEST_CHECK(result.xpEarned > 0);
}

void testNumberMemoryProcessesARealRecall() {
    NumberMemoryGame first(100);
    NumberMemoryGame second(100);
    TEST_CHECK(first.currentSequence() == second.currentSequence());
    TEST_CHECK(first.currentSequence().size() == 3);
    TEST_CHECK(first.isMemorizing());

    first.update(10.0f, GameInput{}, kArea);
    TEST_CHECK(first.isAwaitingRecall());

    GameInput answer;
    answer.text = first.currentSequence();
    answer.confirmPressed = true;
    first.update(0.0f, answer, kArea);

    TEST_CHECK(first.isShowingFeedback());
    TEST_CHECK(first.successes() == 1);

    first.update(2.0f, GameInput{}, kArea);
    TEST_CHECK(first.currentRound() == 1);
    TEST_CHECK(first.sequenceLength() == 4);
    TEST_CHECK(first.isMemorizing());
}

void testNumberMemoryRejectsWrongRecall() {
    NumberMemoryGame game(101);
    game.update(10.0f, GameInput{}, kArea);

    GameInput answer;
    answer.text = "999999999";
    answer.confirmPressed = true;
    game.update(0.0f, answer, kArea);

    TEST_CHECK(game.isShowingFeedback());
    TEST_CHECK(game.successes() == 0);
}

void testSequenceLogicProcessesCorrectAndWrongOptions() {
    SequenceLogicGame first(55);
    SequenceLogicGame second(55);
    TEST_CHECK(first.currentSequence() == second.currentSequence());
    TEST_CHECK(first.currentOptions() == second.currentOptions());

    const int expected = solveSequence(first.currentSequence());
    const auto& options = first.currentOptions();
    const auto expectedIt = std::find(options.begin(), options.end(), expected);
    TEST_CHECK(expectedIt != options.end());
    const int correctIndex = static_cast<int>(expectedIt - options.begin());

    GameInput correct;
    correct.optionIndex = correctIndex;
    first.update(0.0f, correct, kArea);
    TEST_CHECK(first.isShowingFeedback());
    TEST_CHECK(first.correctCount() == 1);

    first.update(1.0f, GameInput{}, kArea);
    TEST_CHECK(first.currentRound() == 1);
    TEST_CHECK(first.isAwaitingChoice());

    const int nextExpected = solveSequence(first.currentSequence());
    const auto& nextOptions = first.currentOptions();
    const auto nextCorrectIt =
        std::find(nextOptions.begin(), nextOptions.end(), nextExpected);
    TEST_CHECK(nextCorrectIt != nextOptions.end());
    const int nextCorrect = static_cast<int>(nextCorrectIt - nextOptions.begin());

    GameInput wrong;
    wrong.optionIndex = (nextCorrect + 1) % 4;
    first.update(0.0f, wrong, kArea);
    TEST_CHECK(first.isShowingFeedback());
    TEST_CHECK(first.correctCount() == 1);
}

}  // namespace

int main() {
    std::cout << "Running game model tests...\n";
    testMentalMathDeterminismAndCompletion();
    testNumberMemoryProcessesARealRecall();
    testNumberMemoryRejectsWrongRecall();
    testSequenceLogicProcessesCorrectAndWrongOptions();
    std::cout << "All game model tests passed!\n";
    return 0;
}
