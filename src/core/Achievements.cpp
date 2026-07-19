#include "core/Achievements.h"

#include <functional>

#include "core/Statistics.h"
#include "core/UserProfile.h"

namespace {

using Condition = std::function<bool(const UserProfile&, const Statistics&)>;

struct AchievementRule {
    AchievementDef def;
    Condition isMet;
};

const std::vector<AchievementRule>& rules() {
    static const std::vector<AchievementRule> kRules = {
        {{"first_steps", "Primeiros Passos", "Complete 5 jogos", 100},
         [](const UserProfile&, const Statistics& s) { return s.totalGamesPlayed() >= 5; }},

        {{"memory_in_action", "Memória em Ação", "Treine memória em 3 sessões", 150},
         [](const UserProfile&, const Statistics& s) {
             return s.forCategory(GameCategory::Memory).gamesPlayed >= 3;
         }},

        {{"agile_reasoning", "Raciocínio Ágil", "Alcance 80% em Raciocínio", 200},
         [](const UserProfile&, const Statistics& s) {
             return s.forCategory(GameCategory::Reasoning).skill >= 80.0f;
         }},

        {{"dedicated", "Dedicado", "Mantenha uma sequência de 3 dias", 150},
         [](const UserProfile& p, const Statistics&) { return p.streakDays() >= 3; }},

        {{"level_5", "Mente Afiada", "Alcance o nível 5", 250},
         [](const UserProfile& p, const Statistics&) { return p.level() >= 5; }},

        {{"veteran", "Veterano", "Complete 25 jogos", 300},
         [](const UserProfile&, const Statistics& s) { return s.totalGamesPlayed() >= 25; }},
    };
    return kRules;
}

}  // namespace

namespace Achievements {

const std::vector<AchievementDef>& all() {
    static const std::vector<AchievementDef> kDefs = [] {
        std::vector<AchievementDef> defs;
        for (const AchievementRule& rule : rules()) defs.push_back(rule.def);
        return defs;
    }();
    return kDefs;
}

std::vector<const AchievementDef*> satisfiedBy(const UserProfile& profile,
                                               const Statistics& stats) {
    std::vector<const AchievementDef*> satisfied;
    for (const AchievementRule& rule : rules()) {
        if (rule.isMet(profile, stats)) satisfied.push_back(&rule.def);
    }
    return satisfied;
}

std::vector<AchievementUnlockResult> evaluate(UserProfile& profile,
                                              const Statistics& stats) {
    std::vector<AchievementUnlockResult> unlockedNow;
    for (const AchievementRule& rule : rules()) {
        if (profile.hasAchievement(rule.def.id)) continue;
        if (!rule.isMet(profile, stats)) continue;

        profile.unlockAchievement(rule.def.id);

        AchievementUnlockResult unlock;
        unlock.achievement = &rule.def;
        // The reward ledger outlives partial resets, so each achievement's
        // XP is granted at most once for the lifetime of this progress.
        unlock.firstUnlock = !profile.hasReceivedReward(rule.def.id);
        if (unlock.firstUnlock) {
            profile.addXp(rule.def.xpReward);
            profile.markRewardReceived(rule.def.id);
            unlock.rewardGranted = true;
            unlock.xpGranted = rule.def.xpReward;
        }
        unlockedNow.push_back(unlock);
    }
    return unlockedNow;
}

void markV1RewardsAsReceived(UserProfile& profile, const Statistics& stats) {
    // Listed achievements were rewarded at unlock time in v1; a satisfied
    // condition also proves the reward was granted (v1 evaluated after every
    // game), even when "reset achievements only" cleared the visible list.
    profile.markAllUnlockedRewardsReceived();
    for (const AchievementDef* def : satisfiedBy(profile, stats)) {
        profile.markRewardReceived(def->id);
    }
}

}  // namespace Achievements
