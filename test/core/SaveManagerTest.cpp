#include <cassert>
#include <iostream>
#include "core/SaveManager.h"
#include "core/UserProfile.h"
#include "core/Statistics.h"
#include <cstdio>

void testSaveLoad() {
    const std::string testFile = "test_save.ini";
    SaveManager manager(testFile);

    UserProfile p1;
    Statistics s1;

    p1.addXp(500);
    p1.unlockAchievement("test_achv");

    bool saved = manager.save(p1, s1);
    assert(saved);

    UserProfile p2;
    Statistics s2;
    bool loaded = manager.load(p2, s2);
    assert(loaded);

    assert(p2.xp() == 500);
    assert(p2.hasAchievement("test_achv"));

    // Cleanup
    std::remove(testFile.c_str());
    std::cout << "testSaveLoad passed!" << std::endl;
}

int main() {
    std::cout << "Running SaveManager tests..." << std::endl;
    testSaveLoad();
    std::cout << "All SaveManager tests passed!" << std::endl;
    return 0;
}
