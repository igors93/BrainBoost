#pragma once

#include <string>
#include <vector>

#include "core/Statistics.h"
#include "ui/Rect.h"
#include "ui/Theme.h"
#include "ui/TextFieldModel.h"

class Renderer;
class Input;
struct GameInfo;

// Immediate-mode widgets: each function draws the widget this frame and
// reports interaction through its return value.
namespace Widgets {

// Preserve the existing Widgets::TextFieldState API while keeping the pure
// state model available without Renderer or SDL dependencies.
using ::TextFieldInput;
using ::TextFieldState;
using ::updateTextFieldState;

// Draw-only button used by const game rendering.
void drawButton(Renderer& renderer, const Rect& rect, const std::string& label,
                Color background = Theme::kButton, int fontSize = 16,
                bool hovered = false);

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

// Draw-only text field used by const game rendering.
void drawTextField(Renderer& renderer, const Rect& rect,
                   const std::string& text, bool focused,
                   int fontSize = 22);

// Editable single-line text box. Clicking inside focuses it; clicking outside
// or pressing Escape releases focus. Returns true when Enter is pressed while
// the field is focused.
bool textField(Renderer& renderer, const Input& input, TextFieldState& state,
               const Rect& rect, int fontSize = 22, bool numericOnly = false,
               size_t maxLength = 16);

}  // namespace Widgets
