#include "app/Application.h"

#include <cstdio>

#include "ui/Theme.h"
#include "ui/Widgets.h"

bool Application::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    window_ = SDL_CreateWindow("BrainBoost — Treinamento Cognitivo",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280,
                               800, SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    if (!renderer_.init(window_)) return false;

    SDL_StartTextInput();  // text fields receive SDL_TEXTINPUT events
    context_.loadProgress();
    return true;
}

float Application::renderContent(const Rect& area, float deltaSeconds) {
    switch (context_.screen) {
        case ScreenId::Home:
            return homeScreen_.render(context_, renderer_, input_, area);
        case ScreenId::Games: {
            renderer_.drawText("Todos os jogos", area.x, area.y, 26, Theme::kText,
                               true);
            const Rect grid{area.x, area.y + 50, area.w, area.h - 50};
            return 50.0f + homeScreen_.renderGameGrid(context_, renderer_, input_, grid);
        }
        case ScreenId::Stats:
            return statsScreen_.render(context_, renderer_, input_, area);
        case ScreenId::Achievements:
            return achievementsScreen_.render(context_, renderer_, input_, area);
        case ScreenId::Settings:
            return settingsScreen_.render(context_, renderer_, input_, area);
        case ScreenId::About:
            return aboutScreen_.render(context_, renderer_, input_, area);
        case ScreenId::Playing:
            gameScreen_.render(context_, renderer_, input_, area, deltaSeconds);
            return area.h;
    }
    return 0.0f;
}

void Application::run() {
    Uint64 previousTicks = SDL_GetTicks64();

    while (!context_.quitRequested) {
        input_.beginFrame();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            input_.handleEvent(event);
            if (event.type == SDL_QUIT) context_.quitRequested = true;
        }

        const Uint64 currentTicks = SDL_GetTicks64();
        const float deltaSeconds =
            static_cast<float>(currentTicks - previousTicks) / 1000.0f;
        previousTicks = currentTicks;

        renderer_.beginFrame(Theme::kBackground);

        // Sidebar rendering (static, no scroll)
        renderer_.setTranslation(0.0f, 0.0f);
        input_.setMouseOffset(0.0f, 0.0f);
        sidebar_.render(context_, renderer_, input_);

        const float padding = 26.0f;
        const Rect content{Sidebar::kWidth + padding, padding,
                           renderer_.width() - Sidebar::kWidth - padding * 2.0f,
                           renderer_.height() - padding * 2.0f};

        // Reset scroll position if screen changed
        if (context_.screen != lastScreen_) {
            scrollY_ = 0.0f;
            contentHeight_ = 0.0f;
            lastScreen_ = context_.screen;
        }

        // Apply scroll delta
        if (context_.screen != ScreenId::Playing) {
            scrollY_ -= input_.scrollDeltaY() * 36.0f;
            const float maxScrollY = std::max(0.0f, contentHeight_ - content.h);
            if (scrollY_ < 0.0f) scrollY_ = 0.0f;
            if (scrollY_ > maxScrollY) scrollY_ = maxScrollY;
        } else {
            scrollY_ = 0.0f;
        }

        // Render main content with translation and clipping
        renderer_.setTranslation(0.0f, -scrollY_);
        input_.setMouseOffset(0.0f, scrollY_);
        renderer_.setClipRect(&content);

        contentHeight_ = renderContent(content, deltaSeconds);

        renderer_.setClipRect(nullptr);
        renderer_.setTranslation(0.0f, 0.0f);
        input_.setMouseOffset(0.0f, 0.0f);

        renderer_.endFrame();
    }
}

void Application::shutdown() {
    context_.saveProgress();

    renderer_.shutdown();
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}
