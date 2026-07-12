#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "../TestUtils.h"
#include "core/SaveManager.h"
#include "core/Statistics.h"
#include "core/UserProfile.h"

namespace {
namespace fs = std::filesystem;

fs::path uniqueTestDirectory(const char* name) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return fs::temp_directory_path() /
           (std::string("brainboost_") + name + "_" + std::to_string(stamp));
}

void writeText(const fs::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::trunc);
    file << text;
}

void testSaveLoadAndRepeatedReplacement() {
    const fs::path directory = uniqueTestDirectory("save");
    const fs::path savePath = directory / "nested" / "brainboost_save.ini";
    SaveManager manager(savePath.string());

    UserProfile firstProfile;
    Statistics firstStats;
    firstProfile.addXp(500);
    firstProfile.unlockAchievement("test_achievement");
    TEST_CHECK(manager.save(firstProfile, firstStats));
    TEST_CHECK(fs::exists(savePath));

    UserProfile secondProfile;
    Statistics secondStats;
    secondProfile.addXp(900);
    TEST_CHECK(manager.save(secondProfile, secondStats));
    TEST_CHECK(fs::exists(savePath.string() + ".bak"));

    UserProfile loadedProfile;
    Statistics loadedStats;
    const SaveLoadResult result = manager.loadDetailed(loadedProfile, loadedStats);
    TEST_CHECK(result.status == SaveLoadStatus::Success);
    TEST_CHECK(loadedProfile.xp() == 900);

    UserProfile backupProfile;
    Statistics backupStats;
    SaveManager backupManager(savePath.string() + ".bak");
    TEST_CHECK(backupManager.load(backupProfile, backupStats));
    TEST_CHECK(backupProfile.xp() == 500);

    fs::remove_all(directory);
}

void testVersionStatusesDoNotModifyOutput() {
    const fs::path directory = uniqueTestDirectory("version");
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";

    UserProfile profile;
    Statistics stats;
    profile.addXp(321);

    writeText(savePath, "save.version=999\nprofile.xp=9999\n");
    SaveManager manager(savePath.string());
    SaveLoadResult result = manager.loadDetailed(profile, stats);
    TEST_CHECK(result.status == SaveLoadStatus::UnsupportedVersion);
    TEST_CHECK(profile.xp() == 321);

    writeText(savePath, "save.version=invalid\nprofile.xp=9999\n");
    result = manager.loadDetailed(profile, stats);
    TEST_CHECK(result.status == SaveLoadStatus::Corrupted);
    TEST_CHECK(profile.xp() == 321);

    fs::remove_all(directory);
}

void testDeterministicSaveFailure() {
    const fs::path directory = uniqueTestDirectory("failure");
    fs::create_directories(directory);
    const fs::path parentBlocker = directory / "not_a_directory";
    writeText(parentBlocker, "file");

    SaveManager manager((parentBlocker / "save.ini").string());
    UserProfile profile;
    Statistics stats;
    TEST_CHECK(!manager.save(profile, stats));

    fs::remove_all(directory);
}

}  // namespace

int main() {
    std::cout << "Running SaveManagerTest...\n";
    testSaveLoadAndRepeatedReplacement();
    testVersionStatusesDoNotModifyOutput();
    testDeterministicSaveFailure();
    std::cout << "All SaveManager tests passed!\n";
    return 0;
}
