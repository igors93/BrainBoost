#include "core/SavePaths.h"

#include <filesystem>
#include <system_error>
#include <utility>

#include "core/SaveManager.h"
#include "core/Statistics.h"
#include "core/UserProfile.h"

namespace {
namespace fs = std::filesystem;

bool fileValidates(const std::string& path) {
    UserProfile profile;
    Statistics stats;
    return SaveManager::inspectFile(path, profile, stats).status ==
           SaveLoadStatus::Success;
}

// Copies `source` into `destination` through a temporary file, accepting the
// result only if the copy itself validates as a complete save.
bool copyValidatedFile(const std::string& source, const fs::path& temporary,
                       const fs::path& destination) {
    std::error_code ec;
    fs::copy_file(source, temporary, fs::copy_options::overwrite_existing, ec);
    if (ec) return false;

    if (!fileValidates(temporary.string())) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        return false;
    }

    fs::rename(temporary, destination, ec);
    if (ec) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        return false;
    }
    return true;
}

void markLegacyMigrated(const fs::path& legacyFile) {
    // Best effort: repetition is already prevented by the progress now
    // existing in the new location, so a failed rename is not fatal.
    std::error_code ignored;
    fs::rename(legacyFile, legacyFile.string() + ".migrated", ignored);
}

}  // namespace

SavePaths::SavePaths(std::string baseDirectory)
    : baseDirectory_(std::move(baseDirectory)) {}

std::string SavePaths::mainFile() const {
    return (fs::path(baseDirectory_) / kSaveFileName).string();
}

std::string SavePaths::backupFor(const std::string& mainFile) {
    return mainFile + ".bak";
}

std::string SavePaths::temporaryFor(const std::string& mainFile) {
    return mainFile + ".tmp";
}

std::string SavePaths::quarantinePrefixFor(const std::string& mainFile) {
    return mainFile + ".corrupted";
}

namespace {

SaveLoadStatus inspectStatus(const std::string& path) {
    UserProfile ignoredProfile;
    Statistics ignoredStats;
    return SaveManager::inspectFile(path, ignoredProfile, ignoredStats).status;
}

}  // namespace

SavePaths::MigrationResult SavePaths::migrateLegacyFrom(
    const std::string& legacyMainFile) const {
    const fs::path legacyMain(legacyMainFile);
    const fs::path newMain(mainFile());

    std::error_code ec;
    if (fs::weakly_canonical(legacyMain, ec) == fs::weakly_canonical(newMain, ec)) {
        return MigrationResult::NotNeeded;
    }

    // Classify what already lives in the new location before deciding: only
    // a file with no recoverable or protected progress may be replaced.
    bool newMainNeedsQuarantine = false;
    switch (inspectStatus(mainFile())) {
        case SaveLoadStatus::Success:
        case SaveLoadStatus::RecoveredFromBackup:
            return MigrationResult::NotNeeded;  // valid progress: never replace
        case SaveLoadStatus::UnsupportedVersion:
        case SaveLoadStatus::IoError:
            // Possibly real progress this build cannot read: preserve it and
            // let the load path keep it protected.
            return MigrationResult::NotNeeded;
        case SaveLoadStatus::Corrupted: {
            const SaveLoadStatus backupStatus = inspectStatus(backupFile());
            if (backupStatus == SaveLoadStatus::Success ||
                backupStatus == SaveLoadStatus::UnsupportedVersion ||
                backupStatus == SaveLoadStatus::IoError) {
                // The new location can recover from its own backup (or must
                // protect it); normal load handles both cases.
                return MigrationResult::NotNeeded;
            }
            newMainNeedsQuarantine = true;
            break;
        }
        case SaveLoadStatus::FileNotFound:
            break;
        case SaveLoadStatus::BackupProtected:
            return MigrationResult::NotNeeded;  // inspectFile never emits this
    }

    const fs::path legacyBackup(backupFor(legacyMainFile));
    ec.clear();
    const bool legacyMainExists = fs::exists(legacyMain, ec) && !ec;
    ec.clear();
    const bool legacyBackupExists = fs::exists(legacyBackup, ec) && !ec;
    if (!legacyMainExists && !legacyBackupExists) return MigrationResult::NotNeeded;

    const bool mainValid = legacyMainExists && fileValidates(legacyMainFile);
    const bool backupValid =
        legacyBackupExists && fileValidates(legacyBackup.string());
    if (!mainValid && !backupValid) return MigrationResult::LegacyInvalid;

    ec.clear();
    fs::create_directories(fs::path(baseDirectory_), ec);
    if (ec) return MigrationResult::Failed;

    // Only now that a validated legacy source exists is the corrupted new
    // file moved aside — preserved for diagnosis, never silently replaced.
    if (newMainNeedsQuarantine && !SaveManager::quarantine(mainFile())) {
        return MigrationResult::Failed;
    }

    const std::string source = mainValid ? legacyMainFile : legacyBackup.string();
    if (!copyValidatedFile(source, temporaryFile(), newMain)) {
        return MigrationResult::Failed;
    }

    // Bring a valid legacy backup along so recovery keeps working, without
    // ever replacing a backup that already exists in the new location.
    if (mainValid && backupValid) {
        std::error_code ignored;
        if (!fs::exists(backupFile(), ignored)) {
            fs::copy_file(legacyBackup, backupFile(),
                          fs::copy_options::skip_existing, ignored);
        }
    }

    // The migrated copy validated, so the legacy files may now be renamed;
    // they are kept with a ".migrated" suffix instead of being deleted.
    if (legacyMainExists) markLegacyMigrated(legacyMain);
    if (legacyBackupExists) markLegacyMigrated(legacyBackup);
    return MigrationResult::Migrated;
}
