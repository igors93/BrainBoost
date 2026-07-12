#include "core/SaveManager.h"

#include <filesystem>
#include <fstream>
#include <utility>

#include "core/Statistics.h"
#include "core/UserProfile.h"

SaveManager::SaveManager(std::string filePath) : filePath_(std::move(filePath)) {}

bool SaveManager::load(UserProfile& profile, Statistics& stats) const {
    std::ifstream file(filePath_);
    if (!file.is_open()) return false;

    KeyValueMap values;
    std::string line;
    while (std::getline(file, line)) {
        const size_t separator = line.find('=');
        if (separator == std::string::npos || line.empty() || line[0] == '#') continue;
        values[line.substr(0, separator)] = line.substr(separator + 1);
    }

    try {
        profile.fromMap(values);
        stats.fromMap(values);
    } catch (const std::exception&) {
        // Corrupted save: start fresh instead of crashing.
        profile.reset();
        stats.reset();
        return false;
    }
    return true;
}

bool SaveManager::save(const UserProfile& profile, const Statistics& stats) const {
    KeyValueMap values;
    profile.toMap(values);
    stats.toMap(values);

    std::filesystem::path path(filePath_);
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    std::string tmpPath = filePath_ + ".tmp";

    std::ofstream file(tmpPath, std::ios::trunc);
    if (!file.is_open()) return false;

    file << "# BrainBoost save file\n";
    for (const auto& [key, value] : values) {
        file << key << '=' << value << '\n';
    }

    file.flush();
    if (!file.good()) {
        file.close();
        std::error_code ec;
        std::filesystem::remove(tmpPath, ec);
        return false;
    }
    file.close();

    std::error_code ec;
    std::filesystem::rename(tmpPath, filePath_, ec);
    if (ec) {
        std::filesystem::remove(tmpPath, ec);
        return false;
    }
    return true;
}
