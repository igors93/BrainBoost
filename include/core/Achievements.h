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

// What happened to one achievement during an evaluation. Distinguishes a
// real first unlock (reward granted) from a re-unlock after "reset
// achievements only" (the id reappears but no XP is granted), so the UI can
// never announce XP that was not actually received.
struct AchievementUnlockResult {
    const AchievementDef* achievement = nullptr;
    bool firstUnlock = false;    // false: reactivated after a partial reset
    bool rewardGranted = false;  // XP was granted in this evaluation
    int xpGranted = 0;           // 0 when the reward had already been received
};

namespace Achievements {

// Every achievement known by the app, in display order.
const std::vector<AchievementDef>& all();

// Achievements whose conditions the given progress currently satisfies,
// regardless of unlock state. Single source of truth for the conditions,
// shared by evaluate(), the v1 save migration and tests.
std::vector<const AchievementDef*> satisfiedBy(const UserProfile& profile,
                                               const Statistics& stats);

// Checks unlock conditions against the current progress, unlocks anything
// newly earned and reports exactly what happened. The XP reward is granted
// only the first time each achievement is ever unlocked for this progress
// (tracked by the profile's reward ledger).
std::vector<AchievementUnlockResult> evaluate(UserProfile& profile,
                                              const Statistics& stats);

// Save v1 -> v2 migration of the reward ledger. v1 granted XP at unlock
// time, and "reset achievements only" kept the XP while clearing the list —
// so both the listed achievements and every achievement whose condition the
// profile and statistics already satisfy count as rewarded. Only for
// migrating v1 files; never used on new profiles.
void markV1RewardsAsReceived(UserProfile& profile, const Statistics& stats);

}  // namespace Achievements
