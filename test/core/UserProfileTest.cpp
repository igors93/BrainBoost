#include <cstdint>
#include <iostream>
#include <limits>

#include "../TestUtils.h"
#include "core/UserProfile.h"

void testXpProgressionAndSaturation() {
    UserProfile profile;
    TEST_CHECK(profile.xp() == 0);
    TEST_CHECK(profile.level() == 1);

    profile.addXp(600);
    TEST_CHECK(profile.xp() == 600);
    TEST_CHECK(profile.level() == 2);
    TEST_CHECK(profile.xpIntoLevel() == 100);

    profile.addXp(-500);
    TEST_CHECK(profile.xp() == 600);

    profile.addXp(std::numeric_limits<std::int64_t>::max());
    TEST_CHECK(profile.xp() == profile.maxXp());
    profile.addXp(1);
    TEST_CHECK(profile.xp() == profile.maxXp());
}

void testStreak() {
    UserProfile profile;
    profile.registerPlayToday();
    TEST_CHECK(profile.streakDays() == 1);
    profile.registerPlayToday();
    TEST_CHECK(profile.streakDays() == 1);
}

void testValidatedMapConversion() {
    KeyValueMap values;
    values["profile.name"] = "Igor";
    values["profile.xp"] = "999999999999999999999999";
    values["profile.streak"] = "-7";
    values["profile.last_played_day"] = "invalid";
    values["profile.achievements"] = "first_win,first_win,bad=id,second_win";

    UserProfile profile;
    profile.fromMap(values);
    TEST_CHECK(profile.name == "Igor");
    TEST_CHECK(profile.xp() == 0);  // Out-of-range input is rejected.
    TEST_CHECK(profile.streakDays() == 0);
    TEST_CHECK(profile.achievements().size() == 2);
    TEST_CHECK(profile.hasAchievement("first_win"));
    TEST_CHECK(profile.hasAchievement("second_win"));
}

void testResetScopes() {
    UserProfile profile;
    profile.addXp(1000);
    profile.unlockAchievement("first_win");
    profile.name = "Igor";

    profile.resetAchievementsOnly();
    TEST_CHECK(!profile.hasAchievement("first_win"));
    TEST_CHECK(profile.xp() == 1000);
    TEST_CHECK(profile.name == "Igor");

    profile.resetNameOnly();
    TEST_CHECK(profile.name == "Jogador");
    TEST_CHECK(profile.xp() == 1000);

    profile.reset();
    TEST_CHECK(profile.xp() == 0);
    TEST_CHECK(profile.name == "Jogador");
}

int main() {
    std::cout << "Running UserProfileTest...\n";
    testXpProgressionAndSaturation();
    testStreak();
    testValidatedMapConversion();
    testResetScopes();
    std::cout << "All UserProfile tests passed!\n";
    return 0;
}
