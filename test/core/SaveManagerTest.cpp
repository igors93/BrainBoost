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

std::string readAll(const fs::path& path) {
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

// Structurally broken files must never load as an (empty) valid profile.
void expectCorrupted(const char* label, const std::string& content) {
    const fs::path directory = uniqueTestDirectory(label);
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    writeText(savePath, content);

    SaveManager manager(savePath.string());
    UserProfile profile;
    Statistics stats;
    const SaveLoadResult result = manager.loadDetailed(profile, stats);
    TEST_CHECK(result.status == SaveLoadStatus::Corrupted);
    TEST_CHECK(!fs::exists(savePath));  // quarantined, not silently accepted
    TEST_CHECK(hasQuarantinedFile(directory, "save.ini"));
    fs::remove_all(directory);
}

// Complete, structurally valid v2 file; `xpValue` lets one required value be
// replaced by invalid data while everything else stays intact.
std::string fullSaveText(const std::string& xpValue) {
    std::string text =
        "save.version=2\n"
        "profile.name=Igor\n"
        "profile.xp=" + xpValue + "\n"
        "profile.streak=0\n"
        "profile.last_played_day=0\n"
        "profile.achievements=\n"
        "profile.rewarded_achievements=\n"
        "stats.total_games=0\n"
        "stats.history=\n";
    for (int index = 0; index < kCategoryCount; ++index) {
        const std::string prefix = "stats.category." + std::to_string(index);
        text += prefix + ".skill=0.000000\n" + prefix + ".games=0\n";
    }
    return text;
}

void testStructurallyInvalidFilesAreCorrupted() {
    expectCorrupted("empty", "");
    expectCorrupted("comments", "# BrainBoost save file\n# nothing else\n");
    expectCorrupted("version_only", "save.version=2\n");
    expectCorrupted("missing_field",
                    "save.version=2\nprofile.name=Igor\nprofile.xp=100\n");
    expectCorrupted("negative_required", fullSaveText("-5"));
    expectCorrupted("non_numeric_required", fullSaveText("many"));
    expectCorrupted("version_zero", "save.version=0\nprofile.xp=1\n");
}

void testCompleteCurrentFormatLoads() {
    const fs::path directory = uniqueTestDirectory("valid_v2");
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    writeText(savePath, fullSaveText("321"));

    SaveManager manager(savePath.string());
    UserProfile profile;
    Statistics stats;
    const SaveLoadResult result = manager.loadDetailed(profile, stats);
    TEST_CHECK(result.status == SaveLoadStatus::Success);
    TEST_CHECK(result.detectedVersion == 2);
    TEST_CHECK(profile.xp() == 321);
    fs::remove_all(directory);
}

void testTruncatedFileIsCorrupted() {
    const fs::path directory = uniqueTestDirectory("truncated");
    const fs::path savePath = directory / "save.ini";
    SaveManager manager(savePath.string());

    UserProfile profile;
    Statistics stats;
    profile.addXp(800);
    TEST_CHECK(manager.save(profile, stats));

    // Simulate a write interrupted halfway through.
    const std::string full = readAll(savePath);
    fs::remove(savePath.string() + ".bak");
    writeText(savePath, full.substr(0, full.size() / 2));

    UserProfile loaded;
    Statistics loadedStats;
    const SaveLoadResult result = manager.loadDetailed(loaded, loadedStats);
    TEST_CHECK(result.status == SaveLoadStatus::Corrupted);
    TEST_CHECK(hasQuarantinedFile(directory, "save.ini"));
    fs::remove_all(directory);
}

// The original bug: an empty main file was accepted as a valid (blank)
// profile and the backup was never even considered.
void testEmptyMainFileRecoversFromValidBackup() {
    const fs::path directory = uniqueTestDirectory("empty_recovery");
    const fs::path savePath = directory / "save.ini";
    SaveManager manager(savePath.string());

    UserProfile first;
    Statistics stats;
    first.addXp(500);
    TEST_CHECK(manager.save(first, stats));
    UserProfile second;
    second.addXp(900);
    TEST_CHECK(manager.save(second, stats));  // .bak now holds xp=500
    writeText(savePath, "");

    UserProfile recovered;
    Statistics recoveredStats;
    const SaveLoadResult result = manager.loadDetailed(recovered, recoveredStats);
    TEST_CHECK(result.status == SaveLoadStatus::RecoveredFromBackup);
    TEST_CHECK(recovered.xp() == 500);
    TEST_CHECK(hasQuarantinedFile(directory, "save.ini"));
    TEST_CHECK(manager.save(recovered, recoveredStats));  // keeps saving normally
    fs::remove_all(directory);
}

void testMainAndBackupBothInvalid() {
    const fs::path directory = uniqueTestDirectory("both_invalid");
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    writeText(savePath, "");
    writeText(savePath.string() + ".bak", "save.version=2\n");

    SaveManager manager(savePath.string());
    UserProfile profile;
    Statistics stats;
    const SaveLoadResult result = manager.loadDetailed(profile, stats);
    TEST_CHECK(result.status == SaveLoadStatus::Corrupted);
    TEST_CHECK(result.safeToWrite);  // fresh start allowed after quarantine
    TEST_CHECK(hasQuarantinedFile(directory, "save.ini"));
    TEST_CHECK(manager.save(profile, stats));
    fs::remove_all(directory);
}

void testVersion1FileStillLoadsAndIsUpgraded() {
    const fs::path directory = uniqueTestDirectory("v1_compat");
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    std::string v1 =
        "save.version=1\n"
        "profile.name=Igor\n"
        "profile.xp=750\n"
        "profile.streak=0\n"
        "profile.last_played_day=0\n"
        "profile.achievements=first_steps\n"
        "stats.total_games=5\n"
        "stats.history=\n";
    for (int index = 0; index < kCategoryCount; ++index) {
        const std::string prefix = "stats.category." + std::to_string(index);
        v1 += prefix + ".skill=40.000000\n" + prefix + ".games=1\n";
    }
    writeText(savePath, v1);

    SaveManager manager(savePath.string());
    UserProfile profile;
    Statistics stats;
    const SaveLoadResult result = manager.loadDetailed(profile, stats);
    TEST_CHECK(result.status == SaveLoadStatus::Success);
    TEST_CHECK(result.detectedVersion == 1);
    TEST_CHECK(profile.xp() == 750);
    // v1 migration: unlocked achievements count as already rewarded.
    TEST_CHECK(profile.hasReceivedReward("first_steps"));

    TEST_CHECK(manager.save(profile, stats));
    const std::string upgraded = readAll(savePath);
    TEST_CHECK(upgraded.find("save.version=2") != std::string::npos);
    TEST_CHECK(upgraded.find("profile.rewarded_achievements=first_steps") !=
               std::string::npos);
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
    testStructurallyInvalidFilesAreCorrupted();
    testCompleteCurrentFormatLoads();
    testTruncatedFileIsCorrupted();
    testEmptyMainFileRecoversFromValidBackup();
    testMainAndBackupBothInvalid();
    testVersion1FileStillLoadsAndIsUpgraded();
    testDeterministicSaveFailure();
    std::cout << "All SaveManager tests passed!\n";
    return 0;
}
