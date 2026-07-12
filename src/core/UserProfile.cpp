#include "core/UserProfile.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>

namespace {

long daysSinceEpoch() {
    const std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    int y = local->tm_year + 1900;
    int m = local->tm_mon + 1;
    int d = local->tm_mday;
    if (m < 3) { y -= 1; m += 12; }
    return 365L * y + y / 4 - y / 100 + y / 400 + (153 * m - 457) / 5 + d - 306;
}

}  // namespace

void UserProfile::registerPlayToday() {
    const long today = daysSinceEpoch();
    if (lastPlayedDay_ == today) return;

    if (lastPlayedDay_ == today - 1) {
        ++streakDays_;
    } else {
        streakDays_ = 1;
    }
    lastPlayedDay_ = today;
}

bool UserProfile::hasAchievement(const std::string& id) const {
    return std::find(achievements_.begin(), achievements_.end(), id) != achievements_.end();
}

void UserProfile::unlockAchievement(const std::string& id) {
    if (!hasAchievement(id)) achievements_.push_back(id);
}

void UserProfile::toMap(KeyValueMap& out) const {
    out["profile.name"] = name;
    out["profile.xp"] = std::to_string(xp_);
    out["profile.streak"] = std::to_string(streakDays_);
    out["profile.last_played_day"] = std::to_string(lastPlayedDay_);

    std::ostringstream joined;
    for (size_t i = 0; i < achievements_.size(); ++i) {
        if (i > 0) joined << ',';
        joined << achievements_[i];
    }
    out["profile.achievements"] = joined.str();
}

void UserProfile::fromMap(const KeyValueMap& in) {
    reset();
    if (auto it = in.find("profile.name"); it != in.end() && !it->second.empty())
        name = it->second;
    if (auto it = in.find("profile.xp"); it != in.end())
        xp_ = std::stoi(it->second);
    if (auto it = in.find("profile.streak"); it != in.end())
        streakDays_ = std::stoi(it->second);
    if (auto it = in.find("profile.last_played_day"); it != in.end())
        lastPlayedDay_ = std::stol(it->second);

    if (auto it = in.find("profile.achievements"); it != in.end()) {
        std::istringstream stream(it->second);
        std::string id;
        while (std::getline(stream, id, ',')) {
            if (!id.empty()) achievements_.push_back(id);
        }
    }

    // A break longer than one day resets the streak on the next launch.
    const long today = daysSinceEpoch();
    if (lastPlayedDay_ != 0 && today - lastPlayedDay_ > 1) streakDays_ = 0;
}

void UserProfile::reset() {
    xp_ = 0;
    streakDays_ = 0;
    lastPlayedDay_ = 0;
    achievements_.clear();
}
