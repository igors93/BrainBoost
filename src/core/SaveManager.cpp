#include "core/SaveManager.h"

#include <filesystem>
#include <fstream>
#include <limits>
#include <system_error>
#include <utility>

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

void removeIfPresent(const fs::path& path) {
    std::error_code ignored;
    fs::remove(path, ignored);
}

bool restoreBackup(const fs::path& backupPath, const fs::path& destination) {
    std::error_code ec;
    if (!fs::exists(backupPath, ec) || ec) return false;
    fs::copy_file(backupPath, destination, fs::copy_options::overwrite_existing, ec);
    return !ec;
}

bool replaceWithTemporaryFile(const fs::path& temporaryPath,
                              const fs::path& destination,
                              const fs::path& backupPath) {
    std::error_code ec;

    // POSIX systems normally replace an existing destination atomically here.
    fs::rename(temporaryPath, destination, ec);
    if (!ec) return true;

    // Windows generally refuses rename-over-existing. The old save has already
    // been copied to .bak, so remove-and-rename is recoverable.
    ec.clear();
    if (fs::exists(destination, ec) && !ec) {
        ec.clear();
        fs::remove(destination, ec);
        if (ec) return false;
    }

    ec.clear();
    fs::rename(temporaryPath, destination, ec);
    if (!ec) return true;

    restoreBackup(backupPath, destination);
    return false;
}

}  // namespace

SaveManager::SaveManager(std::string filePath) : filePath_(std::move(filePath)) {}

SaveLoadResult SaveManager::loadDetailed(UserProfile& profile, Statistics& stats) const {
    const fs::path path(filePath_);
    std::error_code ec;
    const bool exists = fs::exists(path, ec);
    if (ec) {
        return {SaveLoadStatus::IoError, 0, ec.message()};
    }
    if (!exists) {
        return {SaveLoadStatus::FileNotFound, 0, "Save file does not exist."};
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return {SaveLoadStatus::IoError, 0, "Save file could not be opened."};
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
        return {SaveLoadStatus::IoError, 0, "An I/O error occurred while reading the save."};
    }

    int version = 0;  // Unversioned legacy saves are version 0.
    if (const auto versionIt = values.find("save.version"); versionIt != values.end()) {
        if (!parseStrictInt(versionIt->second, version) || version < 0) {
            return {SaveLoadStatus::Corrupted, 0, "Invalid save.version value."};
        }
    }

    if (version > kCurrentSaveVersion) {
        return {SaveLoadStatus::UnsupportedVersion, version,
                "The save was created by a newer BrainBoost version."};
    }

    // Parse into temporary objects. A failed load must never partially modify
    // the currently active profile or statistics.
    UserProfile loadedProfile;
    Statistics loadedStats;
    try {
        loadedProfile.fromMap(values);
        loadedStats.fromMap(values);
    } catch (const std::exception& error) {
        return {SaveLoadStatus::Corrupted, version, error.what()};
    } catch (...) {
        return {SaveLoadStatus::Corrupted, version, "Unknown save parsing error."};
    }

    profile = std::move(loadedProfile);
    stats = std::move(loadedStats);
    return {SaveLoadStatus::Success, version, {}};
}

bool SaveManager::load(UserProfile& profile, Statistics& stats) const {
    return loadDetailed(profile, stats).succeeded();
}

bool SaveManager::save(const UserProfile& profile, const Statistics& stats) const {
    KeyValueMap values;
    profile.toMap(values);
    stats.toMap(values);

    const fs::path destination(filePath_);
    const fs::path temporaryPath = destination.string() + ".tmp";
    const fs::path backupPath = destination.string() + ".bak";

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
    for (const auto& [key, value] : values) {
        file << key << '=' << value << '\n';
    }

    file.flush();
    const bool writeSucceeded = file.good();
    file.close();
    if (!writeSucceeded) {
        removeIfPresent(temporaryPath);
        return false;
    }

    std::error_code ec;
    if (fs::exists(destination, ec)) {
        if (ec) {
            removeIfPresent(temporaryPath);
            return false;
        }
        fs::copy_file(destination, backupPath, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            removeIfPresent(temporaryPath);
            return false;
        }
    } else if (ec) {
        removeIfPresent(temporaryPath);
        return false;
    }

    if (!replaceWithTemporaryFile(temporaryPath, destination, backupPath)) {
        removeIfPresent(temporaryPath);
        return false;
    }

    return true;
}
