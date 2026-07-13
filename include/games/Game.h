#pragma once

#include "core/GameResult.h"
#include "games/GameInput.h"
#include "ui/Rect.h"

class Renderer;

// Base interface for every playable game. update() owns only rules and semantic
// actions; render() owns geometry and drawing.
class Game {
public:
    virtual ~Game() = default;

    virtual void update(float deltaSeconds, const GameInput& input) = 0;
    virtual void render(Renderer& renderer, const Rect& area) const = 0;

    virtual bool isFinished() const = 0;
    virtual GameResult result() const = 0;
};
