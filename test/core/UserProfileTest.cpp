#include <iostream>
#include "core/UserProfile.h"
#include "../TestUtils.h"

void testAddXp() {
    UserProfile profile;
    TEST_CHECK(profile.xp() == 0);
    TEST_CHECK(profile.level() == 1);

    profile.addXp(600);
    TEST_CHECK(profile.xp() == 600);
    TEST_CHECK(profile.level() == 2);
    TEST_CHECK(profile.xpIntoLevel() == 100);

    std::cout << "testAddXp passed!" << std::endl;
}

void testStreak() {
    UserProfile profile;
    TEST_CHECK(profile.streakDays() == 0);

    profile.registerPlayToday();
    TEST_CHECK(profile.streakDays() == 1);

    // If we call it again today, streak should still be 1.
    profile.registerPlayToday();
    TEST_CHECK(profile.streakDays() == 1);

    std::cout << "testStreak passed!" << std::endl;
}

void testMapConversion() {
    UserProfile p1;
    p1.addXp(1200);
    p1.unlockAchievement("first_blood");

    KeyValueMap map;
    p1.toMap(map);

    UserProfile p2;
    p2.fromMap(map);

    TEST_CHECK(p2.xp() == 1200);
    TEST_CHECK(p2.level() == 3);
    TEST_CHECK(p2.hasAchievement("first_blood"));

    std::cout << "testMapConversion passed!" << std::endl;
}

int main() {
    std::cout << "Running UserProfile tests..." << std::endl;
    testAddXp();
    testStreak();
    testMapConversion();
    std::cout << "All UserProfile tests passed!" << std::endl;
    return 0;
}
