#include <iostream>

#include "../TestUtils.h"
#include "ui/TextFieldModel.h"
#include "ui/Theme.h"

namespace {

void focus(TextFieldState& state, const Rect& field) {
    TextFieldInput input;
    input.primaryPressed = true;
    input.pointerX = field.x + 1.0f;
    input.pointerY = field.y + 1.0f;
    updateTextFieldState(state, input, field, false, 24);
}

void testThemeColors() {
    const Color bg = Theme::kBackground;
    TEST_CHECK(bg.r == 0x0A && bg.g == 0x0E && bg.b == 0x1A);
}

void testTextFieldFocus() {
    TextFieldState state;
    const Rect field{10.0f, 10.0f, 200.0f, 40.0f};
    TextFieldInput ignored;
    ignored.text = "ignored";
    updateTextFieldState(state, ignored, field, false, 20);
    TEST_CHECK(state.text.empty());
    focus(state, field);
    TEST_CHECK(state.focused);
}

void testUtf8CharactersAreNeverSplit() {
    TextFieldState state;
    const Rect field{0.0f, 0.0f, 200.0f, 40.0f};
    focus(state, field);

    TextFieldInput name;
    name.text = u8"João";
    updateTextFieldState(state, name, field, false, 4);
    TEST_CHECK(state.text == u8"João");
    TEST_CHECK(utf8CodepointCount(state.text) == 4);

    TextFieldInput overflow;
    overflow.text = u8"ç";
    updateTextFieldState(state, overflow, field, false, 4);
    TEST_CHECK(state.text == u8"João");

    TextFieldInput backspace;
    backspace.backspacePressed = true;
    updateTextFieldState(state, backspace, field, false, 4);
    TEST_CHECK(state.text == u8"Joã");
    TEST_CHECK(utf8CodepointCount(state.text) == 3);
}

void testNumericFieldRejectsUnicodeAndLetters() {
    TextFieldState state;
    const Rect field{0.0f, 0.0f, 100.0f, 30.0f};
    focus(state, field);
    TextFieldInput input;
    input.text = u8"12á3";
    updateTextFieldState(state, input, field, true, 3);
    TEST_CHECK(state.text == "123");
}

}  // namespace

int main() {
    std::cout << "Running UI model tests...\n";
    testThemeColors();
    testTextFieldFocus();
    testUtf8CharactersAreNeverSplit();
    testNumericFieldRejectsUnicodeAndLetters();
    std::cout << "All UI model tests passed!\n";
    return 0;
}
