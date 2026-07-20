#include <cmath>
#include <iostream>
#include <memory>

#include "../TestUtils.h"
#include "games/ReactionTimeGame.h"

namespace {

class ManualClock final : public GameClock {
public:
    double nowSeconds() const override { return now_; }
    void advance(double seconds) { now_ += seconds; }
private:
    double now_ = 1000.0;
};

GameInput startInput() { GameInput input; input.startPressed = true; return input; }
GameInput panelClick() { GameInput input; input.primaryActionPressed = true; return input; }

void advanceToGreen(ReactionTimeGame& game) {
    TEST_CHECK(game.isWaiting());
    game.update(game.waitingSecondsRemaining() + 0.01f, GameInput{});
    TEST_CHECK(game.isReady());
}

void testClockMeasuresReactionIndependentlyFromFrameDelta() {
    auto clock = std::make_shared<ManualClock>();
    ReactionTimeGame game(2, clock);
    game.update(0.0f, startInput());
    advanceToGreen(game);
    clock->advance(0.250);
    game.update(5.0f, GameInput{});  // frame delay must not become reaction time
    game.update(0.0f, panelClick());
    TEST_CHECK(std::fabs(game.averageReactionMs() - 250.0f) < 0.1f);
}

void testEarlyClickConsumesTrial() {
    auto clock = std::make_shared<ManualClock>();
    ReactionTimeGame game(1, clock);
    game.update(0.0f, startInput());
    game.update(0.0f, panelClick());
    TEST_CHECK(game.isShowingTooEarly());
    TEST_CHECK(game.falseStarts() == 1);
    TEST_CHECK(game.completedTrials() == 1);
}

void testAllFalseStartsAwardNothing() {
    auto clock = std::make_shared<ManualClock>();
    ReactionTimeGame game(9, clock);
    game.update(0.0f, startInput());
    while (!game.isFinished()) {
        if (game.isWaiting()) game.update(0.0f, panelClick());
        else game.update(2.0f, GameInput{});
    }
    const GameResult result = game.result();
    TEST_CHECK(result.correct == 0);
    TEST_CHECK(result.score == 0);
    TEST_CHECK(result.xpEarned == 0);
}

void testValidSessionProducesBoundedResult() {
    auto clock = std::make_shared<ManualClock>();
    ReactionTimeGame game(12, clock);
    game.update(0.0f, startInput());
    while (!game.isFinished()) {
        if (game.isWaiting()) advanceToGreen(game);
        else if (game.isReady()) {
            clock->advance(0.300);
            game.update(0.0f, panelClick());
        } else game.update(2.0f, GameInput{});
    }
    const GameResult result = game.result();
    TEST_CHECK(result.correct == 5);
    TEST_CHECK(result.score >= 5 && result.score <= 100);
    TEST_CHECK(result.xpEarned > 0);
}

// Higher difficulty shortens the minimum wait before "go" (default floor is
// 1.5s; kMaxDifficulty pulls it down to 0.5s), so across enough seeds the
// minimum observed wait must fall below the default floor.
void testHigherDifficultyShortensMinimumWait() {
    bool sawBelowDefaultFloor = false;
    for (std::uint32_t seed = 1; seed <= 100 && !sawBelowDefaultFloor; ++seed) {
        auto clock = std::make_shared<ManualClock>();
        ReactionTimeGame game(seed, clock, 10);
        game.update(0.0f, startInput());
        TEST_CHECK(game.waitingSecondsRemaining() >= 0.5f - 0.001f);
        if (game.waitingSecondsRemaining() < 1.5f) sawBelowDefaultFloor = true;
    }
    TEST_CHECK(sawBelowDefaultFloor);
}

void testDifficultyIsClampedToMaximum() {
    auto clock = std::make_shared<ManualClock>();
    ReactionTimeGame game(3, clock, 999);
    game.update(0.0f, startInput());
    TEST_CHECK(game.waitingSecondsRemaining() >= 0.5f - 0.001f);
}

// A false start is an unambiguous "too hard right now" signal: it eases the
// intra-session difficulty back immediately (see update()).
void testFalseStartLowersDifficultyImmediately() {
    auto clock = std::make_shared<ManualClock>();
    ReactionTimeGame game(5, clock, 4);
    TEST_CHECK(game.result().difficulty == 4);
    game.update(0.0f, startInput());
    game.update(0.0f, panelClick());  // false start: still in Waiting
    TEST_CHECK(game.isShowingTooEarly());
    TEST_CHECK(game.result().difficulty == 3);
}

// Two clean reactions in a row raise the intra-session difficulty (see
// update()'s Phase::Go branch).
void testCleanStreakRaisesDifficulty() {
    auto clock = std::make_shared<ManualClock>();
    ReactionTimeGame game(6, clock, 2);
    TEST_CHECK(game.result().difficulty == 2);
    game.update(0.0f, startInput());
    for (int trial = 0; trial < 2; ++trial) {
        advanceToGreen(game);
        clock->advance(0.2);
        game.update(0.0f, panelClick());
        game.update(2.0f, GameInput{});  // pass the TrialResult pause
    }
    TEST_CHECK(game.result().difficulty == 3);
}

}  // namespace

int main() {
    std::cout << "Running ReactionTimeGame model tests...\n";
    testClockMeasuresReactionIndependentlyFromFrameDelta();
    testEarlyClickConsumesTrial();
    testAllFalseStartsAwardNothing();
    testValidSessionProducesBoundedResult();
    testHigherDifficultyShortensMinimumWait();
    testDifficultyIsClampedToMaximum();
    testFalseStartLowersDifficultyImmediately();
    testCleanStreakRaisesDifficulty();
    std::cout << "All ReactionTimeGame model tests passed!\n";
    return 0;
}
