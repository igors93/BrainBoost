#pragma once

#include <SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <map>
#include <string>

#include "ui/Rect.h"
#include "ui/Theme.h"

// 2D drawing layer: wraps SDL_Renderer and renders TrueType text through
// FreeType, caching one texture per glyph/size. All text is UTF-8 and `y`
// is always the top of the text line.
class Renderer {
public:
    bool init(SDL_Window* window);
    void shutdown();

    void beginFrame(Color clearColor);
    void endFrame();

    float width() const;
    float height() const;

    void fillRect(const Rect& rect, Color color);
    void outlineRect(const Rect& rect, Color color, int thickness = 2);
    void drawLine(float x1, float y1, float x2, float y2, Color color,
                  int thickness = 1);

    void drawText(const std::string& text, float x, float y, int size, Color color,
                  bool bold = false);
    void drawTextCentered(const std::string& text, float centerX, float y, int size,
                          Color color, bool bold = false);
    float textWidth(const std::string& text, int size, bool bold = false);
    float lineHeight(int size) const { return static_cast<float>(size) * 1.35f; }

    // Word-wraps `text` inside `maxWidth`; returns the total height drawn.
    float drawTextWrapped(const std::string& text, float x, float y, float maxWidth,
                          int size, Color color, bool bold = false);

    void setTranslation(float tx, float ty) {
        translationX_ = tx;
        translationY_ = ty;
    }
    void setClipRect(const Rect* rect);

private:
    struct Glyph {
        SDL_Texture* texture = nullptr;  // null for blank glyphs (e.g. space)
        int width = 0;
        int height = 0;
        int bearingX = 0;
        int bearingY = 0;
        int advance = 0;
    };

    bool loadFaces();
    FT_Face pickFace(bool bold) const;
    const Glyph& glyph(uint32_t codepoint, int size, bool bold);
    float ascent(int size, bool bold);

    SDL_Window* window_ = nullptr;
    SDL_Renderer* sdl_ = nullptr;
    FT_Library library_ = nullptr;
    FT_Face regular_ = nullptr;
    FT_Face bold_ = nullptr;

    float translationX_ = 0.0f;
    float translationY_ = 0.0f;

    std::map<uint64_t, Glyph> glyphCache_;
    std::map<int, float> ascentCache_;
};
