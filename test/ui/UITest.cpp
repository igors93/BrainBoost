#include <iostream>
#include "ui/Theme.h"
#include "../TestUtils.h"

void testThemeColors() {
    Color bg = Theme::kBackground;
    TEST_CHECK(bg.r == 0x0A);
    TEST_CHECK(bg.g == 0x0E);
    TEST_CHECK(bg.b == 0x1A);
    TEST_CHECK(bg.a == 0xFF);
    
    Color cat0 = Theme::categoryColor(GameCategory::Logic);
    TEST_CHECK(cat0.r == 0xF4);
    TEST_CHECK(cat0.g == 0x72);
    TEST_CHECK(cat0.b == 0xB6);
}

int main() {
    std::cout << "Running UI tests..." << std::endl;
    testThemeColors();
    std::cout << "All UI tests passed!" << std::endl;
    return 0;
}
