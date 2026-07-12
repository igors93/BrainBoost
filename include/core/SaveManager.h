#pragma once

#include <string>

class UserProfile;
class Statistics;

enum class SaveLoadStatus {
    Success,
    FileNotFound,
    UnsupportedVersion,
    Corrupted,
    IoError
};

struct SaveLoadResult {
    SaveLoadStatus status = SaveLoadStatus::FileNotFound;
    int detectedVersion = 0;
    std::string detail;

    bool succeeded() const { return status == SaveLoadStatus::Success; }
};

// Persists progress locally as a versioned key=value text file.
class SaveManager {
public:
    static constexpr int kCurrentSaveVersion = 1;

    explicit SaveManager(std::string filePath = "brainboost_save.ini");

    // Detailed loading result lets callers distinguish a missing save from an
    // incompatible or unreadable one. The output objects are changed only on
    // successful loading.
    SaveLoadResult loadDetailed(UserProfile& profile, Statistics& stats) const;

    // Compatibility wrapper for existing callers and tests.
    bool load(UserProfile& profile, Statistics& stats) const;

    // Writes through a temporary file and keeps the previous file as .bak.
    bool save(const UserProfile& profile, const Statistics& stats) const;

    const std::string& filePath() const { return filePath_; }

private:
    std::string filePath_;
};
