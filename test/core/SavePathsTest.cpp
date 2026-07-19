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

bool hasQuarantinedFile(const fs::path& directory, const std::string& prefix) {
    if (!fs::exists(directory)) return false;
    for (const auto& entry : fs::directory_iterator(directory)) {
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix + ".corrupted", 0) == 0) return true;
    }
    return false;
}

// A future-version file in the new location may be real progress this build
// cannot read: migration must preserve it and never migrate over it.
void testFutureVersionNewFileIsPreserved() {
    const fs::path base = uniqueTestDirectory("future_new");
    const fs::path legacyDir = uniqueTestDirectory("future_old");
    fs::create_directories(base);
    fs::create_directories(legacyDir);
    const fs::path legacyMain = legacyDir / "brainboost_save.ini";
    writeValidSave(legacyMain, 300);

    const SavePaths paths(base.string());
    const std::string futureContent = "save.version=999\nprofile.xp=9999\n";
    writeText(paths.mainFile(), futureContent);

    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::NotNeeded);
    TEST_CHECK(readAll(paths.mainFile()) == futureContent);
    TEST_CHECK(fs::exists(legacyMain));  // legacy kept for a compatible build
    fs::remove_all(base);
    fs::remove_all(legacyDir);
}

// The original bug: any file in the new location (even an empty one) blocked
// the migration, so the valid legacy progress was ignored and the user
// started from scratch.
void testCorruptedNewFileDoesNotBlockValidLegacyMigration() {
    const std::string corruptedContents[] = {
        "",                                   // empty
        "save.version=2\nprofile.name=Igor\n"  // truncated mid-write
    };
    for (const std::string& corrupted : corruptedContents) {
        const fs::path base = uniqueTestDirectory("blocked_new");
        const fs::path legacyDir = uniqueTestDirectory("blocked_old");
        fs::create_directories(base);
        fs::create_directories(legacyDir);
        const fs::path legacyMain = legacyDir / "brainboost_save.ini";
        writeValidSave(legacyMain, 450);
        const SavePaths paths(base.string());
        writeText(paths.mainFile(), corrupted);

        TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
                   SavePaths::MigrationResult::Migrated);
        TEST_CHECK(loadXp(paths.mainFile()) == 450);
        // The corrupted new file was preserved for diagnosis, not replaced
        // silently.
        TEST_CHECK(hasQuarantinedFile(base, "brainboost_save.ini"));
        TEST_CHECK(fs::exists(legacyMain.string() + ".migrated"));
        fs::remove_all(base);
        fs::remove_all(legacyDir);
    }
}

// When the new location can recover from its own backup, that recovery wins
// over the legacy file.
void testCorruptedNewFileWithValidNewBackupRecoversBackup() {
    const fs::path base = uniqueTestDirectory("newbak_new");
    const fs::path legacyDir = uniqueTestDirectory("newbak_old");
    fs::create_directories(base);
    fs::create_directories(legacyDir);
    const fs::path legacyMain = legacyDir / "brainboost_save.ini";
    writeValidSave(legacyMain, 300);

    const SavePaths paths(base.string());
    writeValidSave(paths.backupFile(), 777);
    writeText(paths.mainFile(), "");  // corrupted new main

    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::NotNeeded);
    TEST_CHECK(fs::exists(legacyMain));  // legacy untouched

    // The normal load path then restores the new location's own backup.
    UserProfile profile;
    Statistics stats;
    const SaveLoadResult loaded =
        SaveManager(paths.mainFile()).loadDetailed(profile, stats);
    TEST_CHECK(loaded.status == SaveLoadStatus::RecoveredFromBackup);
    TEST_CHECK(profile.xp() == 777);
    fs::remove_all(base);
    fs::remove_all(legacyDir);
}

// Corrupted new main, no new backup, corrupted legacy main — but the legacy
// backup is valid: it must still be migrated.
void testLegacyBackupMigratesOverCorruptedNewFile() {
    const fs::path base = uniqueTestDirectory("legbak_new");
    const fs::path legacyDir = uniqueTestDirectory("legbak_old");
    fs::create_directories(base);
    fs::create_directories(legacyDir);
    const fs::path legacyMain = legacyDir / "brainboost_save.ini";
    writeValidSave(legacyMain.string() + ".bak", 222);
    writeText(legacyMain, "");  // corrupted legacy main

    const SavePaths paths(base.string());
    writeText(paths.mainFile(), "save.version=2\n");  // corrupted new main

    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::Migrated);
    TEST_CHECK(loadXp(paths.mainFile()) == 222);
    TEST_CHECK(hasQuarantinedFile(base, "brainboost_save.ini"));
    fs::remove_all(base);
    fs::remove_all(legacyDir);
}

// Nothing valid on either side: the new file stays in place (the load path
// will quarantine it) and the legacy files are not touched.
void testBothSidesInvalidLeavesEverythingInPlace() {
    const fs::path base = uniqueTestDirectory("bothbad_new");
    const fs::path legacyDir = uniqueTestDirectory("bothbad_old");
    fs::create_directories(base);
    fs::create_directories(legacyDir);
    const fs::path legacyMain = legacyDir / "brainboost_save.ini";
    writeText(legacyMain, "# junk\n");

    const SavePaths paths(base.string());
    writeText(paths.mainFile(), "");

    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::LegacyInvalid);
    TEST_CHECK(fs::exists(paths.mainFile()));
    TEST_CHECK(!hasQuarantinedFile(base, "brainboost_save.ini"));
    TEST_CHECK(fs::exists(legacyMain));
    fs::remove_all(base);
    fs::remove_all(legacyDir);
}

// A filesystem failure mid-migration must leave the legacy files intact so
// the migration can be retried.
void testMigrationFailureLeavesLegacyIntact() {
    const fs::path base = uniqueTestDirectory("fail_new");
    const fs::path legacyDir = uniqueTestDirectory("fail_old");
    fs::create_directories(base);
    fs::create_directories(legacyDir);
    const fs::path legacyMain = legacyDir / "brainboost_save.ini";
    writeValidSave(legacyMain, 640);

    // Read-only destination directory: the copy cannot be created.
    fs::permissions(base, fs::perms::owner_read | fs::perms::owner_exec);
    const SavePaths paths(base.string());
    TEST_CHECK(paths.migrateLegacyFrom(legacyMain.string()) ==
               SavePaths::MigrationResult::Failed);
    fs::permissions(base, fs::perms::owner_all);

    TEST_CHECK(fs::exists(legacyMain));  // untouched, retry possible
    TEST_CHECK(loadXp(legacyMain) == 640);
    TEST_CHECK(!fs::exists(paths.mainFile()));
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
    testFutureVersionNewFileIsPreserved();
    testCorruptedNewFileDoesNotBlockValidLegacyMigration();
    testCorruptedNewFileWithValidNewBackupRecoversBackup();
    testLegacyBackupMigratesOverCorruptedNewFile();
    testBothSidesInvalidLeavesEverythingInPlace();
    testMigrationFailureLeavesLegacyIntact();
    testInvalidLegacyFilesAreLeftUntouched();
    std::cout << "All SavePaths tests passed!\n";
    return 0;
}
