#pragma once

#include <string>

// Single owner of every persistence path. The main save, its backup, the
// temporary write file and quarantined copies all derive from one
// user-specific base directory, so the directory the program was started
// from never influences where progress lives.
//
// The app builds this from SDL_GetPrefPath(); tests inject temporary
// directories and never touch the real user data directory.
class SavePaths {
public:
    static constexpr const char* kSaveFileName = "brainboost_save.ini";

    explicit SavePaths(std::string baseDirectory);

    const std::string& baseDirectory() const { return baseDirectory_; }
    std::string mainFile() const;
    std::string backupFile() const { return backupFor(mainFile()); }
    std::string temporaryFile() const { return temporaryFor(mainFile()); }
    // Quarantined files are named <prefix>, <prefix>.1, <prefix>.2, ...
    std::string quarantinePrefix() const { return quarantinePrefixFor(mainFile()); }

    // Naming convention shared with SaveManager so every component derives
    // the same sibling files from a given main save path.
    static std::string backupFor(const std::string& mainFile);
    static std::string temporaryFor(const std::string& mainFile);
    static std::string quarantinePrefixFor(const std::string& mainFile);

    enum class MigrationResult {
        NotNeeded,      // no legacy file, or the new location already has progress
        Migrated,       // legacy progress (or its backup) now lives in the new location
        LegacyInvalid,  // legacy files exist but none validated; nothing was changed
        Failed          // a filesystem operation failed; legacy files were kept
    };

    // One-time migration from the legacy working-directory save. Validates
    // the legacy file (or its backup) before copying, never overwrites
    // progress already present in the new location, and only renames the
    // legacy files to "<name>.migrated" after the migrated copy validated.
    MigrationResult migrateLegacyFrom(const std::string& legacyMainFile) const;

private:
    std::string baseDirectory_;
};
