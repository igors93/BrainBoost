#pragma once

#include <string>
#include <vector>

class UserProfile;
class Statistics;

// A single achievement definition. Unlock conditions live in
// Achievements.cpp; unlocked ids are stored in the UserProfile.
struct AchievementDef {
    std::string id;
    std::string title;
    std::string description;
    int xpReward = 0;
};

namespace Achievements {

// Every achievement known by the app, in display order.
const std::vector<AchievementDef>& all();

// Checks unlock conditions against the current progress, unlocks anything
// newly earned (also granting its XP) and returns what was unlocked now.
std::vector<const AchievementDef*> evaluate(UserProfile& profile, const Statistics& stats);

}  // namespace Achievements
