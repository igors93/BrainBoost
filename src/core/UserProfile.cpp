#include "core/UserProfile.h"

#include "core/SaveNumbers.h"

#include <algorithm>
#include <ctime>
#include <sstream>

namespace {

std::int64_t daysSinceEpoch() {
    const std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    if (local == nullptr) return 0;

    int year = local->tm_year + 1900;
    int month = local->tm_mon + 1;
    const int day = local->tm_mday;
    if (month < 3) {
        --year;
        month += 12;
    }
    return 365LL * year + year / 4 - year / 100 + year / 400 +
           (153LL * month - 457) / 5 + day - 306;
}

bool isSafeAchievementId(const std::string& id) {
    return !id.empty() && id.find_first_of(",\r\n=") == std::string::npos;
}

}  // namespace

void UserProfile::addXp(std::int64_t amount) {
    if (amount <= 0 || xp_ >= kMaxXp) return;

    const std::int64_t remaining = static_cast<std::int64_t>(kMaxXp) - xp_;
    if (amount >= remaining) {
        xp_ = kMaxXp;
        return;
    }
    xp_ += static_cast<int>(amount);
}

void UserProfile::registerPlayToday() {
    const std::int64_t today = daysSinceEpoch();
    if (today <= 0 || lastPlayedDay_ == today) return;

    if (lastPlayedDay_ == today - 1) {
        if (streakDays_ < std::numeric_limits<int>::max()) ++streakDays_;
    } else {
        streakDays_ = 1;
    }
    lastPlayedDay_ = today;
}

bool UserProfile::hasAchievement(const std::string& id) const {
    return std::find(achievements_.begin(), achievements_.end(), id) != achievements_.end();
}

void UserProfile::unlockAchievement(const std::string& id) {
    if (isSafeAchievementId(id) && !hasAchievement(id)) achievements_.push_back(id);
}

bool UserProfile::hasReceivedReward(const std::string& id) const {
    return std::find(rewardedAchievements_.begin(), rewardedAchievements_.end(),
                     id) != rewardedAchievements_.end();
}

void UserProfile::markRewardReceived(const std::string& id) {
    if (isSafeAchievementId(id) && !hasReceivedReward(id)) {
        rewardedAchievements_.push_back(id);
    }
}

void UserProfile::markAllUnlockedRewardsReceived() {
    for (const std::string& id : achievements_) markRewardReceived(id);
}

namespace {

std::string joinIds(const std::vector<std::string>& ids) {
    std::ostringstream joined;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) joined << ',';
        joined << ids[i];
    }
    return joined.str();
}

}  // namespace

void UserProfile::toMap(KeyValueMap& out) const {
    out["profile.name"] = name;
    out["profile.xp"] = std::to_string(xp_);
    out["profile.streak"] = std::to_string(streakDays_);
    out["profile.last_played_day"] = std::to_string(lastPlayedDay_);
    out["profile.achievements"] = joinIds(achievements_);
    out["profile.rewarded_achievements"] = joinIds(rewardedAchievements_);
}

void UserProfile::fromMap(const KeyValueMap& in) {
    reset();

    if (const auto it = in.find("profile.name"); it != in.end() && !it->second.empty()) {
        name = it->second.substr(0, 64);
    }

    if (const auto it = in.find("profile.xp"); it != in.end()) {
        int xp = 0;
        if (savenum::parseNonNegativeInt(it->second, kMaxXp, xp)) xp_ = xp;
    }
    if (const auto it = in.find("profile.streak"); it != in.end()) {
        int streak = 0;
        if (savenum::parseNonNegativeInt(
                it->second, std::numeric_limits<int>::max(), streak)) {
            streakDays_ = streak;
        }
    }
    if (const auto it = in.find("profile.last_played_day"); it != in.end()) {
        std::int64_t day = 0;
        if (savenum::parseNonNegative(it->second, INT64_MAX, day)) {
            lastPlayedDay_ = day;
        }
    }

    if (const auto it = in.find("profile.achievements"); it != in.end()) {
        std::istringstream stream(it->second);
        std::string id;
        while (std::getline(stream, id, ',')) {
            unlockAchievement(id);
        }
    }

    if (const auto it = in.find("profile.rewarded_achievements"); it != in.end()) {
        std::istringstream stream(it->second);
        std::string id;
        while (std::getline(stream, id, ',')) {
            markRewardReceived(id);
        }
    }

    const std::int64_t today = daysSinceEpoch();
    if (today > 0 && lastPlayedDay_ != 0 && today - lastPlayedDay_ > 1) {
        streakDays_ = 0;
    }
}

void UserProfile::reset() {
    xp_ = 0;
    streakDays_ = 0;
    lastPlayedDay_ = 0;
    achievements_.clear();
    rewardedAchievements_.clear();
    name = "Jogador";
}

// Only the unlocked list is cleared. The reward ledger survives on purpose:
// conditions that are still met will re-unlock the achievements, but their
// XP reward is never granted a second time.
void UserProfile::resetAchievementsOnly() { achievements_.clear(); }

void UserProfile::resetNameOnly() { name = "Jogador"; }
