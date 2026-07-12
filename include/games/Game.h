#pragma once

#include "core/GameResult.h"
#include "ui/Rect.h"

class Renderer;
class Input;

// Base interface for every playable game.
//
// frame() is called once per frame while the session is active: it advances
// timers/state and draws the game inside `area`. Once isFinished() returns
// true the host (GameScreen) reads result(), applies XP/statistics and
// offers replay or exit.
class Game {
public:
    virtual ~Game() = default;

    virtual void frame(float deltaSeconds, Renderer& renderer, const Input& input,
                       const Rect& area) = 0;

    virtual bool isFinished() const = 0;
    virtual GameResult result() const = 0;
};
