#include "games/ReactionTimeGame.h"

#include <algorithm>
#include <numeric>

namespace {

Rect startButtonRect(const Rect& area) {
    return Rect{area.centerX() - 110.0f, area.y + 150.0f, 220.0f, 46.0f};
}

Rect reactionPanelRect(const Rect& area) {
    return Rect{area.x + 16.0f, area.y + 48.0f, area.w - 32.0f,
                area.h - 64.0f};
}

}  // namespace

ReactionTimeGame::ReactionTimeGame() : rng_(std::random_device{}()) {}

ReactionTimeGame::ReactionTimeGame(std::uint32_t seed) : rng_(seed) {}

void ReactionTimeGame::startTrial() {
    std::uniform_real_distribution<float> delay(1.5f, 3.5f);
    waitTimer_ = delay(rng_);
    goElapsedSeconds_ = 0.0f;
    phase_ = Phase::Waiting;
}

float ReactionTimeGame::averageReactionMs() const {
    if (reactionTimesMs_.empty()) return 0.0f;
    const float sum =
        std::accumulate(reactionTimesMs_.begin(), reactionTimesMs_.end(), 0.0f);
    return sum / static_cast<float>(reactionTimesMs_.size());
}

void ReactionTimeGame::finishPauseOrGame() {
    if (trial_ >= kTotalTrials) {
        phase_ = Phase::Done;
    } else {
        startTrial();
    }
}

void ReactionTimeGame::update(float deltaSeconds, const GameInput& input,
                              const Rect& area) {
    if (phase_ == Phase::Done) return;

    const float safeDelta = std::max(0.0f, deltaSeconds);
    const Rect panel = reactionPanelRect(area);
    const bool panelClicked =
        input.primaryPressed && panel.contains(input.pointerX, input.pointerY);

    // Process actions against the state visible at the beginning of the frame.
    // This prevents a click made during the red phase from being accepted after
    // the countdown crosses zero in the same update.
    if (phase_ == Phase::Instructions) {
        const bool clickedStart =
            input.primaryPressed &&
            startButtonRect(area).contains(input.pointerX, input.pointerY);
        if (input.startPressed || clickedStart || input.confirmPressed) {
            startTrial();
            return;
        }
    } else if (phase_ == Phase::Waiting && panelClicked) {
        ++trial_;
        ++falseStarts_;
        phase_ = Phase::TooEarly;
        pauseTimer_ = 1.2f;
        return;
    } else if (phase_ == Phase::Go && panelClicked) {
        const float reactionMs =
            std::max(0.0f, (goElapsedSeconds_ + safeDelta) * 1000.0f);
        reactionTimesMs_.push_back(reactionMs);
        ++trial_;
        phase_ = Phase::TrialResult;
        pauseTimer_ = 1.2f;
        return;
    }

    switch (phase_) {
        case Phase::Waiting:
            waitTimer_ -= safeDelta;
            if (waitTimer_ <= 0.0f) {
                waitTimer_ = 0.0f;
                goElapsedSeconds_ = 0.0f;
                phase_ = Phase::Go;
            }
            break;
        case Phase::Go:
            goElapsedSeconds_ += safeDelta;
            break;
        case Phase::TrialResult:
        case Phase::TooEarly:
            pauseTimer_ -= safeDelta;
            if (pauseTimer_ <= 0.0f) finishPauseOrGame();
            break;
        default:
            break;
    }
}

bool ReactionTimeGame::isFinished() const { return phase_ == Phase::Done; }

GameResult ReactionTimeGame::result() const {
    GameResult result;
    result.correct = static_cast<int>(reactionTimesMs_.size());
    result.total = kTotalTrials;

    if (reactionTimesMs_.empty()) {
        result.score = 5;
    } else {
        const float avg = averageReactionMs();
        result.score = static_cast<int>(
            std::clamp((650.0f - avg) / 4.5f, 5.0f, 100.0f));
    }
    result.xpEarned = result.score / 2 + 20;
    result.difficulty = std::max(0, kTotalTrials - falseStarts_);
    return result;
}
