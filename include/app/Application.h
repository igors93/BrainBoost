#pragma once

#include <SDL.h>

#include "app/AppContext.h"
#include "ui/AboutScreen.h"
#include "ui/AchievementsScreen.h"
#include "ui/GameScreen.h"
#include "ui/HomeScreen.h"
#include "ui/Input.h"
#include "ui/Renderer.h"
#include "ui/SettingsScreen.h"
#include "ui/Sidebar.h"
#include "ui/StatsScreen.h"

// Owns the SDL window, the renderer and the main loop, and routes each frame
// to the sidebar plus the currently selected screen.
class Application {
public:
    bool init();
    void run();
    void shutdown();

private:
    void renderContent(const Rect& area, float deltaSeconds);

    SDL_Window* window_ = nullptr;
    Renderer renderer_;
    Input input_;
    AppContext context_;

    Sidebar sidebar_;
    HomeScreen homeScreen_;
    GameScreen gameScreen_;
    StatsScreen statsScreen_;
    AchievementsScreen achievementsScreen_;
    SettingsScreen settingsScreen_;
    AboutScreen aboutScreen_;
};
