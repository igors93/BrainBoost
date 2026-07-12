#include <cmath>
#include <iostream>

#include "../TestUtils.h"
#include "games/ReactionTimeGame.h"

namespace {

const Rect kArea{0.0f, 0.0f, 800.0f, 600.0f};

GameInput startInput() {
    GameInput input;
    input.startPressed = true;
    return input;
}

GameInput panelClick() {
    GameInput input;
    input.primaryPressed = true;
    input.pointerX = 100.0f;
    input.pointerY = 100.0f;
    return input;
}

void advanceToGreen(ReactionTimeGame& game) {
    TEST_CHECK(game.isWaiting());
    game.update(game.waitingSecondsRemaining() + 0.01f, GameInput{}, kArea);
    TEST_CHECK(game.isReady());
}

void testSeedMakesWaitingDelayDeterministic() {
    ReactionTimeGame first(1234);
    ReactionTimeGame second(1234);

    first.update(0.0f, startInput(), kArea);
    second.update(0.0f, startInput(), kArea);

    TEST_CHECK(first.isWaiting());
    TEST_CHECK(second.isWaiting());
    TEST_CHECK(std::fabs(first.waitingSecondsRemaining() -
                         second.waitingSecondsRemaining()) < 0.0001f);
}

void testEarlyClickConsumesTrial() {
    ReactionTimeGame game(1);
    game.update(0.0f, startInput(), kArea);
    game.update(0.0f, panelClick(), kArea);

    TEST_CHECK(game.isShowingTooEarly());
    TEST_CHECK(game.falseStarts() == 1);
    TEST_CHECK(game.completedTrials() == 1);
}

void testValidReactionUsesDeterministicElapsedTime() {
    ReactionTimeGame game(2);
    game.update(0.0f, startInput(), kArea);
    advanceToGreen(game);

    game.update(0.250f, GameInput{}, kArea);
    game.update(0.0f, panelClick(), kArea);

    TEST_CHECK(game.isShowingResult());
    TEST_CHECK(game.completedTrials() == 1);
    TEST_CHECK(std::fabs(game.averageReactionMs() - 250.0f) < 0.1f);
    TEST_CHECK(game.result().correct == 1);
}

void testCompleteGameWithOneFalseStart() {
    ReactionTimeGame game(9);
    game.update(0.0f, startInput(), kArea);

    game.update(0.0f, panelClick(), kArea);
    TEST_CHECK(game.completedTrials() == 1);
    game.update(2.0f, GameInput{}, kArea);

    while (!game.isFinished()) {
        if (game.isWaiting()) {
            advanceToGreen(game);
        } else if (game.isReady()) {
            game.update(0.300f, GameInput{}, kArea);
            game.update(0.0f, panelClick(), kArea);
        } else {
            game.update(2.0f, GameInput{}, kArea);
        }
    }

    const GameResult result = game.result();
    TEST_CHECK(game.falseStarts() == 1);
    TEST_CHECK(result.correct == 4);
    TEST_CHECK(result.total == 5);
    TEST_CHECK(result.score >= 5 && result.score <= 100);
}

}  // namespace

int main() {
    std::cout << "Running ReactionTimeGame model tests...\n";
    testSeedMakesWaitingDelayDeterministic();
    testEarlyClickConsumesTrial();
    testValidReactionUsesDeterministicElapsedTime();
    testCompleteGameWithOneFalseStart();
    std::cout << "All ReactionTimeGame model tests passed!\n";
    return 0;
}
