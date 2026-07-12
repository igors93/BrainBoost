#pragma once

#include "core/GameResult.h"
#include "games/GameInput.h"
#include "ui/Rect.h"

class Renderer;

// Base interface for every playable game.
//
// update() owns state transitions and input processing. render() is const and
// only draws the current state. This separation allows the rules to be tested
// without creating an SDL window.
class Game {
public:
    virtual ~Game() = default;

    virtual void update(float deltaSeconds, const GameInput& input,
                        const Rect& area) = 0;
    virtual void render(Renderer& renderer, const Rect& area) const = 0;

    virtual bool isFinished() const = 0;
    virtual GameResult result() const = 0;
};
