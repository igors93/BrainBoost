#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "../TestUtils.h"
#include "core/SaveManager.h"
#include "core/SavePaths.h"
#include "core/Statistics.h"
#include "core/UserProfile.h"

namespace {
namespace fs = std::filesystem;

fs::path uniqueTestDirectory(const char* name) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return fs::temp_directory_path() /
           (std::string("brainboost_paths_") + name + "_" + std::to_string(stamp));
}

std::string readAll(const fs::path& path) {
    std::ifstream file(path);
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

void writeText(const fs::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::trunc);
    file << text;
}

void writeValidSave(const fs::path& path, int xp) {
    UserProfile profile;
    Statistics stats;
    profile.addXp(xp);
    TEST_CHECK(SaveManager(path.string()).save(profile, stats));
}

int loadXp(const fs::path& path) {
    UserProfile profile;
    Statistics stats;
    TEST_CHECK(SaveManager(path.string()).load(profile, stats));
    return profile.xp();
}

void testPathComposition() {
    const SavePaths paths("/data/base");
    const std::string main = (fs::path("/data/base") / "brainboost_save.ini").string();
    TEST_CHECK(paths.mainFile() == main);
    TEST_CHECK(paths.backupFile() == main + ".bak");
    TEST_CHECK(paths.temporaryFile() == main + ".tmp");
    TEST_CHECK(paths.quarantinePrefix() == main + ".corrupted");
}

// The same absolute base directory must resolve to the same save file no
// matter which directory the program was started from.
void testSameSaveRegardlessOfWorkingDirectory() {
    const fs::path base = uniqueTestDirectory("stable");
    const fs::path runFromA = uniqueTestDirectory("cwd_a");
    const fs::path runFromB = uniqueTestDirectory("cwd_b");
    fs::create_directories(runFromA);
    fs::create_directories(runFromB);
    const fs::path originalCwd = fs::current_path();

    const SavePaths paths(base.string());

    // "Run" the program from directory A and save progress.
    fs::current_path(runFromA);
    writeValidSave(paths.mainFile(), 700);

    // "Run" it again from directory B: the very same file must be found.
    fs::current_path(runFromB);
    TEST_CHECK(loadXp(paths.mainFile()) == 700);
    TEST_CHECK(fs::equivalent(fs::path(paths.mainFile()),
                              base / SavePaths::kSaveFileName));

    fs::current_path(originalCwd);
    fs::remove_all(base);
    fs::remove_all(runFromA);
    fs::remove_all(runFromB);
}

void testMigratesValidLegacySaveOnce() {
    const fs::path base = uniqueTestDirectory("migrate_new");
    const fs::path legacyDir = uniqueTestDirectory("migrate_old");
    fs::create_directories(legacyDir);
    const fs::path legacyMain = legacyDir / "brainboost_save.ini";
    writeValidSave(legacyMain, 450);
    const std::string legacyContent = readAll(legacyMain);

    const SavePaths paths(base.string());
    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::Migrated);
    TEST_CHECK(loadXp(paths.mainFile()) == 450);
    TEST_CHECK(readAll(paths.mainFile()) == legacyContent);

    // The legacy file is preserved under a marker name, never deleted blindly.
    TEST_CHECK(!fs::exists(legacyMain));
    TEST_CHECK(fs::exists(legacyMain.string() + ".migrated"));

    // A second run must not migrate again.
    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::NotNeeded);
    fs::remove_all(base);
    fs::remove_all(legacyDir);
}

void testMigrationNeverOverwritesExistingProgress() {
    const fs::path base = uniqueTestDirectory("no_overwrite_new");
    const fs::path legacyDir = uniqueTestDirectory("no_overwrite_old");
    fs::create_directories(legacyDir);
    const fs::path legacyMain = legacyDir / "brainboost_save.ini";
    writeValidSave(legacyMain, 999);

    const SavePaths paths(base.string());
    writeValidSave(paths.mainFile(), 111);

    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::NotNeeded);
    TEST_CHECK(loadXp(paths.mainFile()) == 111);
    TEST_CHECK(fs::exists(legacyMain));  // legacy untouched
    fs::remove_all(base);
    fs::remove_all(legacyDir);
}

void testMigratesValidLegacyBackupWhenMainIsCorrupted() {
    const fs::path base = uniqueTestDirectory("backup_new");
    const fs::path legacyDir = uniqueTestDirectory("backup_old");
    fs::create_directories(legacyDir);
    const fs::path legacyMain = legacyDir / "brainboost_save.ini";

    writeValidSave(legacyMain, 300);
    fs::copy_file(legacyMain, legacyMain.string() + ".bak");
    writeText(legacyMain, "");  // main destroyed after the backup was taken

    const SavePaths paths(base.string());
    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::Migrated);
    TEST_CHECK(loadXp(paths.mainFile()) == 300);
    fs::remove_all(base);
    fs::remove_all(legacyDir);
}

void testMigrationCarriesValidBackupAlong() {
    const fs::path base = uniqueTestDirectory("carry_new");
    const fs::path legacyDir = uniqueTestDirectory("carry_old");
    fs::create_directories(legacyDir);
    const fs::path legacyMain = legacyDir / "brainboost_save.ini";

    writeValidSave(legacyMain, 200);
    writeValidSave(legacyMain, 500);  // second save creates a .bak with xp=200

    const SavePaths paths(base.string());
    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::Migrated);
    TEST_CHECK(loadXp(paths.mainFile()) == 500);
    TEST_CHECK(loadXp(paths.backupFile()) == 200);
    fs::remove_all(base);
    fs::remove_all(legacyDir);
}

void testInvalidLegacyFilesAreLeftUntouched() {
    const fs::path base = uniqueTestDirectory("invalid_new");
    const fs::path legacyDir = uniqueTestDirectory("invalid_old");
    fs::create_directories(legacyDir);
    const fs::path legacyMain = legacyDir / "brainboost_save.ini";
    writeText(legacyMain, "# only a comment\n");
    writeText(legacyMain.string() + ".bak", "save.version=1\n");

    const SavePaths paths(base.string());
    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::LegacyInvalid);
    TEST_CHECK(!fs::exists(paths.mainFile()));
    TEST_CHECK(fs::exists(legacyMain));
    TEST_CHECK(fs::exists(legacyMain.string() + ".bak"));
    fs::remove_all(base);
    fs::remove_all(legacyDir);
}

}  // namespace

int main() {
    std::cout << "Running SavePathsTest...\n";
    testPathComposition();
    testSameSaveRegardlessOfWorkingDirectory();
    testMigratesValidLegacySaveOnce();
    testMigrationNeverOverwritesExistingProgress();
    testMigratesValidLegacyBackupWhenMainIsCorrupted();
    testMigrationCarriesValidBackupAlong();
    testInvalidLegacyFilesAreLeftUntouched();
    std::cout << "All SavePaths tests passed!\n";
    return 0;
}
