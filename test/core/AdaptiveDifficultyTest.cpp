#include <iostream>

#include "../TestUtils.h"
#include "core/AdaptiveDifficulty.h"

namespace {

void testStrongSessionRaisesDifficulty() {
    TEST_CHECK(AdaptiveDifficulty::nextDifficulty(0.0f, 80) == 1.0f);
    TEST_CHECK(AdaptiveDifficulty::nextDifficulty(4.0f, 100) == 5.0f);
}

void testWeakSessionLowersDifficulty() {
    TEST_CHECK(AdaptiveDifficulty::nextDifficulty(4.0f, 50) == 3.0f);
    TEST_CHECK(AdaptiveDifficulty::nextDifficulty(4.0f, 0) == 3.0f);
}

void testMiddleBandLeavesDifficultyUnchanged() {
    TEST_CHECK(AdaptiveDifficulty::nextDifficulty(4.0f, 51) == 4.0f);
    TEST_CHECK(AdaptiveDifficulty::nextDifficulty(4.0f, 79) == 4.0f);
    TEST_CHECK(AdaptiveDifficulty::nextDifficulty(4.0f, 65) == 4.0f);
}

void testDifficultyNeverGoesBelowMinimum() {
    TEST_CHECK(AdaptiveDifficulty::nextDifficulty(0.0f, 0) == 0.0f);
    TEST_CHECK(AdaptiveDifficulty::nextDifficulty(0.5f, 10) == 0.0f);
}

}  // namespace

int main() {
    std::cout << "Running AdaptiveDifficultyTest...\n";
    testStrongSessionRaisesDifficulty();
    testWeakSessionLowersDifficulty();
    testMiddleBandLeavesDifficultyUnchanged();
    testDifficultyNeverGoesBelowMinimum();
    std::cout << "All AdaptiveDifficulty tests passed!\n";
    return 0;
}
