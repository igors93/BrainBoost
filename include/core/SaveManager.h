#pragma once

#include <string>

class UserProfile;
class Statistics;

enum class SaveLoadStatus {
    Success,
    RecoveredFromBackup,
    FileNotFound,
    UnsupportedVersion,
    Corrupted,
    IoError
};

struct SaveLoadResult {
    SaveLoadStatus status = SaveLoadStatus::FileNotFound;
    int detectedVersion = 0;
    std::string detail;
    bool safeToWrite = false;

    bool succeeded() const {
        return status == SaveLoadStatus::Success ||
               status == SaveLoadStatus::RecoveredFromBackup;
    }
};

// Persists progress locally as a versioned key=value text file.
class SaveManager {
public:
    static constexpr int kCurrentSaveVersion = 1;

    explicit SaveManager(std::string filePath = "brainboost_save.ini");

    // Loads the main save and, when it is corrupted, attempts recovery from
    // the validated .bak file. Output objects change only after a valid load.
    SaveLoadResult loadDetailed(UserProfile& profile, Statistics& stats) const;

    bool load(UserProfile& profile, Statistics& stats) const;

    // Writes through a temporary file. Only a validated current save may
    // replace the backup; corrupted files are quarantined instead.
    bool save(const UserProfile& profile, const Statistics& stats) const;

    const std::string& filePath() const { return filePath_; }

private:
    std::string filePath_;
};
