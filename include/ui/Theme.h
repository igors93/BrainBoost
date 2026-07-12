#pragma once

#include <cstdint>

#include "core/GameCategory.h"

// RGBA color. Everything the app draws goes through this type.
struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
};

// Builds a Color from 0xRRGGBB plus an optional alpha.
constexpr Color rgb(unsigned int hex, uint8_t alpha = 255) {
    return Color{static_cast<uint8_t>((hex >> 16) & 0xFF),
                 static_cast<uint8_t>((hex >> 8) & 0xFF),
                 static_cast<uint8_t>(hex & 0xFF), alpha};
}

// Returns the color with every channel raised by `amount` (hover effect).
constexpr Color lighten(Color color, int amount = 22) {
    auto up = [](uint8_t channel, int add) {
        const int value = channel + add;
        return static_cast<uint8_t>(value > 255 ? 255 : value);
    };
    return Color{up(color.r, amount), up(color.g, amount), up(color.b, amount),
                 color.a};
}

// Central palette: no screen should hardcode color values.
namespace Theme {

constexpr Color kBackground = rgb(0x0A0E1A);
constexpr Color kSidebar = rgb(0x0D1320);
constexpr Color kPanel = rgb(0x111A2C);
constexpr Color kPanelSoft = rgb(0x18233A);
constexpr Color kGrid = rgb(0x273043);
constexpr Color kButton = rgb(0x1F2A44);
constexpr Color kAccent = rgb(0x3B82F6);
constexpr Color kText = rgb(0xE5EAF3);
constexpr Color kTextMuted = rgb(0x8B93A7);
constexpr Color kSuccess = rgb(0x4ADE80);
constexpr Color kDanger = rgb(0xF87171);
constexpr Color kDangerButton = rgb(0x7F1D1D);
constexpr Color kWarning = rgb(0xFBBF24);

constexpr Color categoryColor(GameCategory category) {
    switch (category) {
        case GameCategory::Memory:    return rgb(0xA78BFA);
        case GameCategory::Reasoning: return rgb(0xFACC15);
        case GameCategory::Attention: return rgb(0x60A5FA);
        case GameCategory::Language:  return rgb(0x4ADE80);
        case GameCategory::Spatial:   return rgb(0x22D3EE);
        case GameCategory::Logic:     return rgb(0xF472B6);
        default:                      return kText;
    }
}

}  // namespace Theme
