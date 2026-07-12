#include <iostream>

#include "../TestUtils.h"
#include "ui/TextFieldModel.h"
#include "ui/Theme.h"

namespace {

void testThemeColors() {
    const Color bg = Theme::kBackground;
    TEST_CHECK(bg.r == 0x0A);
    TEST_CHECK(bg.g == 0x0E);
    TEST_CHECK(bg.b == 0x1A);
    TEST_CHECK(bg.a == 0xFF);
}

void testTextFieldFocusAndTyping() {
    TextFieldState state;
    const Rect field{10.0f, 10.0f, 200.0f, 40.0f};

    TextFieldInput unfocusedTyping;
    unfocusedTyping.text = "ignored";
    TEST_CHECK(!updateTextFieldState(state, unfocusedTyping, field, false, 20));
    TEST_CHECK(state.text.empty());

    TextFieldInput focus;
    focus.primaryPressed = true;
    focus.pointerX = 20.0f;
    focus.pointerY = 20.0f;
    updateTextFieldState(state, focus, field, false, 20);
    TEST_CHECK(state.focused);

    TextFieldInput typing;
    typing.text = "Igor";
    updateTextFieldState(state, typing, field, false, 20);
    TEST_CHECK(state.text == "Igor");

    TextFieldInput submit;
    submit.confirmPressed = true;
    TEST_CHECK(updateTextFieldState(state, submit, field, false, 20));

    TextFieldInput clickOutside;
    clickOutside.primaryPressed = true;
    clickOutside.pointerX = 500.0f;
    clickOutside.pointerY = 500.0f;
    updateTextFieldState(state, clickOutside, field, false, 20);
    TEST_CHECK(!state.focused);
}

void testNumericFieldFiltersInputAndHandlesBackspace() {
    TextFieldState state;
    const Rect field{0.0f, 0.0f, 100.0f, 30.0f};

    TextFieldInput focusAndType;
    focusAndType.primaryPressed = true;
    focusAndType.pointerX = 5.0f;
    focusAndType.pointerY = 5.0f;
    focusAndType.text = "12a3";
    updateTextFieldState(state, focusAndType, field, true, 3);
    TEST_CHECK(state.text == "123");

    TextFieldInput backspace;
    backspace.backspacePressed = true;
    updateTextFieldState(state, backspace, field, true, 3);
    TEST_CHECK(state.text == "12");

    TextFieldInput escape;
    escape.cancelPressed = true;
    updateTextFieldState(state, escape, field, true, 3);
    TEST_CHECK(!state.focused);
}

}  // namespace

int main() {
    std::cout << "Running UI model tests...\n";
    testThemeColors();
    testTextFieldFocusAndTyping();
    testNumericFieldFiltersInputAndHandlesBackspace();
    std::cout << "All UI model tests passed!\n";
    return 0;
}
