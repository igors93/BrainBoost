#pragma once

#include <map>
#include <string>
#include <vector>

using KeyValueMap = std::map<std::string, std::string>;

// Player identity and progression: name, XP, level, daily streak and
// unlocked achievements. Serialized through toMap()/fromMap().
class UserProfile {
public:
    std::string name = "Jogador";

    int xp() const { return xp_; }
    void addXp(int amount) { xp_ += amount; }

    // Level grows every kXpPerLevel points, starting at level 1.
    int level() const { return 1 + xp_ / kXpPerLevel; }
    int xpIntoLevel() const { return xp_ % kXpPerLevel; }
    int xpPerLevel() const { return kXpPerLevel; }

    int streakDays() const { return streakDays_; }

    // Must be called whenever a game session finishes; keeps the daily
    // streak counter in sync with the calendar.
    void registerPlayToday();

    bool hasAchievement(const std::string& id) const;
    void unlockAchievement(const std::string& id);
    const std::vector<std::string>& achievements() const { return achievements_; }

    void toMap(KeyValueMap& out) const;
    void fromMap(const KeyValueMap& in);

    void reset();

private:
    static constexpr int kXpPerLevel = 500;

    int xp_ = 0;
    int streakDays_ = 0;
    long lastPlayedDay_ = 0;  // days since Unix epoch, 0 = never played
    std::vector<std::string> achievements_;
};
