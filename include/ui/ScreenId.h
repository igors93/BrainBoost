#pragma once

// Which page the content area is currently showing.
enum class ScreenId {
    Home,
    Games,
    Stats,
    Achievements,
    Settings,
    About,
    Playing,  // a game session is active (GameScreen)
};
