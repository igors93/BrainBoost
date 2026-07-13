#pragma once

#include <chrono>

class GameClock {
public:
    virtual ~GameClock() = default;
    virtual double nowSeconds() const = 0;
};

class SteadyGameClock final : public GameClock {
public:
    double nowSeconds() const override {
        using Seconds = std::chrono::duration<double>;
        return Seconds(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};
