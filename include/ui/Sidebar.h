#pragma once

struct AppContext;
class Renderer;
class Input;

// Left navigation column: logo, page links, and the current streak box.
class Sidebar {
public:
    static constexpr float kWidth = 230.0f;

    void render(AppContext& context, Renderer& renderer, const Input& input);
};
