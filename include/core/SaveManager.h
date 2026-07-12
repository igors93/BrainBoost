#pragma once

#include <string>

class UserProfile;
class Statistics;

// Persists progress locally as a "key=value" text file next to where the
// application is run (brainboost_save.ini). No external dependencies.
class SaveManager {
public:
    explicit SaveManager(std::string filePath = "brainboost_save.ini");

    bool load(UserProfile& profile, Statistics& stats) const;
    bool save(const UserProfile& profile, const Statistics& stats) const;

    const std::string& filePath() const { return filePath_; }

private:
    std::string filePath_;
};
