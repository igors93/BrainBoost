#include "ui/Renderer.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {

// Known DejaVu Sans locations (Flatpak runtime, Fedora, Debian, Arch).
struct FontPaths {
    const char* regular;
    const char* bold;
};
constexpr FontPaths kFontCandidates[] = {
    {"/usr/share/fonts/dejavu/DejaVuSans.ttf",
     "/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf"},
    {"/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
     "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans-Bold.ttf"},
    {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
     "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"},
    {"/usr/share/fonts/TTF/DejaVuSans.ttf",
     "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf"},
};

bool fileExists(const char* path) {
    if (std::FILE* file = std::fopen(path, "rb")) {
        std::fclose(file);
        return true;
    }
    return false;
}

// Decodes the UTF-8 codepoint starting at `i` and advances `i` past it.
uint32_t nextCodepoint(const std::string& text, size_t& i) {
    const auto byte = [&](size_t offset) {
        return static_cast<uint8_t>(text[i + offset]);
    };
    const uint8_t first = byte(0);

    if (first < 0x80) {
        i += 1;
        return first;
    }
    if ((first >> 5) == 0x6 && i + 1 < text.size()) {
        const uint32_t cp = ((first & 0x1F) << 6) | (byte(1) & 0x3F);
        i += 2;
        return cp;
    }
    if ((first >> 4) == 0xE && i + 2 < text.size()) {
        const uint32_t cp =
            ((first & 0x0F) << 12) | ((byte(1) & 0x3F) << 6) | (byte(2) & 0x3F);
        i += 3;
        return cp;
    }
    if ((first >> 3) == 0x1E && i + 3 < text.size()) {
        const uint32_t cp = ((first & 0x07) << 18) | ((byte(1) & 0x3F) << 12) |
                            ((byte(2) & 0x3F) << 6) | (byte(3) & 0x3F);
        i += 4;
        return cp;
    }
    i += 1;  // invalid byte: skip it
    return '?';
}

uint64_t glyphKey(uint32_t codepoint, int size, bool bold) {
    return (static_cast<uint64_t>(size) << 40) |
           (static_cast<uint64_t>(bold ? 1 : 0) << 32) | codepoint;
}

}  // namespace

bool Renderer::init(SDL_Window* window) {
    window_ = window;

    sdl_ = SDL_CreateRenderer(window, -1,
                              SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (sdl_ == nullptr) {
        std::fprintf(stderr, "Accelerated renderer unavailable (%s), using software.\n",
                     SDL_GetError());
        sdl_ = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (sdl_ == nullptr) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderDrawBlendMode(sdl_, SDL_BLENDMODE_BLEND);

    if (FT_Init_FreeType(&library_) != 0) {
        std::fprintf(stderr, "FT_Init_FreeType failed\n");
        return false;
    }
    return loadFaces();
}

bool Renderer::loadFaces() {
    for (const FontPaths& candidate : kFontCandidates) {
        if (!fileExists(candidate.regular)) continue;

        if (FT_New_Face(library_, candidate.regular, 0, &regular_) != 0) continue;
        if (!fileExists(candidate.bold) ||
            FT_New_Face(library_, candidate.bold, 0, &bold_) != 0) {
            bold_ = regular_;  // fall back to the regular face
        }
        return true;
    }
    std::fprintf(stderr, "No usable TTF font found (looked for DejaVu Sans).\n");
    return false;
}

void Renderer::shutdown() {
    for (auto& [key, glyph] : glyphCache_) {
        if (glyph.texture != nullptr) SDL_DestroyTexture(glyph.texture);
    }
    glyphCache_.clear();

    if (bold_ != nullptr && bold_ != regular_) FT_Done_Face(bold_);
    if (regular_ != nullptr) FT_Done_Face(regular_);
    if (library_ != nullptr) FT_Done_FreeType(library_);
    regular_ = bold_ = nullptr;
    library_ = nullptr;

    if (sdl_ != nullptr) SDL_DestroyRenderer(sdl_);
    sdl_ = nullptr;
}

void Renderer::beginFrame(Color clear) {
    SDL_SetRenderDrawColor(sdl_, clear.r, clear.g, clear.b, 255);
    SDL_RenderClear(sdl_);
}

void Renderer::endFrame() { SDL_RenderPresent(sdl_); }

float Renderer::width() const {
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(sdl_, &w, &h);
    return static_cast<float>(w);
}

float Renderer::height() const {
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(sdl_, &w, &h);
    return static_cast<float>(h);
}

void Renderer::fillRect(const Rect& rect, Color color) {
    SDL_SetRenderDrawColor(sdl_, color.r, color.g, color.b, color.a);
    const SDL_FRect area{rect.x, rect.y, rect.w, rect.h};
    SDL_RenderFillRectF(sdl_, &area);
}

void Renderer::outlineRect(const Rect& rect, Color color, int thickness) {
    SDL_SetRenderDrawColor(sdl_, color.r, color.g, color.b, color.a);
    for (int i = 0; i < thickness; ++i) {
        const SDL_FRect area{rect.x + static_cast<float>(i),
                             rect.y + static_cast<float>(i),
                             rect.w - static_cast<float>(i) * 2.0f,
                             rect.h - static_cast<float>(i) * 2.0f};
        SDL_RenderDrawRectF(sdl_, &area);
    }
}

void Renderer::drawLine(float x1, float y1, float x2, float y2, Color color,
                        int thickness) {
    SDL_SetRenderDrawColor(sdl_, color.r, color.g, color.b, color.a);
    // Thickness is approximated by drawing parallel lines offset on the
    // minor axis; good enough for charts and dividers.
    const bool mostlyHorizontal = (x2 - x1) * (x2 - x1) >= (y2 - y1) * (y2 - y1);
    for (int i = 0; i < thickness; ++i) {
        const float offset = static_cast<float>(i);
        if (mostlyHorizontal) {
            SDL_RenderDrawLineF(sdl_, x1, y1 + offset, x2, y2 + offset);
        } else {
            SDL_RenderDrawLineF(sdl_, x1 + offset, y1, x2 + offset, y2);
        }
    }
}

FT_Face Renderer::pickFace(bool bold) const { return bold ? bold_ : regular_; }

float Renderer::ascent(int size, bool bold) {
    const int key = size * 2 + (bold ? 1 : 0);
    if (auto it = ascentCache_.find(key); it != ascentCache_.end()) return it->second;

    FT_Face face = pickFace(bold);
    FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(size));
    const float value = static_cast<float>(face->size->metrics.ascender >> 6);
    ascentCache_[key] = value;
    return value;
}

const Renderer::Glyph& Renderer::glyph(uint32_t codepoint, int size, bool bold) {
    const uint64_t key = glyphKey(codepoint, size, bold);
    if (auto it = glyphCache_.find(key); it != glyphCache_.end()) return it->second;

    FT_Face face = pickFace(bold);
    FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(size));
    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER) != 0) {
        FT_Load_Char(face, '?', FT_LOAD_RENDER);
    }

    const FT_GlyphSlot slot = face->glyph;
    Glyph entry;
    entry.width = static_cast<int>(slot->bitmap.width);
    entry.height = static_cast<int>(slot->bitmap.rows);
    entry.bearingX = slot->bitmap_left;
    entry.bearingY = slot->bitmap_top;
    entry.advance = static_cast<int>(slot->advance.x >> 6);

    if (entry.width > 0 && entry.height > 0) {
        // White texture with the glyph coverage as alpha; tinted at draw time.
        std::vector<uint32_t> pixels(
            static_cast<size_t>(entry.width) * static_cast<size_t>(entry.height));
        for (int row = 0; row < entry.height; ++row) {
            const uint8_t* source = slot->bitmap.buffer +
                                    static_cast<size_t>(row) * slot->bitmap.pitch;
            for (int col = 0; col < entry.width; ++col) {
                pixels[static_cast<size_t>(row) * entry.width + col] =
                    (static_cast<uint32_t>(source[col]) << 24) | 0x00FFFFFF;
            }
        }
        entry.texture =
            SDL_CreateTexture(sdl_, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STATIC, entry.width, entry.height);
        if (entry.texture != nullptr) {
            SDL_UpdateTexture(entry.texture, nullptr, pixels.data(), entry.width * 4);
            SDL_SetTextureBlendMode(entry.texture, SDL_BLENDMODE_BLEND);
        }
    }
    return glyphCache_.emplace(key, entry).first->second;
}

void Renderer::drawText(const std::string& text, float x, float y, int size,
                        Color color, bool bold) {
    const float baseline = y + ascent(size, bold);
    float pen = x;

    size_t i = 0;
    while (i < text.size()) {
        const uint32_t codepoint = nextCodepoint(text, i);
        const Glyph& g = glyph(codepoint, size, bold);
        if (g.texture != nullptr) {
            SDL_SetTextureColorMod(g.texture, color.r, color.g, color.b);
            SDL_SetTextureAlphaMod(g.texture, color.a);
            const SDL_FRect destination{std::round(pen + static_cast<float>(g.bearingX)),
                                        std::round(baseline - static_cast<float>(g.bearingY)),
                                        static_cast<float>(g.width),
                                        static_cast<float>(g.height)};
            SDL_RenderCopyF(sdl_, g.texture, nullptr, &destination);
        }
        pen += static_cast<float>(g.advance);
    }
}

float Renderer::textWidth(const std::string& text, int size, bool bold) {
    float width = 0.0f;
    size_t i = 0;
    while (i < text.size()) {
        width += static_cast<float>(glyph(nextCodepoint(text, i), size, bold).advance);
    }
    return width;
}

void Renderer::drawTextCentered(const std::string& text, float centerX, float y,
                                int size, Color color, bool bold) {
    drawText(text, centerX - textWidth(text, size, bold) * 0.5f, y, size, color, bold);
}

float Renderer::drawTextWrapped(const std::string& text, float x, float y,
                                float maxWidth, int size, Color color, bool bold) {
    std::string line;
    std::string word;
    float cursorY = y;

    const auto flushLine = [&] {
        if (!line.empty()) drawText(line, x, cursorY, size, color, bold);
        cursorY += lineHeight(size);
        line.clear();
    };
    const auto pushWord = [&] {
        if (word.empty()) return;
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (textWidth(candidate, size, bold) <= maxWidth || line.empty()) {
            line = candidate;
        } else {
            flushLine();
            line = word;
        }
        word.clear();
    };

    for (char character : text) {
        if (character == ' ') {
            pushWord();
        } else if (character == '\n') {
            pushWord();
            flushLine();
        } else {
            word += character;
        }
    }
    pushWord();
    if (!line.empty()) flushLine();

    return cursorY - y;
}
