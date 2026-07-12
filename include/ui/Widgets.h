#pragma once

#include <string>
#include <vector>

#include "ui/Rect.h"
#include "ui/Theme.h"
#include "core/Statistics.h"

class Renderer;
class Input;
struct GameInfo;

// Immediate-mode widgets: each function draws the widget this frame and
// reports interaction through its return value.
namespace Widgets {

// Returns true when clicked.
bool button(Renderer& renderer, const Input& input, const Rect& rect,
            const std::string& label, Color background = Theme::kButton,
            int fontSize = 16);

// Small stat box: muted label on top, bold colored value below.
void statChip(Renderer& renderer, const Rect& rect, const std::string& label,
              const std::string& value, Color valueColor);

// Horizontal bar with label on the left and percentage on the right.
void skillBar(Renderer& renderer, const Rect& rect, const std::string& label,
              float value01, float labelWidth = 110.0f);

// Thin progress bar (fraction 0..1).
void progressBar(Renderer& renderer, const Rect& rect, float fraction,
                 Color fill = Theme::kAccent);

// Game catalog card. Returns true when clicked and the game is playable.
bool gameCard(Renderer& renderer, const Input& input, const Rect& rect,
              const GameInfo& info);

// Single-series score chart (values 0..100, oldest first) with gridlines and
// a hover readout. Shows a placeholder while there is not enough data.
void scoreLineChart(Renderer& renderer, const Input& input, const Rect& rect,
                    const ChartSeries& series);

// Editable single-line text box (always focused; one per screen).
// Returns true when Enter is pressed.
struct TextFieldState {
    std::string text;
};
bool textField(Renderer& renderer, const Input& input, TextFieldState& state,
               const Rect& rect, int fontSize = 22, bool numericOnly = false,
               size_t maxLength = 16);

}  // namespace Widgets
