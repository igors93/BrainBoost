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

bool hasQuarantinedFile(const fs::path& directory, const std::string& prefix) {
    for (const auto& entry : fs::directory_iterator(directory)) {
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix + ".corrupted", 0) == 0) return true;
    }
    return false;
}

void testSaveLoadAndRepeatedReplacement() {
    const fs::path directory = uniqueTestDirectory("save");
    const fs::path savePath = directory / "nested" / "brainboost_save.ini";
    SaveManager manager(savePath.string());

    UserProfile firstProfile;
    Statistics firstStats;
    firstProfile.addXp(500);
    TEST_CHECK(manager.save(firstProfile, firstStats));

    UserProfile secondProfile;
    Statistics secondStats;
    secondProfile.addXp(900);
    TEST_CHECK(manager.save(secondProfile, secondStats));
    TEST_CHECK(fs::exists(savePath.string() + ".bak"));

    UserProfile loadedProfile;
    Statistics loadedStats;
    TEST_CHECK(manager.load(loadedProfile, loadedStats));
    TEST_CHECK(loadedProfile.xp() == 900);
    fs::remove_all(directory);
}

void testCorruptedMainRecoversValidatedBackup() {
    const fs::path directory = uniqueTestDirectory("recovery");
    const fs::path savePath = directory / "save.ini";
    SaveManager manager(savePath.string());

    UserProfile first;
    Statistics stats;
    first.addXp(500);
    TEST_CHECK(manager.save(first, stats));

    UserProfile second;
    second.addXp(900);
    TEST_CHECK(manager.save(second, stats));
    writeText(savePath, "save.version=invalid\nprofile.xp=9999\n");

    UserProfile recovered;
    Statistics recoveredStats;
    const SaveLoadResult result = manager.loadDetailed(recovered, recoveredStats);
    TEST_CHECK(result.status == SaveLoadStatus::RecoveredFromBackup);
    TEST_CHECK(result.safeToWrite);
    TEST_CHECK(recovered.xp() == 500);
    TEST_CHECK(hasQuarantinedFile(directory, "save.ini"));

    UserProfile backup;
    Statistics backupStats;
    SaveManager backupManager(savePath.string() + ".bak");
    TEST_CHECK(backupManager.load(backup, backupStats));
    TEST_CHECK(backup.xp() == 500);

    recovered.addXp(100);
    TEST_CHECK(manager.save(recovered, recoveredStats));
    UserProfile savedAgain;
    Statistics savedAgainStats;
    TEST_CHECK(manager.load(savedAgain, savedAgainStats));
    TEST_CHECK(savedAgain.xp() == 600);
    fs::remove_all(directory);
}

void testCorruptedFileWithoutBackupIsQuarantined() {
    const fs::path directory = uniqueTestDirectory("quarantine");
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    writeText(savePath, "save.version=broken\n");

    SaveManager manager(savePath.string());
    UserProfile profile;
    Statistics stats;
    const SaveLoadResult result = manager.loadDetailed(profile, stats);
    TEST_CHECK(result.status == SaveLoadStatus::Corrupted);
    TEST_CHECK(result.safeToWrite);
    TEST_CHECK(!fs::exists(savePath));
    TEST_CHECK(hasQuarantinedFile(directory, "save.ini"));
    TEST_CHECK(manager.save(profile, stats));
    fs::remove_all(directory);
}

void testFutureVersionRemainsProtected() {
    const fs::path directory = uniqueTestDirectory("future");
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    const std::string original = "save.version=999\nprofile.xp=9999\n";
    writeText(savePath, original);

    SaveManager manager(savePath.string());
    UserProfile profile;
    Statistics stats;
    const SaveLoadResult result = manager.loadDetailed(profile, stats);
    TEST_CHECK(result.status == SaveLoadStatus::UnsupportedVersion);
    TEST_CHECK(!result.safeToWrite);
    TEST_CHECK(!manager.save(profile, stats));
    fs::remove_all(directory);
}

void testDeterministicSaveFailure() {
    const fs::path directory = uniqueTestDirectory("failure");
    fs::create_directories(directory);
    const fs::path blocker = directory / "not_a_directory";
    writeText(blocker, "file");
    SaveManager manager((blocker / "save.ini").string());
    UserProfile profile;
    Statistics stats;
    TEST_CHECK(!manager.save(profile, stats));
    fs::remove_all(directory);
}

}  // namespace

int main() {
    std::cout << "Running SaveManagerTest...\n";
    testSaveLoadAndRepeatedReplacement();
    testCorruptedMainRecoversValidatedBackup();
    testCorruptedFileWithoutBackupIsQuarantined();
    testFutureVersionRemainsProtected();
    testDeterministicSaveFailure();
    std::cout << "All SaveManager tests passed!\n";
    return 0;
}
