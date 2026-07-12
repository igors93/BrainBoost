#pragma once

// Outcome of a finished game session.
struct GameResult {
    int score = 0;     // normalized 0..100, feeds skill statistics
    int xpEarned = 0;  // experience points awarded to the profile
    int correct = 0;   // correct answers / successful trials
    int total = 0;     // total questions / trials
};
