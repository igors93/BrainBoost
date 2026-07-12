#include <iostream>
#include <filesystem>
#include "../TestUtils.h"
#include "core/SaveManager.h"
#include "core/UserProfile.h"
#include "core/Statistics.h"
#include "app/AppContext.h"
#include <cstdio>

void testSaveLoad() {
    const std::string testFile = "test_dir/test_save.ini";
    SaveManager manager(testFile);

    UserProfile p1;
    Statistics s1;

    p1.addXp(500);
    p1.unlockAchievement("test_achv");

    bool saved = manager.save(p1, s1);
    TEST_CHECK(saved);
    TEST_CHECK(std::filesystem::exists("test_dir"));
    TEST_CHECK(std::filesystem::exists(testFile));

    UserProfile p2;
    Statistics s2;
    bool loaded = manager.load(p2, s2);
    TEST_CHECK(loaded);

    TEST_CHECK(p2.xp() == 500);
    TEST_CHECK(p2.hasAchievement("test_achv"));

    // Test saving failure (invalid path)
    SaveManager badManager("/invalid_root_dir/test.ini");
    TEST_CHECK(!badManager.save(p1, s1));
    
    // Cleanup
    std::filesystem::remove_all("test_dir");
    std::cout << "testSaveLoad passed!" << std::endl;
}

int main() {
    std::cout << "Running SaveManager tests..." << std::endl;
    testSaveLoad();
    std::cout << "All SaveManager tests passed!" << std::endl;
    return 0;
}
