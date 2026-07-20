#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "games/Game.h"

class SpatialMemoryGame : public Game {
public:
    SpatialMemoryGame();
    explicit SpatialMemoryGame(std::uint32_t seed);
    // startingDifficulty is the persisted adaptive level (see
    // GameStats::difficultyLevel); it is clamped to [0, kMaxDifficulty] and
    // added on top of kStartLength.
    SpatialMemoryGame(std::uint32_t seed, int startingDifficulty);

    void update(float deltaSeconds, const GameInput& input) override;
    void render(Renderer& renderer, const Rect& area) const override;
    bool isFinished() const override;
    GameResult result() const override;

    int currentRound() const { return round_; }
    int successes() const { return successes_; }
    int sequenceLength() const { return sequenceLength_; }
    bool isAwaitingRecall() const { return phase_ == Phase::Recall; }
    // Cell the player must click next to keep the recall correct; only
    // meaningful while isAwaitingRecall() is true.
    int expectedCell() const { return sequence_[static_cast<std::size_t>(userIndex_)]; }

private:
    enum class Phase { StartDelay, ShowSequence, Recall, Feedback, Done };
    static constexpr int kTotalRounds = 5;
    static constexpr int kStartLength = 3;
    static constexpr int kMaxDifficulty = 10;
    static constexpr int kMinLength = 1;
    static constexpr int kMissesToRetreat = 2;

    void startRound();

    std::mt19937 rng_;
    Phase phase_ = Phase::StartDelay;
    int round_ = 0;
    int successes_ = 0;
    int sequenceLength_ = kStartLength;
    int consecutiveMisses_ = 0;

    std::vector<int> sequence_;
    int sequenceIndex_ = 0;
    int userIndex_ = 0;

    bool lastRoundCorrect_ = false;
    float phaseTimer_ = 0.0f;
    
    int hoverIndex_ = -1;
    int clickIndex_ = -1;
    float clickTimer_ = 0.0f;
};
