#include "core/SaveManager.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <system_error>
#include <utility>

#include "core/SavePaths.h"
#include "core/Statistics.h"
#include "core/UserProfile.h"

namespace {
namespace fs = std::filesystem;

bool parseStrictInt(const std::string& text, int& value) {
    try {
        std::size_t parsed = 0;
        const long long candidate = std::stoll(text, &parsed, 10);
        if (parsed != text.size() || candidate < std::numeric_limits<int>::min() ||
            candidate > std::numeric_limits<int>::max()) {
            return false;
        }
        value = static_cast<int>(candidate);
        return true;
    } catch (...) {
        return false;
    }
}

bool isNonNegativeInteger(const std::string& text) {
    try {
        std::size_t parsed = 0;
        return std::stoll(text, &parsed, 10) >= 0 && parsed == text.size();
    } catch (...) {
        return false;
    }
}

bool isFiniteFloatText(const std::string& text) {
    try {
        std::size_t parsed = 0;
        const float value = std::stof(text, &parsed);
        return parsed == text.size() && std::isfinite(value);
    } catch (...) {
        return false;
    }
}

// Structural validation: a file is only accepted when every field the writer
// always emits for `version` is present and every required numeric value
// parses as a non-negative number. This rejects empty, comment-only,
// version-only and truncated files before they can silently reset progress.
// Out-of-range but parseable values (e.g. XP above the cap) are still
// normalized later by fromMap(); only unparseable or negative required
// values count as corruption.
bool validateStructure(const KeyValueMap& values, int version,
                       std::string& issue) {
    static constexpr const char* kRequiredPresent[] = {
        "profile.name", "profile.achievements", "stats.history"};
    static constexpr const char* kRequiredNonNegative[] = {
        "profile.xp", "profile.streak", "profile.last_played_day",
        "stats.total_games"};

    for (const char* key : kRequiredPresent) {
        if (values.find(key) == values.end()) {
            issue = std::string("missing required field ") + key;
            return false;
        }
    }

    for (const char* key : kRequiredNonNegative) {
        const auto it = values.find(key);
        if (it == values.end()) {
            issue = std::string("missing required field ") + key;
            return false;
        }
        if (!isNonNegativeInteger(it->second)) {
            issue = std::string("invalid value for required field ") + key;
            return false;
        }
    }

    for (int index = 0; index < kCategoryCount; ++index) {
        const std::string prefix = "stats.category." + std::to_string(index);
        const auto skillIt = values.find(prefix + ".skill");
        if (skillIt == values.end() || !isFiniteFloatText(skillIt->second)) {
            issue = "missing or invalid " + prefix + ".skill";
            return false;
        }
        const auto gamesIt = values.find(prefix + ".games");
        if (gamesIt == values.end() || !isNonNegativeInteger(gamesIt->second)) {
            issue = "missing or invalid " + prefix + ".games";
            return false;
        }
    }

    if (version >= 2 &&
        values.find("profile.rewarded_achievements") == values.end()) {
        issue = "missing required field profile.rewarded_achievements";
        return false;
    }
    return true;
}

void removeIfPresent(const fs::path& path) {
    std::error_code ignored;
    fs::remove(path, ignored);
}

SaveLoadResult loadSingleFile(const fs::path& path, UserProfile& profile,
                              Statistics& stats) {
    std::error_code ec;
    const bool exists = fs::exists(path, ec);
    if (ec) return {SaveLoadStatus::IoError, 0, ec.message(), false};
    if (!exists) {
        return {SaveLoadStatus::FileNotFound, 0,
                "Save file does not exist.", true};
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return {SaveLoadStatus::IoError, 0,
                "Save file could not be opened.", false};
    }

    KeyValueMap values;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos || separator == 0) continue;
        values[line.substr(0, separator)] = line.substr(separator + 1);
    }
    if (file.bad()) {
        return {SaveLoadStatus::IoError, 0,
                "An I/O error occurred while reading the save.", false};
    }

    const auto versionIt = values.find("save.version");
    if (versionIt == values.end()) {
        return {SaveLoadStatus::Corrupted, 0,
                "Missing save.version (empty, comment-only or truncated file).",
                false};
    }

    int version = 0;
    if (!parseStrictInt(versionIt->second, version) || version < 1) {
        return {SaveLoadStatus::Corrupted, 0,
                "Invalid save.version value.", false};
    }

    if (version > SaveManager::kCurrentSaveVersion) {
        return {SaveLoadStatus::UnsupportedVersion, version,
                "The save was created by a newer BrainBoost version.", false};
    }

    std::string structuralIssue;
    if (!validateStructure(values, version, structuralIssue)) {
        return {SaveLoadStatus::Corrupted, version,
                "Structurally invalid save: " + structuralIssue + ".", false};
    }

    UserProfile loadedProfile;
    Statistics loadedStats;
    try {
        loadedProfile.fromMap(values);
        loadedStats.fromMap(values);
    } catch (const std::exception& error) {
        return {SaveLoadStatus::Corrupted, version, error.what(), false};
    } catch (...) {
        return {SaveLoadStatus::Corrupted, version,
                "Unknown save parsing error.", false};
    }

    // Explicit v1 -> v2 migration: v1 granted achievement XP at unlock time,
    // so every unlocked achievement's reward counts as already received.
    if (version == 1) loadedProfile.markAllUnlockedRewardsReceived();

    profile = std::move(loadedProfile);
    stats = std::move(loadedStats);
    return {SaveLoadStatus::Success, version, {}, true};
}

fs::path nextQuarantinePath(const fs::path& original) {
    const std::string prefix = SavePaths::quarantinePrefixFor(original.string());
    fs::path candidate = prefix;
    std::error_code ec;
    for (int suffix = 1; fs::exists(candidate, ec) && !ec; ++suffix) {
        candidate = prefix + "." + std::to_string(suffix);
    }
    return candidate;
}

bool quarantineFile(const fs::path& path, fs::path* quarantinedPath = nullptr) {
    std::error_code ec;
    if (!fs::exists(path, ec) || ec) return !ec;

    const fs::path target = nextQuarantinePath(path);
    fs::rename(path, target, ec);
    if (ec) return false;
    if (quarantinedPath != nullptr) *quarantinedPath = target;
    return true;
}

bool copyFileReplacing(const fs::path& source, const fs::path& destination) {
    std::error_code ec;
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing, ec);
    return !ec;
}

bool replaceWithTemporaryFile(const fs::path& temporaryPath,
                              const fs::path& destination,
                              const fs::path& backupPath,
                              bool backupCanRestore) {
    std::error_code ec;

    fs::rename(temporaryPath, destination, ec);
    if (!ec) return true;

    ec.clear();
    if (fs::exists(destination, ec) && !ec) {
        ec.clear();
        fs::remove(destination, ec);
        if (ec) return false;
    }

    ec.clear();
    fs::rename(temporaryPath, destination, ec);
    if (!ec) return true;

    if (backupCanRestore) copyFileReplacing(backupPath, destination);
    return false;
}

}  // namespace

SaveManager::SaveManager(std::string filePath) : filePath_(std::move(filePath)) {}

SaveLoadResult SaveManager::inspectFile(const std::string& path,
                                        UserProfile& profile,
                                        Statistics& stats) {
    return loadSingleFile(fs::path(path), profile, stats);
}

SaveLoadResult SaveManager::loadDetailed(UserProfile& profile,
                                         Statistics& stats) const {
    const fs::path path(filePath_);

    UserProfile mainProfile;
    Statistics mainStats;
    SaveLoadResult mainResult = loadSingleFile(path, mainProfile, mainStats);
    if (mainResult.succeeded()) {
        profile = std::move(mainProfile);
        stats = std::move(mainStats);
        return mainResult;
    }

    if (mainResult.status != SaveLoadStatus::Corrupted) return mainResult;

    const fs::path backupPath = SavePaths::backupFor(filePath_);
    UserProfile backupProfile;
    Statistics backupStats;
    const SaveLoadResult backupResult =
        loadSingleFile(backupPath, backupProfile, backupStats);

    fs::path quarantinedPath;
    if (!quarantineFile(path, &quarantinedPath)) {
        mainResult.detail += " The corrupted file could not be quarantined.";
        mainResult.safeToWrite = false;
        return mainResult;
    }

    if (backupResult.succeeded()) {
        if (!copyFileReplacing(backupPath, path)) {
            return {SaveLoadStatus::IoError, backupResult.detectedVersion,
                    "A valid backup was found, but restoring the main save failed.",
                    false};
        }

        profile = std::move(backupProfile);
        stats = std::move(backupStats);
        return {SaveLoadStatus::RecoveredFromBackup,
                backupResult.detectedVersion,
                "Progress recovered from the validated backup. Corrupted file: " +
                    quarantinedPath.string(),
                true};
    }

    mainResult.detail += " The corrupted file was moved to " +
                         quarantinedPath.string() + ".";
    mainResult.safeToWrite = true;
    return mainResult;
}

bool SaveManager::load(UserProfile& profile, Statistics& stats) const {
    return loadDetailed(profile, stats).succeeded();
}

bool SaveManager::save(const UserProfile& profile,
                       const Statistics& stats) const {
    KeyValueMap values;
    profile.toMap(values);
    stats.toMap(values);

    const fs::path destination(filePath_);
    const fs::path temporaryPath = SavePaths::temporaryFor(filePath_);
    const fs::path backupPath = SavePaths::backupFor(filePath_);

    if (destination.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(destination.parent_path(), ec);
        if (ec) return false;
    }

    removeIfPresent(temporaryPath);
    std::ofstream file(temporaryPath, std::ios::trunc);
    if (!file.is_open()) return false;

    file << "# BrainBoost save file\n";
    file << "save.version=" << kCurrentSaveVersion << '\n';
    for (const auto& [key, value] : values) file << key << '=' << value << '\n';

    file.flush();
    const bool writeSucceeded = file.good();
    file.close();
    if (!writeSucceeded) {
        removeIfPresent(temporaryPath);
        return false;
    }

    bool backupCanRestore = false;
    std::error_code ec;
    if (fs::exists(destination, ec)) {
        if (ec) {
            removeIfPresent(temporaryPath);
            return false;
        }

        UserProfile currentProfile;
        Statistics currentStats;
        const SaveLoadResult current =
            loadSingleFile(destination, currentProfile, currentStats);

        if (current.status == SaveLoadStatus::UnsupportedVersion ||
            current.status == SaveLoadStatus::IoError) {
            removeIfPresent(temporaryPath);
            return false;
        }

        if (current.succeeded()) {
            if (!copyFileReplacing(destination, backupPath)) {
                removeIfPresent(temporaryPath);
                return false;
            }
            backupCanRestore = true;
        } else if (current.status == SaveLoadStatus::Corrupted) {
            if (!quarantineFile(destination)) {
                removeIfPresent(temporaryPath);
                return false;
            }
        }
    } else if (ec) {
        removeIfPresent(temporaryPath);
        return false;
    }

    if (!replaceWithTemporaryFile(temporaryPath, destination, backupPath,
                                  backupCanRestore)) {
        removeIfPresent(temporaryPath);
        return false;
    }
    return true;
}
