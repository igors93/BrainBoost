#include "ui/Widgets.h"

#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <cstdio>

#include "core/GameInfo.h"
#include "ui/Input.h"
#include "ui/Renderer.h"

namespace Widgets {

bool button(Renderer& renderer, const Input& input, const Rect& rect,
            const std::string& label, Color background, int fontSize) {
    const bool hovered = rect.contains(input.mouseX(), input.mouseY());

    renderer.fillRect(rect, hovered ? lighten(background) : background);
    const float textY = rect.y + (rect.h - renderer.lineHeight(fontSize)) * 0.5f;
    renderer.drawTextCentered(label, rect.centerX(), textY, fontSize, Theme::kText);

    return hovered && input.mousePressed();
}

void statChip(Renderer& renderer, const Rect& rect, const std::string& label,
              const std::string& value, Color valueColor) {
    renderer.fillRect(rect, Theme::kPanel);
    renderer.drawText(label, rect.x + 14, rect.y + 8, 13, Theme::kTextMuted);
    renderer.drawText(value, rect.x + 14, rect.y + 26, 20, valueColor, true);
}

void skillBar(Renderer& renderer, const Rect& rect, const std::string& label,
              float value01, float labelWidth) {
    value01 = std::clamp(value01, 0.0f, 1.0f);

    const float valueWidth = 48.0f;
    const float barHeight = 8.0f;
    const float barWidth = std::max(30.0f, rect.w - labelWidth - valueWidth);

    renderer.drawText(label, rect.x, rect.y, 14, Theme::kText);

    const Rect bar{rect.x + labelWidth, rect.y + (rect.h - barHeight) * 0.5f,
                   barWidth, barHeight};
    renderer.fillRect(bar, Theme::kPanelSoft);
    if (value01 > 0.01f) {
        renderer.fillRect(Rect{bar.x, bar.y, bar.w * value01, bar.h}, Theme::kAccent);
    }

    char percent[16];
    std::snprintf(percent, sizeof(percent), "%.0f%%", value01 * 100.0f);
    renderer.drawText(percent, bar.right() + 10, rect.y, 14, Theme::kTextMuted);
}

void progressBar(Renderer& renderer, const Rect& rect, float fraction, Color fill) {
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    renderer.fillRect(rect, Theme::kPanelSoft);
    if (fraction > 0.0f) {
        renderer.fillRect(Rect{rect.x, rect.y, rect.w * fraction, rect.h}, fill);
    }
}

bool gameCard(Renderer& renderer, const Input& input, const Rect& rect,
              const GameInfo& info) {
    const bool hovered = rect.contains(input.mouseX(), input.mouseY());
    const bool playable = info.isImplemented();

    Color background = rgb(info.color);
    if (!playable) background.a = 110;  // dimmed "coming soon" card
    renderer.fillRect(rect, hovered && playable ? lighten(background, 14) : background);
    if (hovered && playable) renderer.outlineRect(rect, Theme::kAccent, 2);

    const float padding = 14.0f;
    const Color titleColor = playable ? Theme::kText : Theme::kTextMuted;
    renderer.drawText(info.title, rect.x + padding, rect.y + padding, 18, titleColor,
                      true);
    renderer.drawTextWrapped(info.description, rect.x + padding,
                             rect.y + padding + 30.0f, rect.w - padding * 2.0f, 14,
                             rgb(0xB9C1D4, playable ? 255 : 150));

    // Category tag: colored square + name, bottom-left.
    Color tag = Theme::categoryColor(info.category);
    if (!playable) tag.a = 150;
    renderer.fillRect(Rect{rect.x + padding, rect.bottom() - 26.0f, 10, 10}, tag);
    renderer.drawText(categoryName(info.category), rect.x + padding + 18,
                      rect.bottom() - 32.0f, 14, tag);

    if (!playable) {
        const std::string badge = "Em breve";
        const float badgeWidth = renderer.textWidth(badge, 13);
        renderer.drawText(badge, rect.right() - badgeWidth - padding,
                          rect.y + padding, 13, Theme::kTextMuted);
    }

    return playable && hovered && input.mousePressed();
}

void scoreLineChart(Renderer& renderer, const Input& input, const Rect& rect,
                    const ChartSeries& series) {
    const float axisWidth = 34.0f;
    const Rect plot{rect.x + axisWidth, rect.y + 8.0f, rect.w - axisWidth - 8.0f,
                    rect.h - 20.0f};

    // Recessive grid at 0 / 50 / 100 with muted axis labels.
    for (int i = 0; i <= 2; ++i) {
        const float y = plot.y + plot.h * (1.0f - static_cast<float>(i) * 0.5f);
        renderer.drawLine(plot.x, y, plot.right(), y, Theme::kGrid);
        char label[8];
        std::snprintf(label, sizeof(label), "%d", i * 50);
        renderer.drawText(label, rect.x + 4, y - 8.0f, 12, Theme::kTextMuted);
    }

    const int count = static_cast<int>(series.size());
    if (count < 2) {
        renderer.drawTextCentered("Complete jogos para ver sua evolução.",
                                  plot.centerX(), plot.centerY() - 8.0f, 14,
                                  Theme::kTextMuted);
        return;
    }

    const auto pointAt = [&](int index) {
        const float x = plot.x + plot.w * static_cast<float>(index) /
                                     static_cast<float>(count - 1);
        const float value = std::clamp(series[static_cast<size_t>(index)].score, 0.0f, 100.0f);
        const float y = plot.y + plot.h * (1.0f - value / 100.0f);
        return std::pair<float, float>{x, y};
    };

    // Single series: 2px accent line with a marker on the latest point.
    for (int i = 0; i + 1 < count; ++i) {
        const auto [x1, y1] = pointAt(i);
        const auto [x2, y2] = pointAt(i + 1);
        renderer.drawLine(x1, y1, x2, y2, Theme::kAccent, 2);
    }
    const auto [lastX, lastY] = pointAt(count - 1);
    renderer.fillRect(Rect{lastX - 4, lastY - 4, 8, 8}, Theme::kAccent);

    // Hover: vertical cursor plus a readout of the nearest session.
    if (plot.contains(input.mouseX(), input.mouseY())) {
        const float step = plot.w / static_cast<float>(count - 1);
        int index = static_cast<int>((input.mouseX() - plot.x) / step + 0.5f);
        index = std::clamp(index, 0, count - 1);

        const auto [x, y] = pointAt(index);
        renderer.drawLine(x, plot.y, x, plot.bottom(), rgb(0x8B93A7, 90));
        renderer.fillRect(Rect{x - 4, y - 4, 8, 8}, Theme::kText);

        char readout[48];
        std::snprintf(readout, sizeof(readout), "Sessão %d: %.0f pontos", index + 1,
                      series[static_cast<size_t>(index)].score);
        renderer.drawText(readout, plot.x + 4, plot.y - 4.0f, 13, Theme::kText);
    }
}

bool textField(Renderer& renderer, const Input& input, TextFieldState& state,
               const Rect& rect, int fontSize, bool numericOnly, size_t maxLength) {
    // Apply this frame's typing.
    for (char character : input.textTyped()) {
        if (state.text.size() >= maxLength) break;
        if (numericOnly && std::isdigit(static_cast<unsigned char>(character)) == 0) {
            continue;
        }
        state.text += character;
    }
    if (input.backspacePressed() && !state.text.empty()) {
        // Remove one UTF-8 codepoint (skip continuation bytes 10xxxxxx).
        size_t erase = state.text.size() - 1;
        while (erase > 0 &&
               (static_cast<uint8_t>(state.text[erase]) & 0xC0) == 0x80) {
            --erase;
        }
        state.text.erase(erase);
    }

    renderer.fillRect(rect, Theme::kPanelSoft);
    renderer.outlineRect(rect, Theme::kGrid, 1);

    const float textY = rect.y + (rect.h - renderer.lineHeight(fontSize)) * 0.5f;
    renderer.drawText(state.text, rect.x + 10, textY, fontSize, Theme::kText);

    // Blinking caret.
    if ((SDL_GetTicks64() / 500) % 2 == 0) {
        const float caretX =
            rect.x + 12 + renderer.textWidth(state.text, fontSize);
        renderer.fillRect(
            Rect{caretX, rect.y + 7, 2, rect.h - 14}, Theme::kAccent);
    }

    return input.enterPressed();
}

}  // namespace Widgets
